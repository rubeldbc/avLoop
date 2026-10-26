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
        b.mask(c, {320, 320});
        b.mask(c, {320, 320});
        QCOMPARE(b.builds(), 1);
        c.feather = 8;
        b.mask(c, {320, 320});
        QCOMPARE(b.builds(), 2);
        b.mask(c, {640, 640}); // another output size
        QCOMPARE(b.builds(), 3);
    }
};

QTEST_GUILESS_MAIN(TestMask)
#include "test-mask.moc"
