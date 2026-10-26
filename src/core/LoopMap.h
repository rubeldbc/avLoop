#pragma once

#include "core/LoopSettings.h"

namespace avloop {

// One output frame from the segment (spec §8.2): S[a] alone when w == 0, else (1 − w)·S[a] + w·S[b].
struct Pick {
    int a = 0;
    int b = 0;
    double w = 0;
    friend bool operator==(const Pick&, const Pick&) = default;
};

// The loop's maths (spec §8.1, §8.2), pure: segment frames D, the output length L and output frame k → segment frames.
// F and T are in output frames; they are clamped here (F 1 … ⌊D/2⌋, T 1 … ⌊(D − 1)/2⌋).
namespace loopmap {

using Mode = LoopSettings::Mode;
using Curve = LoopSettings::Curve;

// D = round((tOut − tIn) × fps / s), at least 2 (3 for the Ping-pongs).
int segmentFrames(double tIn, double tOut, Rational fps, double speed, Mode mode);
int clampFade(int fadeFrames, int segment);
int clampTurn(int turnFrames, int segment);
// Seconds ↔ whole output frames for F and T.
int framesOf(double seconds, Rational fps);
double secondsOf(int frames, Rational fps);
int length(Mode mode, int segment, int fade, int turn);
Pick pick(Mode mode, int segment, int fade, int turn, Curve curve, int k);
// The smooth turn's position p of output frame k (0 … D − 1, fractional at the ends).
double smoothPosition(int segment, int turn, int k);
// The crossfade's curve c(x).
double curve(Curve c, double x);

} // namespace loopmap

// Everything about one loop the panel, the timeline and the renderer need, worked out once.
struct LoopPlan {
    int segment = 0; // D
    int fade = 0;    // F after the clamp (Crossfade)
    int turn = 0;    // T after the clamp (Ping-pong + Smooth turn)
    int length = 0;  // L
    bool fadeClamped = false;
    bool turnClamped = false;
    static LoopPlan of(const LoopSettings& loop, double tIn, double tOut, Rational fps);
};

} // namespace avloop
