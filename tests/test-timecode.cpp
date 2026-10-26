// Time on the output grid (spec §6.4, §7): HH:MM:SS.FF both ways, frames and the grid, the In/Out rules, and the
// ruler's steps — labels never overlap at any zoom, from 12 px a frame to a two-hour file shown whole.
#include "core/TimeRange.h"
#include "core/Timecode.h"
#include "ui/timeline/TimelineScale.h"

#include <QTest>

using namespace avloop;
namespace tc = avloop::timecode;
namespace tr = avloop::timerange;
namespace ts = avloop::timelinescale;

namespace {
constexpr Rational k30{30, 1};
constexpr Rational kNtsc{30000, 1001};
constexpr Rational k60{60, 1};
} // namespace

class TestTimecode : public QObject {
    Q_OBJECT
private slots:
    void format() {
        QCOMPARE(tc::format(0, k30), QStringLiteral("00:00:00.00"));
        QCOMPARE(tc::format(12.4, k30), QStringLiteral("00:00:12.12"));
        QCOMPARE(tc::format(3725.5, k30), QStringLiteral("01:02:05.15"));
        // 29.97: frame 30 starts just after one second — the first frame of second 1.
        QCOMPARE(tc::format(tc::secondsOf(29, kNtsc), kNtsc), QStringLiteral("00:00:00.29"));
        QCOMPARE(tc::format(tc::secondsOf(30, kNtsc), kNtsc), QStringLiteral("00:00:01.00"));
    }
    void parse() {
        double t = -1;
        QVERIFY(tc::parse(QStringLiteral("12.4"), k30, &t));
        QCOMPARE(t, 12.4);
        QVERIFY(tc::parse(QStringLiteral("00:00:12.12"), k30, &t));
        QCOMPARE(t, 12.4);
        QVERIFY(tc::parse(QStringLiteral("1:02.05"), k30, &t));
        QCOMPARE(t, 62.0 + 5.0 / 30);
        QVERIFY(tc::parse(QStringLiteral("372f"), k30, &t));
        QCOMPARE(t, 12.4);
        QVERIFY(tc::parse(QStringLiteral(" 01:02:05.15 "), k30, &t));
        QCOMPARE(t, 3725.5);
        for (const char* bad : {"", "abc", "-1", "1:2:3:4", "12.4.5f", "1:-2"})
            QVERIFY2(!tc::parse(QString::fromLatin1(bad), k30, &t), bad);
        // Every frame's timecode reads back to that frame.
        for (int n = 0; n < 4000; n += 7) {
            QVERIFY(tc::parse(tc::format(tc::secondsOf(n, kNtsc), kNtsc), kNtsc, &t));
            QCOMPARE(tc::nearestFrame(t, kNtsc), qint64(n));
        }
    }
    void grid() {
        QCOMPARE(tc::frameOf(tc::secondsOf(299, k30), k30), qint64(299));
        QCOMPARE(tc::frameOf(0.999, k30), qint64(29));
        QCOMPARE(tc::snap(0.51, k30), 15.0 / 30);
        QCOMPARE(tc::defaultFps({120, 1}), (Rational{60, 1}));
        QCOMPARE(tc::defaultFps(kNtsc), kNtsc);
        QCOMPARE(tc::defaultFps({}), (Rational{30, 1}));
    }
    void rangeRules() {
        const double d = 10.02; // 300 whole frames at 30
        QCOMPARE(tr::fileEnd(d, k30), 10.0);
        QCOMPARE(tr::whole(d, k30), (TimeRange{0, 10}));
        const TimeRange r{2, 5};
        QCOMPARE(tr::withIn(r, 3.01, d, k30), (TimeRange{3, 5}));                 // snapped to the grid
        QCOMPARE(tr::withIn(r, 6, d, k30), (TimeRange{6, 10}));                   // In after Out: Out to the end
        QCOMPARE(tr::withIn(r, 149.0 / 30, d, k30), (TimeRange{149.0 / 30, 10})); // a frame before Out: the same
        QCOMPARE(tr::withOut(r, 4, d, k30), (TimeRange{2, 4}));
        QCOMPARE(tr::withOut(r, 1, d, k30), (TimeRange{0, 1}));   // Out before In: In to the start
        QCOMPARE(tr::withOut(r, 99, d, k30), (TimeRange{2, 10})); // within the file
        QCOMPARE(tr::clearIn(r, d, k30), (TimeRange{0, 5}));
        QCOMPARE(tr::clearOut(r, d, k30), (TimeRange{2, 10}));
        QCOMPARE(tr::dragIn(r, 9, d, k30), (TimeRange{5 - 2.0 / 30, 5})); // a handle stops two frames short
        QCOMPARE(tr::dragOut(r, 0, d, k30), (TimeRange{2, 2 + 2.0 / 30}));
        QCOMPARE(tr::moved(r, 1.5, d, k30), (TimeRange{3.5, 6.5}));
        QCOMPARE(tr::moved(r, 99, d, k30), (TimeRange{7, 10})); // the length kept, within the file
        QCOMPARE(tr::moved(r, -99, d, k30), (TimeRange{0, 3}));
        QCOMPARE(tr::valid({0, 0}, d, k30), (TimeRange{0, 10})); // none read: the whole file
        QCOMPARE(tr::valid({7, 3}, d, k30), (TimeRange{7, 7 + 2.0 / 30}));
        QCOMPARE(tr::whole(0.04, k30), (TimeRange{0, 1.0 / 30})); // a one-frame file is its whole self
        QCOMPARE(tr::minLength(k60), 2.0 / 60);
    }
    void labelsNeverOverlap() {
        // The widest label at 10 px Consolas is about 66 px; 70 is asked for.
        const double label = 70;
        for (const Rational fps : {k30, kNtsc, k60, Rational{24, 1}})
            for (bool frames : {false, true})
                for (double pps = 0.05; pps <= ts::maxPxPerSecond(fps); pps *= 1.07) {
                    const ts::Steps s = ts::steps(pps, fps, label, frames);
                    QVERIFY2(s.major * pps >= label + 1,
                             qPrintable(QStringLiteral("%1 px/s: step %2 s").arg(pps).arg(s.major)));
                    QVERIFY(s.minor <= s.major);
                    QVERIFY(s.minor * pps >= 6 || s.minor == s.major);
                }
        // 12 px a frame: a label every few frames; one second whole across 1000 px: a frame step.
        QVERIFY(ts::steps(ts::maxPxPerSecond(k30), k30, label, false).major <= 10.0 / 30);
        QCOMPARE(ts::maxPxPerSecond(k30), 360.0);
        QCOMPARE(ts::minPxPerSecond(10, 1000), 100.0);
    }
    void ticksAndLabels() {
        const ts::Steps s = ts::steps(100, k30, 70, false); // 1 s labels at 100 px/s
        QCOMPARE(s.major, 1.0);
        const auto list = ts::ticks(0, 3, s);
        QVERIFY(!list.isEmpty());
        int majors = 0;
        for (const ts::Tick& t : list)
            majors += t.major;
        QCOMPARE(majors, 4); // 0, 1, 2, 3
        QCOMPARE(ts::label(1.0, kNtsc, false), QStringLiteral("00:00:01.00"));
        QCOMPARE(ts::label(1.0, k30, true), QStringLiteral("30"));
        QCOMPARE(ts::widestLabel(100, k30, true), QStringLiteral("8888"));
    }
};

QTEST_GUILESS_MAIN(TestTimecode)
#include "test-timecode.moc"
