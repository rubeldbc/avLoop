#pragma once

#include "core/Rational.h"

namespace avloop {

// The In–Out range in seconds of source time (spec §6.4, §11.1 "range"): the segment's frames start at In, and Out
// is where the segment ends (§8.1: D = (Out − In) × fps frames). Both lie on the output frame grid.
struct TimeRange {
    double in = 0;
    double out = 0;

    double length() const { return out - in; }
    bool isEmpty() const { return out <= in; }
    friend bool operator==(const TimeRange&, const TimeRange&) = default;
};

// The rules of spec §6.4, worked in whole grid frames. `duration` is the file's; each result is on the grid, within
// the file, and at least two output frames long unless the file itself is shorter.
namespace timerange {

double fileEnd(double duration, Rational fps); // the end of the file's last whole grid frame
double minLength(Rational fps);                // two output frames (§8.1: D ≥ 2)
TimeRange whole(double duration, Rational fps);
// On the grid, ordered and within the file (a range read from a file; "out" 0 = the whole file).
TimeRange valid(TimeRange r, double duration, Rational fps);
// I at t: an In at or after Out moves Out to the file's end. O at t: an Out at or before In moves In to the start.
TimeRange withIn(TimeRange r, double t, double duration, Rational fps);
TimeRange withOut(TimeRange r, double t, double duration, Rational fps);
// Alt+I, Alt+O.
TimeRange clearIn(TimeRange r, double duration, Rational fps);
TimeRange clearOut(TimeRange r, double duration, Rational fps);
// A handle dragged: it stops two frames from the other end.
TimeRange dragIn(TimeRange r, double t, double duration, Rational fps);
TimeRange dragOut(TimeRange r, double t, double duration, Rational fps);
// The bar dragged: the length kept, the range kept within the file.
TimeRange moved(TimeRange r, double delta, double duration, Rational fps);

} // namespace timerange

} // namespace avloop
