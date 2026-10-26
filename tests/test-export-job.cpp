// The export job (spec §10.1, §10.3, §12.3) on a sample, on the CPU (the offscreen platform has no OpenGL): the loop
// written and checked (frame count, rate, size, codec, alpha, first keyframe), no `.part` left; Cancel leaves nothing;
// a destination held open is written beside it as `name (1).webm`; Two-pass runs two passes; Target size lands
// within 5 %. And the pieces: the default file name, the alternative name, the size estimate's arithmetic.
#include "Samples.h"
#include "export/ExportJob.h"
#include "export/FormatProfiles.h"
#include "export/OutputVerify.h"
#include "export/SizeEstimator.h"
#include "media/MediaProbe.h"
#include "media/WorkerThread.h"
#include "render/RenderService.h"

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include <windows.h>

#include <memory>
#include <optional>

using namespace avloop;

namespace {

// counter-640p30, 2 s to 4.5 s, an ellipse with a soft edge: a loop with alpha.
std::optional<render::RenderJob> sampleJob(const QualitySettings& quality = {},
                                           OutputFormat format = OutputFormat::WebmVp9) {
    const QString video = test::samplePath(QStringLiteral("counter-640p30.mp4"));
    const media::MediaProbe::Result probed = media::MediaProbe::probe(video);
    if (!probed.info) return std::nullopt;
    ProjectData p;
    p.source.path = video;
    p.range = {2.0, 4.5};
    p.transform = VideoTransform::initial(QSizeF(probed.info->width, probed.info->height));
    p.crop.kind = CropShape::Kind::Ellipse;
    p.crop.rect = QRectF(120, 40, 400, 280);
    p.crop.feather = 8;
    p.output.format = format;
    p.quality = quality;
    return render::RenderJob::of(p, *probed.info);
}

} // namespace

class TestExportJob : public QObject {
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

    void names() {
        QCOMPARE(
            exporting::defaultFileName(QStringLiteral("D:/clips/fire.mp4"), QSize(640, 640), OutputFormat::WebmVp9),
            QStringLiteral("fire_loop_640x640.webm"));
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("loop.webm"));
        QCOMPARE(exporting::alternativePath(path), dir.filePath(QStringLiteral("loop (1).webm")));
        QFile taken(dir.filePath(QStringLiteral("loop (1).webm")));
        QVERIFY(taken.open(QIODevice::WriteOnly));
        taken.close();
        QCOMPARE(exporting::alternativePath(path), dir.filePath(QStringLiteral("loop (2).webm")));
    }
    void estimateArithmetic() {
        QCOMPARE(exporting::SizeEstimator::partStarts(30), QList<int>{0});
        QCOMPARE(exporting::SizeEstimator::partStarts(150), (QList<int>{0, 70, 140}));
        // One keyframe of 10 000 bytes, the other frames 1 000 each: 10 000 + 1 000 × 149 + the container.
        const QList<exporting::SizeEstimator::Part> parts(3, {10, 19000, 10000});
        QCOMPARE(exporting::SizeEstimator::extrapolate(parts, 150), std::int64_t(10000 + 149000 + 1024 + 16 * 150));
        QCOMPARE(exporting::SizeEstimator::sizeText(1887437), QStringLiteral("1.8 MB"));
        QCOMPARE(exporting::SizeEstimator::sizeText(293601), QStringLiteral("0.28 MB"));
        QCOMPARE(exporting::ExportJob::targetKbps(1.0, 10.0), 755); // 1 MB × 8 / 10 s × 0.9
    }

    void exportsAndChecks() {
        const auto job = sampleJob();
        if (!job) QSKIP("counter-640p30.mp4 is missing (tools/media/make-samples.mjs)");
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("loop.webm"));
        const exporting::ExportJob::Result r = run(*job, path);
        QCOMPARE(r.error, QString());
        QCOMPARE(r.path, path);
        QVERIFY(!QFileInfo::exists(path + QStringLiteral(".part")));
        const exporting::Verified v =
            exporting::verifyOutput(path, {OutputFormat::WebmVp9, job->outputSize, job->fps, job->plan.length, true});
        QCOMPARE(v.problem, QString());
        QCOMPARE(v.frames, job->plan.length); // the default Crossfade: D − F
        QVERIFY(v.alphaMarked && v.alphaPlane && v.firstKey);
        QCOMPARE(v.audioStreams, 0);
        // The check finds what is wrong.
        const exporting::Verified wrong =
            exporting::verifyOutput(path, {OutputFormat::WebmVp9, job->outputSize, job->fps, 76, true});
        QCOMPARE(wrong.problem, QStringLiteral("%1 frames, not 76").arg(job->plan.length));
    }
    void cancelLeavesNothing() {
        const auto job = sampleJob();
        if (!job) QSKIP("counter-640p30.mp4 is missing");
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("cancelled.webm"));
        exporting::ExportJob exporter(*job, path, service_.get(), context_.get());
        exporter.setPreferGpu(false);
        QSignalSpy finished(&exporter, &exporting::ExportJob::finished);
        connect(&exporter, &exporting::ExportJob::progressed, &exporter, [&] {
            if (exporter.done() >= 10) exporter.cancel();
        });
        exporter.start();
        QVERIFY(finished.wait(60000));
        QCOMPARE(exporter.state(), exporting::ExportJob::State::Cancelled);
        QVERIFY(!QFileInfo::exists(path));
        QVERIFY(!QFileInfo::exists(path + QStringLiteral(".part")));
    }
    void lockedDestinationGoesBeside() {
        const auto job = sampleJob();
        if (!job) QSKIP("counter-640p30.mp4 is missing");
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("held.webm"));
        QFile old(path);
        QVERIFY(old.open(QIODevice::WriteOnly));
        old.close();
        // Held as OBS's Media Source holds a file it plays: no sharing.
        HANDLE held = CreateFileW(reinterpret_cast<LPCWSTR>(QDir::toNativeSeparators(path).utf16()), GENERIC_READ, 0,
                                  nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        QVERIFY(held != INVALID_HANDLE_VALUE);
        const exporting::ExportJob::Result r = run(*job, path);
        CloseHandle(held);
        QCOMPARE(r.error, QString());
        QVERIFY(r.movedAside);
        QCOMPARE(r.path, dir.filePath(QStringLiteral("held (1).webm")));
        QVERIFY(QFileInfo(r.path).size() > 0);
        QCOMPARE(QFileInfo(path).size(), qint64(0)); // the held file untouched
        QVERIFY(!QFileInfo::exists(path + QStringLiteral(".part")));
    }
    void twoPassAndTarget_data() {
        QTest::addColumn<int>("format");
        QTest::addColumn<double>("targetMB");
        QTest::newRow("VP9 two-pass") << int(OutputFormat::WebmVp9) << 0.0;
        QTest::newRow("VP9 target 0.1 MB") << int(OutputFormat::WebmVp9) << 0.1;
        QTest::newRow("VP8 target 0.1 MB") << int(OutputFormat::WebmVp8) << 0.1;
    }
    void twoPassAndTarget() {
        QFETCH(int, format);
        QFETCH(double, targetMB);
        QualitySettings q;
        q.preset = QualitySettings::Preset::Custom;
        q.twoPass = true;
        q.targetMB = targetMB;
        const auto job = sampleJob(q, OutputFormat(format));
        if (!job) QSKIP("counter-640p30.mp4 is missing");
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("passes.webm"));
        int passes = 0;
        const exporting::ExportJob::Result r = run(*job, path, &passes);
        QCOMPARE(r.error, QString());
        QCOMPARE(r.verifyProblem, QString());
        QVERIFY(passes >= 2);
        if (targetMB > 0) {
            const double want = targetMB * 1048576.0;
            const QString seen = QStringLiteral("%1 bytes for %2").arg(r.bytes).arg(want);
            QVERIFY2(std::abs(r.bytes - want) <= 0.05 * want, qPrintable(seen));
        }
    }

private:
    exporting::ExportJob::Result run(const render::RenderJob& job, const QString& path, int* passes = nullptr) {
        exporting::ExportJob exporter(job, path, service_.get(), context_.get());
        exporter.setPreferGpu(false);
        QSignalSpy finished(&exporter, &exporting::ExportJob::finished);
        exporter.start();
        if (!finished.wait(120000)) return {};
        if (passes) *passes = exporter.passes();
        return exporter.result();
    }

    std::unique_ptr<media::WorkerThread> thread_;
    std::unique_ptr<QObject> context_;
    std::unique_ptr<render::RenderService> service_;
};

QTEST_MAIN(TestExportJob)
#include "test-export-job.moc"
