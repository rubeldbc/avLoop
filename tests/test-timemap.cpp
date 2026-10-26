// Segment frame → source frames (spec §8.3, §8.4): Nearest (a tie to the earlier), Frame Blend (weights by time),
// Motion (the pre-pass's frame at τ); a variable rate by its PTS; 23.976 → 30; Speed 0.5 and 2; the output rate.
#include "core/TimeMap.h"

#include <QTest>

#include <cmath>

using namespace avloop;
using Method = LoopSettings::Method;

class TestTimeMap : public QObject {
    Q_OBJECT
private slots:
    void tauAndSpeed() {
        QCOMPARE(timemap::tau(2.0, 0, 1.0, {30, 1}), 2.0);
        QCOMPARE(timemap::tau(2.0, 30, 1.0, {30, 1}), 3.0);
        QCOMPARE(timemap::tau(2.0, 30, 0.5, {30, 1}), 2.5); // slow motion: half a second of source a second
        QCOMPARE(timemap::tau(2.0, 30, 2.0, {30, 1}), 4.0);
    }
    void nearestCfr() {
        const FrameTimes f = FrameTimes::constant({30, 1}, 300);
        QCOMPARE(f.at(1.0), qint64(30));
        QCOMPARE(f.at(1.0 - 1e-4), qint64(29));
        const SourceMix exact = timemap::sources(Method::Nearest, 1.0, f);
        QCOMPARE(exact.count, 1);
        QCOMPARE(exact.seconds[0], 1.0);
        // 0.4 of the way to the next frame: this one; 0.6: the next; halfway: the earlier.
        QCOMPARE(timemap::sources(Method::Nearest, 1.0 + 0.4 / 30, f).seconds[0], 1.0);
        QCOMPARE(timemap::sources(Method::Nearest, 1.0 + 0.6 / 30, f).seconds[0], 31.0 / 30);
        QCOMPARE(timemap::sources(Method::Nearest, 1.0 + 0.5 / 30, f).seconds[0], 1.0);
        // Past the end: the last frame.
        QCOMPARE(timemap::sources(Method::Nearest, 99, f).seconds[0], 299.0 / 30);
    }
    void film24To30() {
        // 23.976 → 30: Nearest repeats a source frame about every fifth output frame.
        const Rational film{24000, 1001};
        const FrameTimes f = FrameTimes::constant(film, 1000);
        int repeats = 0;
        double previous = -1;
        for (int i = 0; i < 300; ++i) {
            const double t = timemap::sources(Method::Nearest, timemap::tau(0, i, 1.0, {30, 1}), f).seconds[0];
            if (t == previous) ++repeats;
            QVERIFY(t >= previous);
            previous = t;
        }
        QVERIFY2(repeats >= 55 && repeats <= 65, qPrintable(QString::number(repeats))); // 300 × (1 − 23.976/30) ≈ 60
    }
    void frameBlend() {
        const FrameTimes f = FrameTimes::constant({30, 1}, 300);
        const SourceMix m = timemap::sources(Method::FrameBlend, 1.0 + 0.25 / 30, f);
        QCOMPARE(m.count, 2);
        QCOMPARE(m.seconds[0], 1.0);
        QCOMPARE(m.seconds[1], 31.0 / 30);
        QVERIFY(std::abs(m.weight[0] - 0.75) < 1e-9);
        QVERIFY(std::abs(m.weight[1] - 0.25) < 1e-9);
        const SourceMix on = timemap::sources(Method::FrameBlend, 1.0, f); // on a frame: that frame alone
        QCOMPARE(on.count, 1);
        // Speed 0.5 at the source's own rate: every other output frame falls halfway — an even blend.
        const SourceMix half = timemap::sources(Method::FrameBlend, timemap::tau(1.0, 1, 0.5, {30, 1}), f);
        QCOMPARE(half.count, 2);
        QVERIFY(std::abs(half.weight[1] - 0.5) < 1e-9);
    }
    void variableRateByPts() {
        // A variable rate: frames at uneven times; mapping goes by time, never by index.
        const FrameTimes f = FrameTimes::variable({0.0, 0.030, 0.070, 0.100, 0.150, 0.170, 0.200});
        QCOMPARE(f.count(), qint64(7));
        QCOMPARE(f.at(0.1), qint64(3));
        QCOMPARE(f.at(0.16), qint64(4));
        QCOMPARE(timemap::sources(Method::Nearest, 0.162, f).seconds[0], 0.17); // 0.17 is nearer than 0.15
        QCOMPARE(timemap::sources(Method::Nearest, 0.158, f).seconds[0], 0.15);
        QCOMPARE(timemap::sources(Method::Nearest, 0.05, f).seconds[0], 0.03); // a tie → the earlier
        const SourceMix m = timemap::sources(Method::FrameBlend, 0.16, f);
        QCOMPARE(m.seconds[0], 0.15);
        QCOMPARE(m.seconds[1], 0.17);
        QVERIFY(std::abs(m.weight[1] - 0.5) < 1e-9);
    }
    void motion() {
        const FrameTimes f = FrameTimes::constant({30, 1}, 300);
        const SourceMix m = timemap::sources(Method::Motion, 1.234, f);
        QVERIFY(m.motion);
        QCOMPARE(m.seconds[0], 1.234);
    }
    void outputRate() {
        OutputSettings o; // Source
        QCOMPARE(o.effectiveFps({30000, 1001}, false), (Rational{30000, 1001}));
        QCOMPARE(o.effectiveFps({120, 1}, false), (Rational{60, 1}));         // at most 60
        QCOMPARE(o.effectiveFps({2997, 100}, true), (Rational{30000, 1001})); // variable: the nearest common
        QCOMPARE(o.fpsId(), QStringLiteral("source"));
        o = OutputSettings::fromFpsId(QStringLiteral("24000/1001"));
        QVERIFY(!o.sourceFps);
        QCOMPARE(o.effectiveFps({30, 1}, false), (Rational{24000, 1001}));
        QCOMPARE(OutputSettings::customRate(47.9521), (Rational{47952, 1000}).reduced());
        QCOMPARE(OutputSettings::customRate(99), (Rational{60, 1}));
        QCOMPARE(OutputSettings::fromFpsId(QStringLiteral("120/1")).fps, (Rational{60, 1}));
    }
};

QTEST_GUILESS_MAIN(TestTimeMap)
#include "test-timemap.moc"
