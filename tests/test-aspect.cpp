// The crop's geometry (spec §6.1, §3.3): the ratios, a ratio change keeping the centre and the area, the crop's limits,
// Alpha Auto's rule, the handles' drawing and resizing with a ratio and from the centre, snapping.
#include "core/AspectRatio.h"
#include "core/CropShape.h"
#include "core/Geometry.h"
#include "ui/tools/CropHandles.h"

#include <QTest>

using namespace avloop;
namespace ch = avloop::crophandles;

class TestAspect : public QObject {
    Q_OBJECT
private slots:
    void theList() {
        QCOMPARE(aspect::presets().size(), 11);
        QCOMPARE(aspect::presets().first(), QStringLiteral("free"));
        QCOMPARE(aspect::presets().last(), QStringLiteral("4:5"));
        QVERIFY(aspect::isCustom(QStringLiteral("21:10")));
        QVERIFY(!aspect::isCustom(QStringLiteral("21:9")));
        QVERIFY(!aspect::isCustom(QStringLiteral("free")));
    }
    void values() {
        const QSizeF video(1920, 1080);
        QCOMPARE(aspect::value(QStringLiteral("free"), video), 0.0);
        QCOMPARE(aspect::value(QStringLiteral("original"), video), 1920.0 / 1080.0);
        QCOMPARE(aspect::value(QStringLiteral("9:16"), video), 9.0 / 16.0);
        QCOMPARE(aspect::value(aspect::custom(2.5, 1), video), 2.5);
        QCOMPARE(aspect::custom(0, 1), QString());
        QCOMPARE(aspect::reduced(180, 320), QStringLiteral("9:16"));
        QCOMPARE(aspect::reduced(1000.4, 300), QStringLiteral("10:3"));
        QCOMPARE(aspect::reduced(0, 300), QString());
    }
    void changeKeepsCentreAndArea() {
        for (const QString& id : aspect::presets()) {
            const double r = aspect::value(id, QSizeF(640, 360));
            const QRectF before(100, 50, 300, 200);
            const QRectF after = aspect::apply(before, r);
            QCOMPARE(after.center(), before.center());
            if (r > 0) {
                QVERIFY(qAbs(after.width() / after.height() - r) < 1e-9);
                QVERIFY(qAbs(after.width() * after.height() - 60000.0) < 1e-6);
            }
        }
    }
    void cropLimits() {
        CropShape c;
        c.kind = CropShape::Kind::Rounded;
        c.rect = QRectF(0, 0, 100, 40);
        c.radius = 50;
        c.feather = 500;
        const CropShape k = c.clamped();
        QCOMPARE(k.radius, 20.0); // half the short side
        QCOMPARE(k.feather, CropShape::kMaxFeather);
        QCOMPARE(CropShape::kindFromId(CropShape::kindId(CropShape::Kind::Ellipse)), CropShape::Kind::Ellipse);
        QCOMPARE(CropShape::toolId(CropShape::Kind::Rounded), QStringLiteral("crop-round"));
    }
    void alphaAuto() {
        const QSizeF video(640, 360);
        const VideoTransform t = VideoTransform::initial(video);
        CropShape c;
        QVERIFY(!geometry::needsAlpha(c, t, video, false)); // no crop
        QVERIFY(geometry::needsAlpha(c, t, video, true));   // a source with alpha
        c.rect = QRectF(10, 10, 100, 100);
        QVERIFY(!geometry::needsAlpha(c, t, video, false)); // a rect inside, no feather
        c.feather = 2;
        QVERIFY(geometry::needsAlpha(c, t, video, false));
        c.feather = 0;
        c.rect = QRectF(600, 10, 100, 100); // past the right edge
        QVERIFY(geometry::needsAlpha(c, t, video, false));
        QCOMPARE(geometry::coverage(c, t, video), geometry::Coverage::Partly);
        c.rect = QRectF(1000, 1000, 50, 50);
        QCOMPARE(geometry::coverage(c, t, video), geometry::Coverage::None);
        c.rect = QRectF(10, 10, 100, 100);
        c.kind = CropShape::Kind::Ellipse;
        QVERIFY(geometry::needsAlpha(c, t, video, false));
    }
    void drawing() {
        QCOMPARE(ch::drawn({10, 10}, {110, 60}, 0, false), QRectF(10, 10, 100, 50));
        QCOMPARE(ch::drawn({10, 10}, {110, 60}, 1.0, false), QRectF(10, 10, 100, 100)); // Shift: 1:1
        QCOMPARE(ch::drawn({100, 100}, {150, 120}, 0, true), QRectF(50, 80, 100, 40));  // Alt: from the centre
        QCOMPARE(ch::drawn({100, 100}, {60, 140}, 0, false), QRectF(60, 100, 40, 40));  // up-left
    }
    void resizing() {
        const QRectF r(100, 100, 200, 100);
        QCOMPARE(ch::resized(r, ch::Handle::Right, {50, 0}, 0, false), QRectF(100, 100, 250, 100));
        QCOMPARE(ch::resized(r, ch::Handle::Right, {50, 0}, 0, true), QRectF(50, 100, 300, 100));
        // A corner with 2:1 kept: the opposite corner stays.
        const QRectF k = ch::resized(r, ch::Handle::BottomRight, {100, 10}, 2.0, false);
        QCOMPARE(k.topLeft(), r.topLeft());
        QCOMPARE(k.width() / k.height(), 2.0);
        QCOMPARE(k.width(), 300.0);
        // An edge with a ratio: the other side follows, centred.
        const QRectF e = ch::resized(r, ch::Handle::Bottom, {0, 50}, 2.0, false);
        QCOMPARE(e.height(), 150.0);
        QCOMPARE(e.width(), 300.0);
        QCOMPARE(e.center().x(), r.center().x());
        QCOMPARE(e.top(), r.top());
    }
    void handles() {
        const QRectF r(0, 0, 100, 50);
        QCOMPARE(ch::hit(r, {101, 49}), ch::Handle::BottomRight);
        QCOMPARE(ch::hit(r, {50, -3}), ch::Handle::Top);
        QCOMPARE(ch::hit(r, {50, 25}), ch::Handle::None);
        QCOMPARE(ch::all().size(), 8);
    }
    void snapping() {
        const QRectF v(0, 0, 640, 360);
        double line = -1;
        QCOMPARE(ch::snap(5, ch::targetsX(v), 8, &line), 0.0);
        QCOMPARE(line, 0.0);
        QCOMPARE(ch::snap(325, ch::targetsX(v), 8), 320.0); // the centre
        QCOMPARE(ch::snap(300, ch::targetsX(v), 8), 300.0); // nothing within reach
    }
};

QTEST_MAIN(TestAspect)
#include "test-aspect.moc"
