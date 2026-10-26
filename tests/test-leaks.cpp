// The Debug leak gate (spec §3.7.8 "zero CRT leak report in Debug"; reading R-45): Qt keeps process-lifetime
// allocations that an exit-time CRT report would list, so the gate counts the CRT debug heap's live blocks around
// repeated cycles instead — after a warm-up cycle, N more cycles of a subsystem leave no block behind. The cycles: a
// project edited, saved and read back; presets saved and listed; a video probed, decoded and closed; a loop encoded;
// an export end to end (CPU). In a Release build there is no debug heap: the cases skip.
#include "Samples.h"
#include "core/Presets.h"
#include "core/Project.h"
#include "core/ProjectJson.h"
#include "export/ExportJob.h"
#include "media/Decoder.h"
#include "media/Encoder.h"
#include "media/MediaProbe.h"
#include "media/WorkerThread.h"
#include "render/RenderService.h"

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#ifdef _DEBUG
#include <crtdbg.h>
#endif

#include <functional>
#include <vector>

using namespace avloop;

namespace {

long long liveBlocks() {
#ifdef _DEBUG
    _CrtMemState s;
    _CrtMemCheckpoint(&s);
    return s.lCounts[_NORMAL_BLOCK] + s.lCounts[_CLIENT_BLOCK];
#else
    return -1;
#endif
}

// The blocks a cycle leaves behind, from the second cycle on (the first one warms up what is made once).
long long growth(const std::function<void()>& cycle, int cycles) {
    cycle();
    QCoreApplication::processEvents();
    const long long before = liveBlocks();
    for (int i = 0; i < cycles; ++i)
        cycle();
    QCoreApplication::processEvents();
    return liveBlocks() - before;
}

} // namespace

class TestLeaks : public QObject {
    Q_OBJECT
private slots:
    void init() {
        if (liveBlocks() < 0) QSKIP("the CRT debug heap exists in a Debug build only");
    }
    void projectEditedSavedRead() {
        QTemporaryDir dir;
        const QString file = dir.filePath(QStringLiteral("p.avloop"));
        const long long g = growth(
            [&] {
                Project p;
                SourceInfo s;
                s.path = QStringLiteral("x.mp4");
                s.width = 640;
                s.height = 360;
                s.fps = {30, 1};
                s.duration = 10;
                p.setSource(s);
                CropShape c;
                c.kind = CropShape::Kind::Ellipse;
                c.rect = QRectF(10, 10, 300, 200);
                p.changeCrop(c);
                p.changeTransform(VideoTransform::initial({640, 360}));
                p.undoStack()->undo();
                p.undoStack()->redo();
                QVERIFY(projectjson::save(p.snapshot(), file, nullptr));
                ProjectData back;
                QVERIFY(projectjson::load(file, back, nullptr));
                p.restore(back);
            },
            20);
        QCOMPARE(g, 0LL);
    }
    void presetsSavedListed() {
        QTemporaryDir dir;
        const long long g = growth(
            [&] {
                Preset p;
                p.name = QStringLiteral("Mine");
                QVERIFY(presets::save(dir.path(), p));
                QCOMPARE(presets::all(dir.path()).size(), 4);
                QVERIFY(presets::remove(dir.path(), p.name));
            },
            20);
        QCOMPARE(g, 0LL);
    }
    void videoDecodedClosed() {
        const QString video = test::samplePath(QStringLiteral("counter-640p30.mp4"));
        if (!QFileInfo::exists(video)) QSKIP("counter-640p30.mp4 is missing");
        const long long g = growth(
            [&] {
                const media::MediaProbe::Result r = media::MediaProbe::probe(video);
                QVERIFY(r.info);
                media::Decoder::Options o;
                o.hardware = false;
                media::Decoder d(*r.info, o);
                QVERIFY(d.open());
                for (int k : {0, 45, 10, 200})
                    QVERIFY(!d.frameAtIndex(k).isNull());
            },
            8);
        QCOMPARE(g, 0LL);
    }
    void loopEncoded() {
        QTemporaryDir dir;
        std::vector<std::uint8_t> px(64 * 64 * 4, 128);
        const long long g = growth(
            [&] {
                media::EncodeSettings s;
                s.size = {64, 64};
                s.frames = 5;
                media::Encoder e(s);
                QVERIFY(e.open(dir.filePath(QStringLiteral("e.webm"))));
                for (int k = 0; k < 5; ++k)
                    QVERIFY(e.write(px.data(), 64 * 4));
                QVERIFY(e.finish());
            },
            8);
        QCOMPARE(g, 0LL);
    }
    void exportEndToEnd() {
        const QString video = test::samplePath(QStringLiteral("counter-640p30.mp4"));
        const media::MediaProbe::Result probed = media::MediaProbe::probe(video);
        if (!probed.info) QSKIP("counter-640p30.mp4 is missing");
        ProjectData p;
        p.source.path = video;
        p.range = {2.0, 2.5};
        p.transform = VideoTransform::initial(QSizeF(probed.info->width, probed.info->height));
        p.crop.kind = CropShape::Kind::Ellipse;
        p.crop.rect = QRectF(220, 130, 200, 100);
        p.loop.mode = LoopSettings::Mode::Simple;
        const render::RenderJob job = render::RenderJob::of(p, *probed.info);
        auto context = std::make_unique<QObject>();
        media::WorkerThread thread(QStringLiteral("test export"));
        context->moveToThread(&thread);
        render::RenderService service;
        QTemporaryDir dir;
        const long long g = growth(
            [&] {
                exporting::ExportJob exporter(job, dir.filePath(QStringLiteral("x.webm")), &service, context.get());
                exporter.setPreferGpu(false);
                QSignalSpy finished(&exporter, &exporting::ExportJob::finished);
                exporter.start();
                QVERIFY(finished.wait(120000));
                QCOMPARE(exporter.state(), exporting::ExportJob::State::Done);
                service.run([] {}); // the renderer let go on the render thread
            },
            3);
        QCOMPARE(g, 0LL);
    }
};

QTEST_MAIN(TestLeaks)
#include "test-leaks.moc"
