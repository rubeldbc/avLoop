#pragma once

#include "core/LoopSettings.h"

#include <QVector>

#include <cstdint>

namespace avloop {

// A source's frame times in seconds (spec §8.3): a constant rate (start + n / fps) or, for a variable rate, the PTS
// list itself. Mapping is always by time, never by frame index.
class FrameTimes {
public:
    static FrameTimes constant(Rational fps, std::int64_t count, double start = 0);
    static FrameTimes variable(QVector<double> sortedTimes);

    std::int64_t count() const;
    double time(std::int64_t n) const;
    // The frame on screen at t: the last one starting at or before it (the first before the start).
    std::int64_t at(double t) const;

private:
    Rational fps_;
    std::int64_t count_ = 0;
    double start_ = 0;
    QVector<double> times_; // a variable rate
};

// The source frames that make segment frame i (spec §8.3): one, or two with weights; Motion asks the Interpolator's
// frame at τ (its pre-pass runs at the output rate).
struct SourceMix {
    int count = 1;
    double seconds[2] = {0, 0};
    double weight[2] = {1, 0};
    bool motion = false;
};

namespace timemap {

// τ(i) = tIn + i × s / fps.
double tau(double tIn, int i, double speed, Rational fps);
SourceMix sources(LoopSettings::Method method, double tau, const FrameTimes& frames);

} // namespace timemap

} // namespace avloop
