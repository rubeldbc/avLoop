#pragma once

#include "core/QualitySettings.h"
#include "core/Rational.h"

#include <QList>
#include <QSize>
#include <QSizeF>
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

// What the export is (spec §10.2, §11.1 "output"): the format, the size, alpha and the colour behind it, the rate.
struct OutputSettings {
    enum class Size { Crop, LongSide, Custom };
    enum class Alpha { Auto, On, Off };

    OutputFormat format = OutputFormat::WebmVp9;
    Size size = Size::Crop;                         // reading R-6: Crop size
    int longSide = 1080;                            // Long side: 1920, 1280, 1080, 720, 512 or 256
    QSize custom{640, 640};                         // Custom W×H
    bool lockRatio = true;                          // Custom keeps the crop's ratio while typing
    Alpha alpha = Alpha::Auto;                      // Auto: only when there are transparent parts
    QString background = QStringLiteral("#000000"); // behind transparency when Alpha is Off
    bool sourceFps = true;                          // "Source" (reading R-6): the stream's rate, at most 60
    Rational fps{30, 1};                            // a chosen rate (15 … 60 or Custom 1–60, three decimals)

    // The output's size for a frame of `frame` scene px (spec §10.2): its own size, its long side made N, or W×H; even,
    // ≥ 16, the long side ≤ 1920 (`scaled` set when that limit took it down).
    QSize sizeFor(const QSizeF& frame, bool* scaled = nullptr) const;
    static const QList<int>& longSides();
    QString sizeId() const; // "crop", "long-1080", "custom"
    void setSizeId(const QString& id);
    static QString alphaId(Alpha a); // "auto", "on", "off"
    static Alpha alphaFromId(const QString& id);

    friend bool operator==(const OutputSettings&, const OutputSettings&) = default;
    // The rate the output has: the chosen one, or the source's (avg_frame_rate; a VFR source's nearest common rate),
    // never above 60.
    Rational effectiveFps(Rational sourceFps, bool vfr) const; // the rate, capped by the format (GIF 50)
    Rational sourceRate(Rational sourceFps, bool vfr) const;   // the rate before the format's cap
    // "source", "30/1", "30000/1001" (spec §11.1).
    QString fpsId() const;
    static OutputSettings fromFpsId(const QString& id);
    // 15, 23.976, 24, 25, 29.97, 30, 50, 59.94, 60.
    static const QList<Rational>& commonRates();
    static Rational customRate(double fps); // 1–60, three decimals
};

} // namespace avloop
