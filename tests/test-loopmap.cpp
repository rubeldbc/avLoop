// The loop's maths (spec §8.1, §8.2): D and L for every mode, F and T clamped at their bounds; Crossfade starts at
// S[F] and its last frame is mostly S[F − 1]; the Ping-pongs show each turn frame once; the smooth turn's halves are
// monotone, its ends ease and its middle runs at a constant speed.
#include "core/LoopMap.h"

#include <QTest>

#include <cmath>

using namespace avloop;
namespace lm = avloop::loopmap;
using Mode = LoopSettings::Mode;
using Curve = LoopSettings::Curve;

class TestLoopMap : public QObject {
    Q_OBJECT
private slots:
    void segmentAndLength() {
        constexpr Rational k30{30, 1};
        QCOMPARE(lm::segmentFrames(1.0, 6.0, k30, 1.0, Mode::Simple), 150);
        QCOMPARE(lm::segmentFrames(1.0, 6.0, k30, 2.0, Mode::Simple), 75);  // faster: fewer frames
        QCOMPARE(lm::segmentFrames(1.0, 6.0, k30, 0.5, Mode::Simple), 300); // slower: more
        QCOMPARE(lm::segmentFrames(1.0, 1.01, k30, 1.0, Mode::Simple), 2);  // the minimum
        QCOMPARE(lm::segmentFrames(1.0, 1.01, k30, 1.0, Mode::PingPong), 3);
        for (int d = 2; d <= 400; ++d) {
            QCOMPARE(lm::length(Mode::Simple, d, 0, 0), d);
            for (int f : {0, 1, d / 2, d, 10'000}) {
                const int clamped = lm::clampFade(f, d);
                QVERIFY(clamped >= 1 && clamped <= std::max(1, d / 2));
                QCOMPARE(lm::length(Mode::Crossfade, d, f, 0), d - clamped);
            }
            if (d >= 3) {
                QCOMPARE(lm::length(Mode::PingPong, d, 0, 0), 2 * d - 2);
                QCOMPARE(lm::length(Mode::PingPongSmooth, d, 0, 5), 2 * d - 2);
                for (int t : {0, 1, (d - 1) / 2, d}) {
                    const int c = lm::clampTurn(t, d);
                    QVERIFY(c >= 1 && c <= std::max(1, (d - 1) / 2));
                }
            }
        }
    }
    void crossfade() {
        for (int d : {4, 10, 149, 400})
            for (int f : {1, 2, d / 4, d / 2})
                for (Curve c : {Curve::Linear, Curve::Smooth}) {
                    const int l = lm::length(Mode::Crossfade, d, f, 0);
                    // O[0] = S[F]: the loop starts after the part blended in (blended itself when D = 2F).
                    const Pick first = lm::pick(Mode::Crossfade, d, f, 0, c, 0);
                    QCOMPARE(first.a, f);
                    if (d > 2 * f) QCOMPARE(first, (Pick{f, f, 0}));
                    // The last frame is mostly S[F − 1], which S[F] follows (half and half when F is 1).
                    const Pick last = lm::pick(Mode::Crossfade, d, f, 0, c, l - 1);
                    QCOMPARE(last.b, f - 1);
                    QVERIFY2(last.w >= 0.5, qPrintable(QStringLiteral("d %1 f %2 w %3").arg(d).arg(f).arg(last.w)));
                    // The weights rise through the fade, the plain part has none.
                    double previous = 0;
                    for (int k = 0; k < l; ++k) {
                        const Pick p = lm::pick(Mode::Crossfade, d, f, 0, c, k);
                        QCOMPARE(p.a, f + k);
                        if (k < d - 2 * f)
                            QCOMPARE(p.w, 0.0);
                        else {
                            QVERIFY(p.w > previous);
                            previous = p.w;
                        }
                    }
                }
        QCOMPARE(lm::curve(Curve::Smooth, 0.5), 0.5);
        QCOMPARE(lm::curve(Curve::Smooth, 0.25), 0.15625);
        QCOMPARE(lm::curve(Curve::Linear, 0.25), 0.25);
    }
    void pingPongTurnsOnce() {
        for (int d : {3, 4, 30, 149}) {
            const int l = lm::length(Mode::PingPong, d, 0, 0);
            QList<int> seq;
            for (int k = 0; k < l; ++k)
                seq << lm::pick(Mode::PingPong, d, 0, 0, Curve::Smooth, k).a;
            QCOMPARE(seq.first(), 0);
            QCOMPARE(seq[d - 1], d - 1);
            QCOMPARE(seq.last(), 1); // then 0 again as the loop restarts
            for (int k = 1; k < l; ++k)
                QVERIFY(seq[k] != seq[k - 1]);
            QVERIFY(seq.last() != seq.first());
        }
    }
    void smoothTurn() {
        const int d = 61;
        const int t = 10;
        const double a = double(t) / (d - 1);
        // The ends: rest at both turns, the far end exactly reached.
        QCOMPARE(lm::smoothPosition(d, t, 0), 0.0);
        QVERIFY(std::abs(lm::smoothPosition(d, t, d - 1) - (d - 1)) < 1e-9);
        // Monotone up, then down, mirror images.
        for (int k = 1; k < d; ++k)
            QVERIFY(lm::smoothPosition(d, t, k) > lm::smoothPosition(d, t, k - 1));
        for (int k = d; k < 2 * d - 2; ++k) {
            QVERIFY(lm::smoothPosition(d, t, k) < lm::smoothPosition(d, t, k - 1));
            QVERIFY(std::abs(lm::smoothPosition(d, t, k) - lm::smoothPosition(d, t, 2 * d - 2 - k)) < 1e-9);
        }
        // At x = a the position is (D − 1)·(a/2)/(1 − a); the middle runs at a constant speed of 1/(1 − a).
        QVERIFY(std::abs(lm::smoothPosition(d, t, t) - (d - 1) * (a / 2) / (1 - a)) < 1e-9);
        const double v1 = lm::smoothPosition(d, t, 30) - lm::smoothPosition(d, t, 29);
        const double v2 = lm::smoothPosition(d, t, 40) - lm::smoothPosition(d, t, 39);
        QVERIFY(std::abs(v1 - 1 / (1 - a)) < 1e-9);
        QVERIFY(std::abs(v1 - v2) < 1e-9);
        // Slow at the ends.
        QVERIFY(lm::smoothPosition(d, t, 1) < 0.2);
        // A fractional position blends its neighbours.
        const Pick p = lm::pick(Mode::PingPongSmooth, d, 0, t, Curve::Smooth, 3);
        QCOMPARE(p.b, p.a + 1);
        QVERIFY(p.w > 0 && p.w < 1);
    }
    // None (D-020): the clip as cut — D frames, each the segment's own, no blend; the same picks as Simple.
    void none() {
        for (int d : {2, 3, 10, 150}) {
            QCOMPARE(lm::length(Mode::None, d, 15, 9), d);
            for (int k = 0; k < d; ++k) {
                const Pick p = lm::pick(Mode::None, d, 15, 9, Curve::Smooth, k);
                QCOMPARE(p, (Pick{k, k, 0}));
                QCOMPARE(p, lm::pick(Mode::Simple, d, 15, 9, Curve::Smooth, k));
            }
        }
        QCOMPARE(lm::segmentFrames(1.0, 1.01, {30, 1}, 1.0, Mode::None), 2);
        LoopSettings l;
        l.mode = Mode::None;
        l.fade = 10;
        l.turn = 10;
        const LoopPlan p = LoopPlan::of(l, 1.0, 2.0, {30, 1});
        QCOMPARE(p.length, 30);
        QVERIFY(!p.fadeClamped && !p.turnClamped);
        QCOMPARE(LoopSettings::modeId(Mode::None), QStringLiteral("none"));
        QCOMPARE(LoopSettings::modeFromId(QStringLiteral("none")), Mode::None);
    }
    void plan() {
        LoopSettings l;
        l.mode = Mode::Crossfade;
        l.fade = 0.5;
        const LoopPlan p = LoopPlan::of(l, 1.0, 6.0, {30, 1});
        QCOMPARE(p.segment, 150);
        QCOMPARE(p.fade, 15);
        QCOMPARE(p.length, 135);
        QVERIFY(!p.fadeClamped);
        l.fade = 10; // more than half the segment
        const LoopPlan q = LoopPlan::of(l, 1.0, 2.0, {30, 1});
        QCOMPARE(q.fade, 15);
        QVERIFY(q.fadeClamped);
        LoopSettings speed = l.clamped();
        speed.speed = 0.33;
        QCOMPARE(speed.clamped().speed, 0.35);
        speed.speed = 9;
        QCOMPARE(speed.clamped().speed, LoopSettings::kMaxSpeed);
    }
};

QTEST_GUILESS_MAIN(TestLoopMap)
#include "test-loopmap.moc"
