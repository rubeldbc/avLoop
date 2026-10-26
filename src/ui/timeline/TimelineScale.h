#pragma once

#include "core/Rational.h"

#include <QList>
#include <QString>

namespace avloop::timelinescale {

// The ruler's ticks (spec §7.1): the step between labelled ticks follows the zoom — 1 frame up to 10 minutes, longer
// for a very long file shown whole — and is the smallest whose spacing holds the widest label with room, so labels
// never overlap. Minor ticks divide it while they stay apart.
struct Steps {
    double major = 1; // seconds
    double minor = 1; // seconds; == major when it is not divided
};
Steps steps(double pxPerSecond, Rational fps, double labelWidthPx, bool frameLabels);

struct Tick {
    double seconds = 0;
    bool major = false;
};
QList<Tick> ticks(double from, double to, const Steps& s);

// A major tick's label: "HH:MM:SS.FF" or the frame number.
QString label(double seconds, Rational fps, bool frameLabels);
// The widest label a file of `duration` can show.
QString widestLabel(double duration, Rational fps, bool frameLabels);

// The zoom's limits (spec §7.2): the whole file in `widthPx`, down to 12 px a frame.
double minPxPerSecond(double duration, double widthPx);
double maxPxPerSecond(Rational fps);

} // namespace avloop::timelinescale
