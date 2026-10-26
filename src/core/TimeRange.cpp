#include "core/TimeRange.h"

#include "core/Timecode.h"

#include <algorithm>
#include <cstdint>

namespace avloop::timerange {

namespace {

using Frames = std::int64_t;
constexpr Frames kMin = 2;

// A range in grid frames, with the file's last frame end.
struct Grid {
    Frames in, out, end;
};

Grid toGrid(TimeRange r, double duration, Rational fps) {
    return {timecode::nearestFrame(r.in, fps), timecode::nearestFrame(r.out, fps),
            std::max<Frames>(0, timecode::frameOf(duration, fps))};
}

TimeRange toTime(Frames in, Frames out, Rational fps) {
    return {timecode::secondsOf(in, fps), timecode::secondsOf(out, fps)};
}

// Within the file and two frames long; a file under two frames is its whole self.
TimeRange settle(Frames in, Frames out, Frames end, Rational fps) {
    if (end < kMin) return toTime(0, end, fps);
    in = std::clamp<Frames>(in, 0, end - kMin);
    out = std::clamp<Frames>(out, in + kMin, end);
    return toTime(in, out, fps);
}

} // namespace

double fileEnd(double duration, Rational fps) {
    return timecode::secondsOf(std::max<Frames>(0, timecode::frameOf(duration, fps)), fps);
}

double minLength(Rational fps) {
    return timecode::secondsOf(kMin, fps);
}

TimeRange whole(double duration, Rational fps) {
    return {0, fileEnd(duration, fps)};
}

TimeRange valid(TimeRange r, double duration, Rational fps) {
    Grid g = toGrid(r, duration, fps);
    if (r.out <= 0) g.out = g.end;
    return settle(g.in, g.out, g.end, fps);
}

TimeRange withIn(TimeRange r, double t, double duration, Rational fps) {
    Grid g = toGrid(r, duration, fps);
    const Frames in = std::clamp<Frames>(timecode::nearestFrame(t, fps), 0, std::max<Frames>(0, g.end - kMin));
    if (in + kMin > g.out) g.out = g.end;
    return settle(in, g.out, g.end, fps);
}

TimeRange withOut(TimeRange r, double t, double duration, Rational fps) {
    Grid g = toGrid(r, duration, fps);
    const Frames out = std::clamp<Frames>(timecode::nearestFrame(t, fps), std::min(kMin, g.end), g.end);
    if (out - kMin < g.in) g.in = 0;
    return settle(g.in, out, g.end, fps);
}

TimeRange clearIn(TimeRange r, double duration, Rational fps) {
    const Grid g = toGrid(r, duration, fps);
    return settle(0, g.out, g.end, fps);
}

TimeRange clearOut(TimeRange r, double duration, Rational fps) {
    const Grid g = toGrid(r, duration, fps);
    return settle(g.in, g.end, g.end, fps);
}

TimeRange dragIn(TimeRange r, double t, double duration, Rational fps) {
    const Grid g = toGrid(r, duration, fps);
    const Frames in = std::clamp<Frames>(timecode::nearestFrame(t, fps), 0, std::max<Frames>(0, g.out - kMin));
    return settle(in, g.out, g.end, fps);
}

TimeRange dragOut(TimeRange r, double t, double duration, Rational fps) {
    const Grid g = toGrid(r, duration, fps);
    const Frames out = std::clamp<Frames>(timecode::nearestFrame(t, fps), g.in + kMin, std::max(g.end, g.in + kMin));
    return settle(g.in, out, g.end, fps);
}

TimeRange moved(TimeRange r, double delta, double duration, Rational fps) {
    const Grid g = toGrid(r, duration, fps);
    const Frames length = g.out - g.in;
    const Frames in =
        std::clamp<Frames>(g.in + timecode::nearestFrame(delta, fps), 0, std::max<Frames>(0, g.end - length));
    return settle(in, in + length, g.end, fps);
}

} // namespace avloop::timerange
