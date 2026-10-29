// Scene geometry (spec §3.3, §6.2): the output size rule, the effective scale, the video rectangle, Fit, Fill,
// Center, Reset and the scale's limits.
#include "core/Geometry.h"
#include "core/LoopSettings.h"
#include "core/VideoTransform.h"

#include <QTest>

using namespace avloop;

class TestGeometry : public QObject {
    Q_OBJECT
private slots:
    void outputSizeRule_data() {
        QTest::addColumn<QSizeF>("frame");
        QTest::addColumn<QSize>("out");
        QTest::newRow("as is") << QSizeF(640, 360) << QSize(640, 360);
        QTest::newRow("4K to 1080p") << QSizeF(3840, 2160) << QSize(1920, 1080);
        QTest::newRow("portrait cap") << QSizeF(2160, 3840) << QSize(1080, 1920);
        QTest::newRow("to even") << QSizeF(1000.6, 333.3) << QSize(1000, 334);
        QTest::newRow("odd over the cap") << QSizeF(2001, 1001) << QSize(1920, 960);
        QTest::newRow("minimum 16") << QSizeF(5, 5) << QSize(16, 16);
        QTest::newRow("thin") << QSizeF(1920, 10) << QSize(1920, 16);
    }
    void outputSizeRule() {
        QFETCH(QSizeF, frame);
        QFETCH(QSize, out);
        QCOMPARE(geometry::outputSize(frame), out);
    }
    // The Output section's Size (spec §10.2): Crop size, a long side, Custom W×H — all through the same rule.
    void outputSettingsSize_data() {
        QTest::addColumn<QString>("mode");
        QTest::addColumn<QSize>("custom");
        QTest::addColumn<QSizeF>("frame");
        QTest::addColumn<QSize>("out");
        QTest::addColumn<bool>("limited");
        QTest::newRow("crop") << "crop" << QSize() << QSizeF(641, 361) << QSize(642, 362) << false;
        QTest::newRow("crop over 1920") << "crop" << QSize() << QSizeF(2400, 1200) << QSize(1920, 960) << true;
        QTest::newRow("long side 1080") << "long-1080" << QSize() << QSizeF(640, 360) << QSize(1080, 608) << false;
        QTest::newRow("long side 256 portrait")
            << "long-256" << QSize() << QSizeF(360, 640) << QSize(144, 256) << false;
        QTest::newRow("custom") << "custom" << QSize(500, 300) << QSizeF(640, 360) << QSize(500, 300) << false;
        QTest::newRow("custom odd") << "custom" << QSize(501, 301) << QSizeF(640, 360) << QSize(502, 302) << false;
        QTest::newRow("custom over 1920")
            << "custom" << QSize(4000, 1000) << QSizeF(640, 360) << QSize(1920, 480) << true;
        QTest::newRow("unknown long side") << "long-999" << QSize() << QSizeF(640, 360) << QSize(640, 360) << false;
    }
    void outputSettingsSize() {
        QFETCH(QString, mode);
        QFETCH(QSize, custom);
        QFETCH(QSizeF, frame);
        QFETCH(QSize, out);
        QFETCH(bool, limited);
        OutputSettings o;
        o.setSizeId(mode);
        if (custom.isValid()) o.custom = custom;
        bool wasLimited = false;
        QCOMPARE(o.sizeFor(frame, &wasLimited), out);
        QCOMPARE(wasLimited, limited);
        if (mode != QLatin1String("long-999")) QCOMPARE(o.sizeId(), mode);
    }
    void effectiveScale() {
        VideoTransform t = VideoTransform::initial({640, 360});
        QCOMPARE(geometry::effectiveScale(t, t.rect({640, 360})), 1.0);
        t.scale = 2.0; // a 1080p video at 200%: the output is capped at 1920 — no upscale
        QCOMPARE(geometry::effectiveScale(t, t.rect({1920, 1080})), 1.0);
        t.scale = 1.5; // a 320 px crop of a video at 150%: upscaled
        QCOMPARE(geometry::effectiveScale(t, QRectF(0, 0, 320, 320)), 1.5);
    }
    void videoRectAndInitial() {
        const VideoTransform t = VideoTransform::initial({1920, 1080});
        QCOMPARE(t.rect({1920, 1080}), QRectF(0, 0, 1920, 1080)); // the top-left at (0,0), 100%
        QVERIFY(!t.flipH && !t.flipV);
        VideoTransform s = t;
        s.scale = 0.5;
        QCOMPARE(s.rect({1920, 1080}), QRectF(480, 270, 960, 540)); // about the centre
    }
    void fitFillCenter() {
        const QSizeF video(1920, 1080);
        const QRectF crop(100, 100, 400, 400);
        VideoTransform t = VideoTransform::initial(video);
        t.flipH = true;
        const VideoTransform fit = t.fittedTo(crop, video);
        QCOMPARE(fit.rect(video).width(), 400.0); // the whole video inside
        QVERIFY(fit.rect(video).height() <= 400.0);
        QCOMPARE(fit.centre(), crop.center());
        QVERIFY(fit.flipH); // flips are kept
        const VideoTransform fill = t.filling(crop, video);
        QCOMPARE(fill.rect(video).height(), 400.0); // the crop completely covered
        QVERIFY(fill.rect(video).width() >= 400.0);
        const VideoTransform centred = t.centredOn(crop);
        QCOMPARE(centred.scale, t.scale);
        QCOMPARE(centred.centre(), crop.center());
    }
    // Inverted (D-021): the export's frame is the video's rectangle and it needs alpha; the hole's coverage is Partly.
    void inverse() {
        const VideoTransform t = VideoTransform::initial({640, 360});
        const QSizeF video(640, 360);
        CropShape c;
        c.kind = CropShape::Kind::Rect;
        c.rect = QRectF(100, 100, 200, 100);
        QCOMPARE(geometry::exportFrame(c, t, video), c.rect);
        QVERIFY(!geometry::needsAlpha(c, t, video, false));
        c.invert = true;
        QCOMPARE(geometry::exportFrame(c, t, video), t.rect(video));
        QVERIFY(geometry::needsAlpha(c, t, video, false));
        QCOMPARE(geometry::coverage(c, t, video), geometry::Coverage::Partly);
        c.rect = QRectF();
        QVERIFY(!geometry::needsAlpha(c, t, video, false)); // no crop: nothing to cut
    }
    void scaleLimits() {
        VideoTransform t;
        t.scale = 50;
        QCOMPARE(t.clamped().scale, VideoTransform::kMaxScale);
        t.scale = 0.001;
        QCOMPARE(t.clamped().scale, VideoTransform::kMinScale);
    }
    void corners() {
        const QRectF r(0, 0, 10, 20);
        QCOMPARE(geometry::corner(r, geometry::Corner::BottomRight), QPointF(10, 20));
        QCOMPARE(geometry::opposite(geometry::Corner::TopRight), geometry::Corner::BottomLeft);
    }
};

QTEST_GUILESS_MAIN(TestGeometry)
#include "test-geometry.moc"
