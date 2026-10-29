#include "core/LoopMap.h"

#include <algorithm>
#include <cmath>

namespace avloop::loopmap {

namespace {

double rate(Rational fps) {
    return fps.isValid() ? fps.value() : 30.0;
}

// The trapezoid velocity's distance (spec §8.2): x² / 2a, then x − a/2, then 1 − a − (1 − x)² / 2a.
double ease(double x, double a) {
    if (x < a) return x * x / (2 * a);
    if (x <= 1 - a) return x - a / 2;
    return 1 - a - (1 - x) * (1 - x) / (2 * a);
}

bool pingPong(Mode m) {
    return m == Mode::PingPong || m == Mode::PingPongSmooth;
}

} // namespace

int segmentFrames(double tIn, double tOut, Rational fps, double speed, Mode mode) {
    const double s = speed > 0 ? speed : 1.0;
    const int d = int(std::lround((tOut - tIn) * rate(fps) / s));
    return std::max(d, pingPong(mode) ? 3 : 2);
}

int clampFade(int fadeFrames, int segment) {
    return std::clamp(fadeFrames, 1, std::max(1, segment / 2));
}

int clampTurn(int turnFrames, int segment) {
    return std::clamp(turnFrames, 1, std::max(1, (segment - 1) / 2));
}

int framesOf(double seconds, Rational fps) {
    return int(std::lround(seconds * rate(fps)));
}

double secondsOf(int frames, Rational fps) {
    return fps.isValid() ? double(frames) * fps.den / fps.num : frames / 30.0;
}

int length(Mode mode, int segment, int fade, int turn) {
    Q_UNUSED(turn);
    switch (mode) {
    case Mode::Simple:
    case Mode::None: // the clip as cut: the segment's frames, nothing more
        return segment;
    case Mode::Crossfade:
        return segment - clampFade(fade, segment);
    case Mode::PingPong:
    case Mode::PingPongSmooth:
        return 2 * segment - 2;
    }
    return segment;
}

double curve(Curve c, double x) {
    return c == Curve::Linear ? x : x * x * (3 - 2 * x);
}

double smoothPosition(int segment, int turn, int k) {
    const int last = segment - 1;
    const double a = double(clampTurn(turn, segment)) / last;
    const double x = k <= last ? double(k) / last : double(2 * segment - 2 - k) / last;
    return last * ease(x, a) / (1 - a);
}

Pick pick(Mode mode, int segment, int fade, int turn, Curve c, int k) {
    const int d = segment;
    k = std::clamp(k, 0, std::max(0, length(mode, segment, fade, turn) - 1));
    switch (mode) {
    case Mode::Simple:
    case Mode::None:
        return {k, k, 0};
    case Mode::Crossfade: {
        const int f = clampFade(fade, d);
        if (k < d - 2 * f) return {f + k, f + k, 0};
        // The end of the segment blends into its first F frames, so S[F] follows the last output frame naturally.
        const double w = curve(c, double(k - (d - 2 * f) + 1) / (f + 1));
        return {f + k, k - (d - 2 * f), w};
    }
    case Mode::PingPong: {
        const int i = k < d ? k : 2 * d - 2 - k; // each turn frame once
        return {i, i, 0};
    }
    case Mode::PingPongSmooth: {
        const double p = std::clamp(smoothPosition(d, turn, k), 0.0, double(d - 1));
        const int a = int(std::floor(p));
        const double w = p - a;
        if (w < 1e-9) return {a, a, 0};
        return {a, std::min(a + 1, d - 1), w};
    }
    }
    return {k, k, 0};
}

} // namespace avloop::loopmap

namespace avloop {

LoopPlan LoopPlan::of(const LoopSettings& loop, double tIn, double tOut, Rational fps) {
    LoopPlan plan;
    plan.segment = loopmap::segmentFrames(tIn, tOut, fps, loop.speed, loop.mode);
    const int fade = loopmap::framesOf(loop.fade, fps);
    const int turn = loopmap::framesOf(loop.turn, fps);
    plan.fade = loopmap::clampFade(fade, plan.segment);
    plan.turn = loopmap::clampTurn(turn, plan.segment);
    plan.fadeClamped = loop.mode == LoopSettings::Mode::Crossfade && plan.fade != fade;
    plan.turnClamped = loop.mode == LoopSettings::Mode::PingPongSmooth && plan.turn != turn;
    plan.length = loopmap::length(loop.mode, plan.segment, plan.fade, plan.turn);
    return plan;
}

} // namespace avloop
