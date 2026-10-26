#include "core/TimeMap.h"

#include <algorithm>
#include <cmath>

namespace avloop {

FrameTimes FrameTimes::constant(Rational fps, std::int64_t count, double start) {
    FrameTimes f;
    f.fps_ = fps.isValid() ? fps : Rational{30, 1};
    f.count_ = std::max<std::int64_t>(0, count);
    f.start_ = start;
    return f;
}

FrameTimes FrameTimes::variable(QVector<double> sortedTimes) {
    FrameTimes f;
    f.times_ = std::move(sortedTimes);
    f.count_ = f.times_.size();
    return f;
}

std::int64_t FrameTimes::count() const {
    return count_;
}

double FrameTimes::time(std::int64_t n) const {
    n = std::clamp<std::int64_t>(n, 0, std::max<std::int64_t>(0, count_ - 1));
    if (!times_.isEmpty()) return times_[qsizetype(n)];
    return start_ + double(n) * fps_.den / fps_.num;
}

std::int64_t FrameTimes::at(double t) const {
    if (count_ == 0) return 0;
    if (!times_.isEmpty()) {
        const auto it = std::upper_bound(times_.begin(), times_.end(), t + 1e-9);
        return std::max<std::int64_t>(0, std::int64_t(it - times_.begin()) - 1);
    }
    const auto n = std::int64_t(std::floor((t - start_) * fps_.num / fps_.den + 1e-9));
    return std::clamp<std::int64_t>(n, 0, count_ - 1);
}

namespace timemap {

double tau(double tIn, int i, double speed, Rational fps) {
    const double r = fps.isValid() ? fps.value() : 30.0;
    return tIn + i * (speed > 0 ? speed : 1.0) / r;
}

SourceMix sources(LoopSettings::Method method, double tau, const FrameTimes& frames) {
    SourceMix mix;
    if (method == LoopSettings::Method::Motion) {
        mix.motion = true;
        mix.seconds[0] = tau;
        return mix;
    }
    const std::int64_t n = frames.at(tau);
    const std::int64_t m = std::min(n + 1, frames.count() - 1);
    const double tn = frames.time(n);
    const double tm = frames.time(m);
    if (method == LoopSettings::Method::Nearest || m == n || tau <= tn) {
        // The frame whose time is nearest τ; a tie goes to the earlier one.
        const bool later = m != n && (tm - tau) < (tau - tn);
        mix.seconds[0] = later ? tm : tn;
        return mix;
    }
    // Frame Blend: the frames either side of τ, weighted by time.
    const double w = std::clamp((tau - tn) / (tm - tn), 0.0, 1.0);
    if (w < 1e-9) {
        mix.seconds[0] = tn;
        return mix;
    }
    mix.count = 2;
    mix.seconds[0] = tn;
    mix.seconds[1] = tm;
    mix.weight[0] = 1 - w;
    mix.weight[1] = w;
    return mix;
}

} // namespace timemap

} // namespace avloop
