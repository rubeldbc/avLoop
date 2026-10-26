#pragma once

#include "core/Rational.h"

#include <QList>
#include <QString>

namespace avloop {

// How the segment becomes a loop (spec §8, §11.1 "loop"): the mode, the crossfade's length and curve, the smooth
// turn's length, Speed and how a segment frame is made. Times in seconds; they sit on the output grid when used.
struct LoopSettings {
    enum class Mode { Simple, Crossfade, PingPong, PingPongSmooth };
    enum class Curve { Linear, Smooth };
    enum class Method { Nearest, FrameBlend, Motion };
    static constexpr double kMinSpeed = 0.10;
    static constexpr double kMaxSpeed = 4.00;

    Mode mode = Mode::Crossfade; // reading R-28: a seamless loop is the point
    double fade = 0.5;           // F, seconds
    Curve curve = Curve::Smooth;
    double turn = 0.3; // T, seconds
    double speed = 1.0;
    Method method = Method::Nearest;

    LoopSettings clamped() const; // Speed 0.10–4.00 on its 0.05 steps; F and T ≥ 0
    friend bool operator==(const LoopSettings&, const LoopSettings&) = default;

    static QString modeId(Mode m); // "simple", "crossfade", "pingpong", "pingpong-smooth"
    static Mode modeFromId(const QString& id);
    static QString curveId(Curve c); // "linear", "smooth"
    static Curve curveFromId(const QString& id);
    static QString methodId(Method m); // "nearest", "blend", "motion"
    static Method methodFromId(const QString& id);
};

// What the export is (spec §10.2, §11.1 "output"); AV-006 holds the frame rate, AV-009 the rest.
struct OutputSettings {
    bool sourceFps = true; // "Source" (reading R-6): the stream's rate, at most 60
    Rational fps{30, 1};   // a chosen rate (15 … 60 or Custom 1–60, three decimals)

    friend bool operator==(const OutputSettings&, const OutputSettings&) = default;
    // The rate the output has: the chosen one, or the source's (avg_frame_rate; a VFR source's nearest common rate),
    // never above 60.
    Rational effectiveFps(Rational sourceFps, bool vfr) const;
    // "source", "30/1", "30000/1001" (spec §11.1).
    QString fpsId() const;
    static OutputSettings fromFpsId(const QString& id);
    // 15, 23.976, 24, 25, 29.97, 30, 50, 59.94, 60.
    static const QList<Rational>& commonRates();
    static Rational customRate(double fps); // 1–60, three decimals
};

} // namespace avloop
