// The batch queue (spec §10.5) on a sample, on the CPU: three entries run one after another in the list's order and
// each file is checked; an entry whose video is gone fails with its reason and the next one runs; Pause stops the
// entry under way, which is Pending again; the queue file brings the entries back, one that was running as Pending; a
// second queue on the same file is not saved.
#include "Samples.h"
#include "export/ExportJob.h"
#include "export/ExportQueue.h"
#include "export/OutputVerify.h"
#include "media/MediaProbe.h"
#include "media/WorkerThread.h"
#include "render/RenderService.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include <memory>
#include <optional>

using namespace avloop;
using exporting::ExportQueue;

namespace {

// counter-640p30, 2 s to 3 s, an ellipse, the long side 256: a short loop with alpha.
std::optional<ProjectData> sampleProject(OutputFormat format) {
    const QString video = test::samplePath(QStringLiteral("counter-640p30.mp4"));
    const media::MediaProbe::Result probed = media::MediaProbe::probe(video);
    if (!probed.info) return std::nullopt;
    ProjectData p;
    p.source.path = video;
    p.range = {2.0, 3.0};
    p.transform = VideoTransform::initial(QSizeF(probed.info->width, probed.info->height));
    p.crop.kind = CropShape::Kind::Ellipse;
    p.crop.rect = QRectF(120, 40, 400, 280);
    p.output.format = format;
    p.output.size = OutputSettings::Size::LongSide;
    p.output.longSide = 256;
    return p;
}

QStringList statesOf(const ExportQueue& q) {
    QStringList s;
    for (const ExportQueue::Entry& e : q.entries())
        s << ExportQueue::stateId(e.state);
    return s;
}

} // namespace

class TestExportQueue : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {
        thread_ = std::make_unique<media::WorkerThread>(QStringLiteral("test export"));
        context_ = std::make_unique<QObject>();
        context_->moveToThread(thread_.get());
        service_ = std::make_unique<render::RenderService>();
    }
    void cleanupTestCase() {
        service_.reset();
        thread_.reset();
        context_.reset();
    }

    void runsInOrderAndMovesOnAfterAFailure() {
        const auto gif = sampleProject(OutputFormat::Gif);
        if (!gif) QSKIP("counter-640p30.mp4 is missing (tools/media/make-samples.mjs)");
        QTemporaryDir dir;
        ExportQueue queue(dir.filePath(QStringLiteral("queue.json")), maker());
        QVERIFY(queue.isSaved());
        QStringList started; // the order the entries ran in
        connect(&queue, &ExportQueue::changed, this, [&] {
            for (const ExportQueue::Entry& e : queue.entries())
                if (e.state == ExportQueue::State::Running && !started.contains(e.id)) started << e.id;
        });
        ProjectData gone = *gif;
        gone.source.path = dir.filePath(QStringLiteral("gone.mp4"));
        queue.add(*sampleProject(OutputFormat::WebmVp9), {256, 180}, dir.filePath(QStringLiteral("a.webm")));
        queue.add(gone, {256, 180}, dir.filePath(QStringLiteral("b.gif")));
        queue.add(*gif, {256, 180}, dir.filePath(QStringLiteral("c.gif")));
        queue.add(*sampleProject(OutputFormat::MovProRes), {256, 180}, dir.filePath(QStringLiteral("d.mov")));
        // Dragged: d before a.
        const QString a = queue.entries()[0].id, b = queue.entries()[1].id, c = queue.entries()[2].id,
                      d = queue.entries()[3].id;
        queue.reorder({d, a, b, c});
        QCOMPARE(queue.entries()[0].id, d);

        QSignalSpy startedChanged(&queue, &ExportQueue::startedChanged);
        queue.start();
        QVERIFY(queue.isStarted());
        QTRY_VERIFY_WITH_TIMEOUT(!queue.isStarted(), 120000);
        QCOMPARE(statesOf(queue), (QStringList{"done", "done", "failed", "done"}));
        QCOMPARE(started, (QStringList{d, a, c})); // one at a time, in the list's order; the failed one never ran
        QVERIFY(queue.entries()[2].reason.contains(QStringLiteral("cannot be opened")));
        for (int i : {0, 1, 3}) {
            const ExportQueue::Entry& e = queue.entries()[i];
            QVERIFY(QFileInfo(e.output).size() > 0);
            QVERIFY(e.bytes > 0);
            QCOMPARE(e.progress, 1.0);
        }

        queue.clearDone();
        QCOMPARE(statesOf(queue), QStringList{"failed"});
    }
    void pauseStopsTheEntryUnderWay() {
        const auto p = sampleProject(OutputFormat::WebmVp9);
        if (!p) QSKIP("counter-640p30.mp4 is missing");
        QTemporaryDir dir;
        ExportQueue queue(dir.filePath(QStringLiteral("queue.json")), maker());
        queue.add(*p, {256, 180}, dir.filePath(QStringLiteral("a.webm")));
        queue.add(*p, {256, 180}, dir.filePath(QStringLiteral("b.webm")));
        connect(&queue, &ExportQueue::progressed, &queue, [&](int i) {
            if (queue.entries()[i].progress > 0.2) queue.pause();
        });
        queue.start();
        QTRY_VERIFY_WITH_TIMEOUT(!queue.isBusy(), 60000);
        QCOMPARE(statesOf(queue), (QStringList{"pending", "pending"}));
        QVERIFY(!queue.isStarted());
        QVERIFY(!QFileInfo::exists(dir.filePath(QStringLiteral("a.webm"))));
        QVERIFY(!QFileInfo::exists(dir.filePath(QStringLiteral("a.webm.part"))));
    }
    void restartBringsThemBackAsPending() {
        const auto p = sampleProject(OutputFormat::Apng);
        if (!p) QSKIP("counter-640p30.mp4 is missing");
        QTemporaryDir dir;
        const QString file = dir.filePath(QStringLiteral("queue.json"));
        {
            ExportQueue queue(file, maker());
            queue.add(*p, {256, 180}, dir.filePath(QStringLiteral("a.apng")));
            queue.add(*p, {256, 180}, dir.filePath(QStringLiteral("b.apng")));
            // A second AvLoop meanwhile: it neither reads nor writes the file.
            ExportQueue other(file, maker());
            QVERIFY(!other.isSaved());
            QCOMPARE(other.entries().size(), 0);
        }
        // As a crash leaves it: the first entry running.
        QFile f(file);
        QVERIFY(f.open(QIODevice::ReadOnly));
        QJsonObject root = QJsonDocument::fromJson(f.readAll()).object();
        f.close();
        QJsonArray entries = root.value(QStringLiteral("entries")).toArray();
        QJsonObject first = entries[0].toObject();
        first[QStringLiteral("state")] = QStringLiteral("running");
        entries[0] = first;
        root[QStringLiteral("entries")] = entries;
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(QJsonDocument(root).toJson());
        f.close();

        ExportQueue reopened(file, maker());
        QVERIFY(reopened.isSaved());
        QCOMPARE(statesOf(reopened), (QStringList{"pending", "pending"}));
        QCOMPARE(reopened.entries()[0].format, QStringLiteral("apng"));
        QCOMPARE(reopened.entries()[0].size, QSize(256, 180));
        QCOMPARE(QFileInfo(reopened.entries()[1].output).fileName(), QStringLiteral("b.apng"));
        // The snapshot is whole: it runs.
        reopened.start();
        QTRY_VERIFY_WITH_TIMEOUT(!reopened.isStarted(), 60000);
        QCOMPARE(statesOf(reopened), (QStringList{"done", "done"}));
    }

private:
    ExportQueue::JobMaker maker() {
        return [this](const render::RenderJob& job, const QString& path) {
            auto j = std::make_unique<exporting::ExportJob>(job, path, service_.get(), context_.get());
            j->setPreferGpu(false);
            return j;
        };
    }

    std::unique_ptr<media::WorkerThread> thread_;
    std::unique_ptr<QObject> context_;
    std::unique_ptr<render::RenderService> service_;
};

QTEST_MAIN(TestExportQueue)
#include "test-export-queue.moc"
