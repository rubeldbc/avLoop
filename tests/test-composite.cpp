// The composite (D-032): one plain clip made by it as AvLoop's own path makes it (with a matte and keyframes too); the
// Razor's halves showing their own media times; a hole cut in V2 showing V1; a keyed opacity; the matte's hole clear;
// eight tracks on two lanes within eight decoders; a project with a layer taken to the composite.
#include "Samples.h"
#include "core/SequenceEdit.h"
#include "media/MediaProbe.h"
#include "render/FrameRenderer.h"
#include "render/SequenceRenderer.h"

#include <QTest>

#include <cmath>

using namespace avloop;
using namespace avloop::test;
namespace se = avloop::sequenceedit;

namespace {

const QString kCounter = QStringLiteral("counter-640p30.mp4");

Clip counter(double scale = 1) {
    Clip c;
    c.name = QStringLiteral("counter");
    c.path = samplePath(kCounter);
    c.size = QSizeF(640, 360);
    c.duration = 10;
    c.range = {0, 10};
    c.transform = VideoTransform::initial({640, 360}); // its centre at (320, 180)
    c.transform.scale = scale;
    return c;
}

Sequence tracks(int n) {
    Sequence s;
    s.frame = QRectF(0, 0, 640, 360);
    s.inOut = {0, 10};
    for (int i = 0; i < n; ++i)
        s.tracks.append({QStringLiteral("V%1").arg(i + 1), true, false, {}});
    return s;
}

QImage at(const Sequence& s, double t) {
    render::SequenceRenderer r(s, QRectF(0, 0, 640, 360), QSize(640, 360), LoopSettings::Method::Nearest, true,
                               Qt::black);
    QImage out;
    if (!r.render({{t, 1, 0, 0}}, out)) return {};
    return out;
}

// The counter's number in an output frame (Samples.h's reading, on the image).
int counterOf(const QImage& img) {
    media::Frame f;
    f.width = img.width();
    f.height = img.height();
    f.stride = int(img.bytesPerLine());
    f.data = std::shared_ptr<const std::uint8_t>(img.constBits(), [](const std::uint8_t*) {});
    return readCounter(f);
}

int maxDiff(const QImage& a, const QImage& b) {
    int most = 0;
    for (int y = 0; y < a.height(); ++y)
        for (int x = 0; x < a.width() * 4; ++x)
            most = std::max(most, std::abs(a.constScanLine(y)[x] - b.constScanLine(y)[x]));
    return most;
}

QRgb px(const QImage& img, int x, int y) {
    return reinterpret_cast<const QRgb*>(img.constScanLine(y))[x];
}

} // namespace

class TestComposite : public QObject {
    Q_OBJECT
private slots:
    void plainAsAvLoop_data() {
        QTest::addColumn<int>("which");
        QTest::newRow("plain") << 0;
        QTest::newRow("ellipse matte, keyframes") << 1;
        QTest::newRow("inverted matte, blend") << 2;
    }
    void plainAsAvLoop() {
        AVLOOP_NEED_SAMPLE(kCounter);
        QFETCH(int, which);
        ProjectData p;
        p.source.path = samplePath(kCounter);
        p.range = {2.0, 5.0};
        p.transform = VideoTransform::initial({640, 360});
        if (which == 1) {
            p.crop.kind = CropShape::Kind::Ellipse;
            p.crop.rect = QRectF(160, 40, 280, 280);
            p.crop.feather = 16;
            p.keyframes = {{2, 320, 180, 1.2, false}, {4, 300, 200, 1.4, true}};
        }
        if (which == 2) {
            p.crop.kind = CropShape::Kind::Rounded;
            p.crop.rect = QRectF(200, 100, 200, 120);
            p.crop.radius = 20;
            p.crop.feather = 6;
            p.crop.invert = true;
            p.loop.mode = LoopSettings::Mode::Crossfade;
            p.loop.fade = 0.5;
        }
        const auto probed = media::MediaProbe::probe(p.source.path);
        QVERIFY(probed.info);
        const render::RenderJob job = render::RenderJob::of(p, *probed.info);
        QVERIFY(!job.composite);
        render::FrameRenderer avloop(job, nullptr, false);
        QVERIFY(avloop.open());
        render::SequenceRenderer composite(job.sequence, job.frame, job.outputSize, job.loop.method, job.alpha,
                                           job.background);
        const LoopPlan& plan = job.plan;
        for (int k : {0, plan.length / 2, plan.length - 1}) {
            const Pick pick = loopmap::pick(job.loop.mode, plan.segment, plan.fade, plan.turn, job.loop.curve, k);
            QList<render::SequenceRenderer::Term> terms;
            if (pick.w < 1) terms.append({job.tau(pick.a), 1 - pick.w, pick.a, 0});
            if (pick.w > 0) terms.append({job.tau(pick.b), pick.w, pick.b, 1});
            QImage a, b;
            QVERIFY(avloop.render(k, a));
            QVERIFY(composite.render(terms, b));
            QVERIFY2(maxDiff(a, b) <= 1, qPrintable(QStringLiteral("frame %1: max %2").arg(k).arg(maxDiff(a, b))));
        }
    }
    void razorHalves() {
        AVLOOP_NEED_SAMPLE(kCounter);
        Sequence s = tracks(1);
        int a = 0, b = 0;
        QVERIFY(se::insert(s, 0, 0, counter(), &a));
        QVERIFY(se::split(s, a, 2, &b));
        QVERIFY(se::trimIn(s, b, 3));  // its media from 3 s, a gap 2–3 …
        QVERIFY(se::move(s, b, 2, 0)); // … closed: at 2 s the media's 3 s
        QCOMPARE(counterOf(at(s, 1.5)), 45);
        QCOMPARE(counterOf(at(s, 2.5)), 105);
    }
    void holeShowsBelow() {
        AVLOOP_NEED_SAMPLE(kCounter);
        Sequence below = tracks(2), above = tracks(2), both = tracks(2);
        Clip top = counter();
        top.range = {5, 10}; // frame 150 at 0 s
        MaskShape hole;
        hole.kind = MaskShape::Kind::Ellipse;
        hole.role = MaskShape::Role::Cut;
        hole.rect = QRectF(100, 130, 200, 100);
        top.masks = {hole};
        Clip mirrored = counter(); // other colours than V2's under the hole and around it
        mirrored.transform.flipH = true;
        QVERIFY(se::insert(below, 0, 0, mirrored));
        QVERIFY(se::insert(both, 0, 0, mirrored));
        QVERIFY(se::insert(both, 1, 0, top));
        top.masks.clear();
        QVERIFY(se::insert(above, 1, 0, top));
        const QImage b = at(below, 0), a = at(above, 0), ab = at(both, 0);
        QCOMPARE(px(ab, 200, 180), px(b, 200, 180)); // inside the hole: V1
        QVERIFY(px(ab, 200, 180) != px(a, 200, 180));
        QCOMPARE(px(ab, 40, 40), px(a, 40, 40)); // outside: V2
        QVERIFY(px(ab, 40, 40) != px(b, 40, 40));
        QCOMPARE(counterOf(ab), 150); // the foot is V2's
    }
    void keyedOpacity() {
        AVLOOP_NEED_SAMPLE(kCounter);
        Sequence s = tracks(2);
        Clip c = counter();
        c.keys = {{0, 320, 180, 1, 0, false}, {2, 320, 180, 1, 100, false}};
        QVERIFY(se::insert(s, 1, 0, c));
        QVERIFY(std::abs(qAlpha(px(at(s, 1), 40, 40)) - 128) <= 1); // half way: 50 %
        QCOMPARE(qAlpha(px(at(s, 0), 40, 40)), 0);
        QCOMPARE(qAlpha(px(at(s, 3), 40, 40)), 255);
    }
    void matteHole() {
        AVLOOP_NEED_SAMPLE(kCounter);
        Sequence s = tracks(1);
        QVERIFY(se::insert(s, 0, 0, counter()));
        s.matte.kind = CropShape::Kind::Ellipse;
        s.matte.rect = QRectF(220, 130, 200, 100);
        s.matte.invert = true;
        const QImage img = at(s, 0);
        QCOMPARE(qAlpha(px(img, 320, 180)), 0);
        QCOMPARE(qAlpha(px(img, 40, 40)), 255);
    }
    void decodersBounded() {
        AVLOOP_NEED_SAMPLE(kCounter);
        Sequence s = tracks(Sequence::kMaxTracks);
        for (int i = 0; i < Sequence::kMaxTracks; ++i)
            QVERIFY(se::insert(s, i, 0, counter(0.3 + 0.1 * i))); // each a working size of its own
        render::SequenceRenderer r(s, QRectF(0, 0, 640, 360), QSize(640, 360), LoopSettings::Method::Nearest, true,
                                   Qt::black);
        QImage out;
        QVERIFY(r.render({{1, 0.5, 30, 0}, {4, 0.5, 120, 1}}, out)); // 16 asked for: two lanes
        QCOMPARE(r.decodersOpen(), render::SequenceRenderer::kMaxDecoders);
        QVERIFY(qAlpha(px(out, 320, 180)) == 255);
    }
    void layerTakesTheComposite() {
        AVLOOP_NEED_SAMPLE(kCounter);
        ProjectData p;
        p.source.path = samplePath(kCounter);
        p.range = {0, 2};
        p.transform = VideoTransform::initial({640, 360});
        VideoLayer l;
        l.path = p.source.path;
        l.size = QSizeF(640, 360);
        l.duration = 10;
        l.range = {0, 10};
        l.track = 2;
        l.transform = VideoTransform::initial({640, 360});
        l.transform.scale = 0.5;
        p.layers = {l};
        const auto probed = media::MediaProbe::probe(p.source.path);
        QVERIFY(probed.info);
        const render::RenderJob job = render::RenderJob::of(p, *probed.info);
        QVERIFY(job.composite);
        render::FrameRenderer r(job, nullptr, false);
        QVERIFY(r.open());
        QImage out;
        QVERIFY(r.render(0, out));
        QCOMPARE(out.size(), job.outputSize);
        QCOMPARE(r.decodersOpen(), 2); // the main video and the layer, at their own sizes
    }
};

QTEST_MAIN(TestComposite)
#include "test-composite.moc"
