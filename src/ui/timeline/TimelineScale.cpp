#include "ui/timeline/TimelineScale.h"

#include "core/Timecode.h"

#include <algorithm>
#include <cmath>

namespace avloop::timelinescale {

namespace {

constexpr double kLabelRoom = 12; // px between two labels
constexpr double kMinorPx = 6;    // minor ticks closer than this are left out

double rate(Rational fps) {
    return fps.isValid() ? fps.value() : 30.0;
}

// The candidate major steps, smallest first, with how many minor ticks divide each.
struct Candidate {
    double seconds;
    int divisions;
};

QList<Candidate> candidates(Rational fps, bool frameLabels) {
    const double frame = 1.0 / rate(fps);
    QList<Candidate> list;
    if (frameLabels) {
        // 1, 2, 5, 10, 20, 50 … frames.
        for (double f = 1; f <= 1e7; f *= 10) {
            list.append({f * frame, f >= 10 ? 5 : int(f)});
            list.append({2 * f * frame, 2});
            list.append({5 * f * frame, 5});
        }
        return list;
    }
    for (int f : {1, 2, 5, 10})
        if (f * frame < 1.0) list.append({f * frame, f == 1 ? 1 : (f == 10 ? 5 : f)});
    // Whole seconds and minutes; past 10 minutes only a file shown whole needs them.
    for (auto [s, d] : {std::pair{1, 0},
                        {2, 2},
                        {5, 5},
                        {10, 5},
                        {15, 3},
                        {30, 3},
                        {60, 4},
                        {120, 2},
                        {300, 5},
                        {600, 5},
                        {1200, 2},
                        {1800, 3},
                        {3600, 4},
                        {7200, 2},
                        {18000, 5},
                        {36000, 5}})
        list.append({double(s), d});
    // One second divides into frames when they are far enough apart.
    if (!list.isEmpty()) {
        for (Candidate& c : list)
            if (c.seconds == 1.0) c.divisions = std::max(1, int(std::lround(rate(fps))));
    }
    return list;
}

} // namespace

Steps steps(double pxPerSecond, Rational fps, double labelWidthPx, bool frameLabels) {
    const QList<Candidate> list = candidates(fps, frameLabels);
    const double needed = labelWidthPx + kLabelRoom;
    Candidate pick = list.last();
    for (const Candidate& c : list)
        if (c.seconds * pxPerSecond >= needed) {
            pick = c;
            break;
        }
    Steps s{pick.seconds, pick.seconds};
    // The finest division whose ticks stay apart: all of them, or a divisor of them.
    for (int d = std::max(1, pick.divisions); d > 1; --d) {
        if (pick.divisions % d != 0) continue;
        if (pick.seconds / d * pxPerSecond >= kMinorPx) {
            s.minor = pick.seconds / d;
            break;
        }
    }
    return s;
}

QList<Tick> ticks(double from, double to, const Steps& s) {
    QList<Tick> out;
    if (s.minor <= 0 || to <= from) return out;
    const auto first = std::int64_t(std::floor(std::max(0.0, from) / s.minor));
    const auto last = std::int64_t(std::ceil(to / s.minor));
    const double ratio = s.major / s.minor;
    for (std::int64_t i = first; i <= last && out.size() < 20000; ++i) {
        const double t = double(i) * s.minor;
        const double k = double(i) / ratio;
        out.append({t, std::abs(k - std::round(k)) < 1e-6});
    }
    return out;
}

QString label(double seconds, Rational fps, bool frameLabels) {
    return frameLabels ? QString::number(timecode::nearestFrame(seconds, fps))
                       : timecode::format(timecode::snap(seconds, fps), fps); // a whole second of 29.97 fps is frame 30
}

QString widestLabel(double duration, Rational fps, bool frameLabels) {
    if (!frameLabels) return QStringLiteral("00:00:00.00");
    return QString(QString::number(timecode::nearestFrame(duration, fps)).size(), QLatin1Char('8'));
}

double minPxPerSecond(double duration, double widthPx) {
    return duration > 0 && widthPx > 0 ? widthPx / duration : 1.0;
}

double maxPxPerSecond(Rational fps) {
    return 12.0 * rate(fps);
}

} // namespace avloop::timelinescale
