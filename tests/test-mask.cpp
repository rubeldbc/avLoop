// The crop's mask (spec §6.1, S5): the Ellipse's corners 0 and its centre 255, a rectangle with no feather opaque, a
// one-pixel anti-aliased edge at feather 0, a feather that steps down gradually over its width, the Rounded rect's
// radius, the cache. With AVLOOP_TEST_OUT set, the masks are written there as PNGs.
#include "core/CropShape.h"
#include "render/MaskBuilder.h"

#include <QDir>
#include <QTest>

#include <cmath>

using namespace avloop;
using render::MaskBuilder;

namespace {

int at(const QImage& m, int x, int y) {
    return qGray(m.pixel(x, y));
}

void keep(const QImage& m, const QString& name) {
    const QString dir = qEnvironmentVariable("AVLOOP_TEST_OUT");
    if (!dir.isEmpty()) m.save(QDir(dir).filePath(name), "PNG");
}

} // namespace

class TestMask : public QObject {
    Q_OBJECT
private slots:
    void ellipseCornersAndCentre() {
        const QImage m = MaskBuilder::build(CropShape::Kind::Ellipse, {640, 640}, 0, 0);
        keep(m, QStringLiteral("mask-ellipse.png"));
        QCOMPARE(m.format(), QImage::Format_Grayscale8);
        for (QPoint p : {QPoint(0, 0), QPoint(639, 0), QPoint(0, 639), QPoint(639, 639), QPoint(60, 60)})
            QCOMPARE(at(m, p.x(), p.y()), 0);
        QCOMPARE(at(m, 320, 320), 255);
        QCOMPARE(at(m, 320, 1), 255); // just inside the top
    }
    void rectWithoutFeatherIsOpaque() {
        const QImage m = MaskBuilder::build(CropShape::Kind::Rect, {200, 100}, 0, 0);
        for (int y = 0; y < 100; ++y)
            for (int x = 0; x < 200; ++x)
                QCOMPARE(at(m, x, y), 255);
    }
    void featherZeroHasAOnePixelEdge() {
        // A circle: every partly covered pixel lies within half a pixel of the edge, so the ramp is one pixel wide.
        const int side = 301;
        const double radius = side / 2.0;
        const QImage m = MaskBuilder::build(CropShape::Kind::Ellipse, {side, side}, 0, 0);
        int partial = 0;
        for (int y = 0; y < side; ++y)
            for (int x = 0; x < side; ++x) {
                const int a = at(m, x, y);
                if (a == 0 || a == 255) continue;
                ++partial;
                const double r = std::hypot(x + 0.5 - radius, y + 0.5 - radius);
                QVERIFY2(std::abs(r - radius) < 0.5 + 1e-9,
                         qPrintable(QStringLiteral("%1,%2: %3").arg(x).arg(y).arg(r)));
            }
        QVERIFY(partial > 100); // anti-aliased, not a hard edge
    }
    void featherStepsDownGradually() {
        const double feather = 20;
        const QImage m = MaskBuilder::build(CropShape::Kind::Ellipse, {400, 400}, 0, feather);
        keep(m, QStringLiteral("mask-ellipse-feather20.png"));
        int previous = -1;
        int steps = 0;
        for (int x = 0; x <= 200; ++x) { // from the left edge to the centre on the middle row
            const int a = at(m, x, 200);
            QVERIFY2(a >= previous, qPrintable(QStringLiteral("x %1: %2 after %3").arg(x).arg(a).arg(previous)));
            if (a > previous && previous >= 0) ++steps;
            previous = a;
        }
        QCOMPARE(at(m, 0, 200), 13);   // the first pixel's centre half a pixel in: 1/20 → 13
        QCOMPARE(at(m, 25, 200), 255); // 20 px in: whole
        QVERIFY(steps >= 18);          // a ramp, not a step
    }
    void roundedRadius() {
        const QImage m = MaskBuilder::build(CropShape::Kind::Rounded, {200, 100}, 30, 0);
        keep(m, QStringLiteral("mask-rounded30.png"));
        QCOMPARE(at(m, 0, 0), 0); // the corner cut away
        QCOMPARE(at(m, 5, 5), 0);
        QCOMPARE(at(m, 30, 30), 255); // the radius' centre
        QCOMPARE(at(m, 100, 0), 255); // the straight edge
        const QImage square = MaskBuilder::build(CropShape::Kind::Rounded, {50, 50}, 0, 0);
        QCOMPARE(at(square, 0, 0), 255); // radius 0 is a rectangle
    }
    void cacheBuildsOnce() {
        MaskBuilder b;
        CropShape c;
        c.kind = CropShape::Kind::Ellipse;
        c.rect = QRectF(0, 0, 320, 320);
        c.feather = 4;
        b.mask(c, c.rect, {320, 320});
        b.mask(c, c.rect, {320, 320});
        QCOMPARE(b.builds(), 1);
        c.feather = 8;
        b.mask(c, c.rect, {320, 320});
        QCOMPARE(b.builds(), 2);
        b.mask(c, c.rect, {640, 640}); // another output size
        QCOMPARE(b.builds(), 3);
        c.invert = true; // another crop
        b.mask(c, QRectF(0, 0, 640, 640), {640, 640});
        QCOMPARE(b.builds(), 4);
        b.mask(c, QRectF(0, 0, 640, 640), {640, 640});
        QCOMPARE(b.builds(), 4);
    }
    // Shapes (D-022): an Add ellipse inside a rectangle crop keeps its inside only; a Cut rectangle over it clears its
    // part; two Add shapes unite; cuts alone cut from the whole frame; no visible shape, the crop's own mask.
    void shapes() {
        CropShape c;
        c.rect = QRectF(0, 0, 400, 200);
        MaskShape e;
        e.kind = MaskShape::Kind::Ellipse;
        e.rect = QRectF(100, 0, 200, 200);
        c.shapes = {e};
        QImage m = MaskBuilder::build(c, c.rect, {400, 200});
        QCOMPARE(at(m, 200, 100), 255); // the ellipse's centre
        QCOMPARE(at(m, 10, 10), 0);     // the crop's corner, outside the ellipse
        QCOMPARE(at(m, 102, 2), 0);     // the ellipse's box corner
        MaskShape cut;
        cut.kind = MaskShape::Kind::Rect;
        cut.role = MaskShape::Role::Cut;
        cut.rect = QRectF(180, 80, 40, 40);
        c.shapes = {e, cut};
        m = MaskBuilder::build(c, c.rect, {400, 200});
        QCOMPARE(at(m, 200, 100), 0);   // cut
        QCOMPARE(at(m, 150, 100), 255); // still selected
        MaskShape left = e;
        left.rect = QRectF(0, 0, 100, 100);
        c.shapes = {e, left};
        m = MaskBuilder::build(c, c.rect, {400, 200});
        QCOMPARE(at(m, 50, 50), 255);
        QCOMPARE(at(m, 200, 100), 255);
        c.shapes = {cut};
        m = MaskBuilder::build(c, c.rect, {400, 200});
        QCOMPARE(at(m, 10, 10), 255); // the whole frame kept …
        QCOMPARE(at(m, 200, 100), 0); // … but the cut
        c.shapes[0].visible = false;
        QCOMPARE(MaskBuilder::build(c, c.rect, {400, 200}),
                 MaskBuilder::build(CropShape::Kind::Rect, {400, 200}, 0, 0));
        // A polygon (a triangle), scaled: its inside, its outside.
        CropShape none;
        none.shapes = {[] {
            MaskShape t;
            t.kind = MaskShape::Kind::Polygon;
            t.points = {PathPoint::corner({0, 0}), PathPoint::corner({100, 0}), PathPoint::corner({0, 100})};
            return t;
        }()};
        m = MaskBuilder::build(none, QRectF(0, 0, 100, 100), {200, 200});
        QCOMPARE(at(m, 20, 20), 255);
        QCOMPARE(at(m, 180, 180), 0);
    }
    // Inverted (D-021): the ellipse a hole in a 640 × 360 frame — its centre 0, the corners 255; not inverted, the
    // same crop's own mask byte for byte as before.
    void inverse() {
        CropShape c;
        c.kind = CropShape::Kind::Ellipse;
        c.rect = QRectF(200, 80, 200, 200);
        const QRectF frame(0, 0, 640, 360);
        QCOMPARE(MaskBuilder::build(c, c.rect, {200, 200}), MaskBuilder::build(c.kind, {200, 200}, 0, 0));
        c.invert = true;
        const QImage m = MaskBuilder::build(c, frame, {640, 360});
        QCOMPARE(m.size(), QSize(640, 360));
        QCOMPARE(at(m, 300, 180), 0); // the hole's centre
        QCOMPARE(at(m, 0, 0), 255);   // the frame's corners kept
        QCOMPARE(at(m, 639, 359), 255);
        QCOMPARE(at(m, 210, 90), 255);  // the crop's own corner: outside the ellipse, kept
        QCOMPARE(at(m, 100, 180), 255); // left of the hole
        // The edge: one pixel of anti-aliasing at feather 0.
        QVERIFY(at(m, 199, 180) > 200 && at(m, 201, 180) < 60);
        // Feather 20: from the edge outward it rises over ~20 px, monotonically.
        c.feather = 20;
        const QImage f = MaskBuilder::build(c, frame, {640, 360});
        QCOMPARE(at(f, 300, 180), 0);
        int last = -1;
        for (int x = 200; x >= 176; --x) {
            QVERIFY(at(f, x, 180) >= last);
            last = at(f, x, 180);
        }
        QVERIFY(at(f, 199, 180) < 30);
        QCOMPARE(at(f, 178, 180), 255);
        // Scaled: a 1280 × 720 output of the same frame puts the hole at twice the place.
        const QImage big = MaskBuilder::build(c, frame, {1280, 720});
        QCOMPARE(at(big, 600, 360), 0);
        QCOMPARE(at(big, 300, 360), 255);
    }
};

QTEST_GUILESS_MAIN(TestMask)
#include "test-mask.moc"
