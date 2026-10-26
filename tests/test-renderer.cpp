// GPU/CPU parity (AVADR-0001, spec §3.6): every unit case within 2/255 per channel; four projects — a Rect crop, an
// Ellipse with Feather, a Crossfade, clip effects (D-017) — frame by frame within a mean of 1.0 and a maximum of 4; one
// shared GPU context draws a second look of the same frames, never the first one's textures. Without an OpenGL 3.3
// context the test says so and is skipped (CI; Remote Desktop).
#include "RenderCases.h"
#include "Samples.h"
#include "media/MediaProbe.h"
#include "render/FrameRenderer.h"
#include "render/GlRenderer.h"

#include <QTest>

#include <cmath>

using namespace avloop;
using namespace avloop::test;

namespace {

struct Diff {
    double mean = 0;
    int max = 0;
};

Diff diff(const QImage& a, const QImage& b) {
    Diff d;
    double sum = 0;
    for (int y = 0; y < a.height(); ++y)
        for (int x = 0; x < a.width() * 4; ++x) {
            const int v = std::abs(a.constScanLine(y)[x] - b.constScanLine(y)[x]);
            sum += v;
            d.max = std::max(d.max, v);
        }
    d.mean = sum / (double(a.width()) * a.height() * 4);
    return d;
}

ProjectData project(const QString& sample) {
    ProjectData p;
    p.source.path = samplePath(sample);
    p.range = {2.0, 5.0};
    p.transform = VideoTransform::initial({640, 360});
    return p;
}

} // namespace

class TestRenderer : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {
        render::GlRenderer probe;
        QString why;
        if (!probe.init(&why))
            QSKIP(qPrintable(QStringLiteral("no OpenGL 3.3 here (%1): the GPU half is skipped").arg(why)));
    }
    void unitCases() {
        render::GlRenderer gl;
        QVERIFY(gl.init());
        render::CpuRenderer cpu;
        for (const Case& c : cases()) {
            QImage a(c.composite.size, QImage::Format_RGBA8888_Premultiplied);
            QImage b(c.composite.size, QImage::Format_RGBA8888_Premultiplied);
            QVERIFY(gl.render(c.composite, a.bits(), int(a.bytesPerLine())));
            cpu.render(c.composite, b.bits(), int(b.bytesPerLine()));
            const Diff d = diff(a, b);
            QVERIFY2(d.max <= 2, qPrintable(QStringLiteral("%1: max %2").arg(QString::fromLatin1(c.name)).arg(d.max)));
        }
    }
    void projects_data() {
        QTest::addColumn<int>("which");
        QTest::newRow("Rect crop") << 0;
        QTest::newRow("Ellipse + Feather") << 1;
        QTest::newRow("Crossfade") << 2;
        QTest::newRow("Clip effects") << 3;
    }
    void projects() {
        AVLOOP_NEED_SAMPLE(QStringLiteral("counter-640p30.mp4"));
        QFETCH(int, which);
        ProjectData p = project(QStringLiteral("counter-640p30.mp4"));
        p.loop.mode = LoopSettings::Mode::Simple;
        if (which == 0) p.crop.rect = QRectF(100.5, 40.25, 300, 200); // a fractional place, scaled to the output
        if (which == 1) {
            p.crop.kind = CropShape::Kind::Ellipse;
            p.crop.rect = QRectF(160, 40, 280, 280);
            p.crop.feather = 16;
            p.transform.scale = 1.3; // a scaled video under it
        }
        if (which == 2) {
            p.loop.mode = LoopSettings::Mode::Crossfade;
            p.loop.fade = 0.5;
            p.transform.flipH = true;
        }
        if (which == 3) {
            p.effects.exposure = 0.7;
            p.effects.hue = 40;
            p.effects.vignette = 60;
            p.effects.blur = 3;
            p.effects.sharpen = 40;
            p.effects.opacity = 80;
        }
        const auto probed = media::MediaProbe::probe(p.source.path);
        QVERIFY(probed.info);
        const render::RenderJob job = render::RenderJob::of(p, *probed.info);
        render::FrameRenderer gpu(job, nullptr, true);
        render::FrameRenderer cpu(job, nullptr, false);
        QVERIFY(gpu.open());
        QVERIFY(cpu.open());
        QCOMPARE(gpu.backend(), render::FrameRenderer::Backend::Gpu);
        const int l = gpu.length();
        for (int k : {0, l / 3, l / 2, l - 2, l - 1}) {
            QImage a, b;
            QVERIFY(gpu.render(k, a));
            QVERIFY(cpu.render(k, b));
            const Diff d = diff(a, b);
            QVERIFY2(d.mean <= 1.0 && d.max <= 4,
                     qPrintable(QStringLiteral("frame %1: mean %2 max %3").arg(k).arg(d.mean).arg(d.max)));
        }
    }
    void sharedContextLooks() {
        AVLOOP_NEED_SAMPLE(QStringLiteral("counter-640p30.mp4"));
        ProjectData p = project(QStringLiteral("counter-640p30.mp4"));
        const auto probed = media::MediaProbe::probe(p.source.path);
        QVERIFY(probed.info);
        render::GlRenderer shared;
        QVERIFY(shared.init());
        const render::RenderJob plain = render::RenderJob::of(p, *probed.info);
        p.effects.invert = true;
        const render::RenderJob inverted = render::RenderJob::of(p, *probed.info);
        render::FrameRenderer first(plain, nullptr, true, &shared);
        render::FrameRenderer second(inverted, nullptr, true, &shared);
        render::FrameRenderer cpu(inverted, nullptr, false);
        QVERIFY(first.open() && second.open() && cpu.open());
        QImage a, b, c;
        QVERIFY(first.render(0, a));
        QVERIFY(second.render(0, b)); // the same source frame, another look: not the pooled texture
        QVERIFY(cpu.render(0, c));
        QVERIFY(diff(a, b).mean > 20);
        const Diff d = diff(b, c);
        QVERIFY2(d.max <= 4, qPrintable(QStringLiteral("max %1").arg(d.max)));
    }
};

QTEST_MAIN(TestRenderer)
#include "test-renderer.moc"
