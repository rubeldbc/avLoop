#pragma once

#include "core/Rational.h"

#include <QString>

#include <cstdint>

namespace avloop::timecode {

// Time on the output frame grid (spec §6.4, §7, §8.1): frame n of the grid starts at n / fps seconds, measured from
// the source's start. Plain functions, Qt Core only.

// The output frame rate until AV-006's setting: the source's, at most 60 (reading R-6); 30 when unknown.
Rational defaultFps(Rational source);
double secondsOf(std::int64_t frame, Rational fps);
// The grid frame a time falls in; a hair of tolerance keeps secondsOf(n) in frame n.
std::int64_t frameOf(double seconds, Rational fps);
// The nearest grid frame and its time.
std::int64_t nearestFrame(double seconds, Rational fps);
double snap(double seconds, Rational fps);
// "HH:MM:SS.FF": the whole seconds of the frame's time and FF its frame within that second.
QString format(double seconds, Rational fps);
// What a time field accepts: "12.4" (seconds), "00:00:12.12", "1:02.05" or "12.05" after a colon (FF frames), "372f"
// (a frame). Returns false for anything else.
bool parse(const QString& text, Rational fps, double* seconds);

} // namespace avloop::timecode
