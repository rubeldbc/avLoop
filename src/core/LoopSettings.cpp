#include "core/LoopSettings.h"

#include <QList>

#include <algorithm>
#include <cmath>

namespace avloop {

LoopSettings LoopSettings::clamped() const {
    LoopSettings s = *this;
    s.speed = std::clamp(std::round(speed / 0.05) * 0.05, kMinSpeed, kMaxSpeed);
    s.fade = std::max(0.0, fade);
    s.turn = std::max(0.0, turn);
    return s;
}

QString LoopSettings::modeId(Mode m) {
    switch (m) {
    case Mode::Simple:
        return QStringLiteral("simple");
    case Mode::PingPong:
        return QStringLiteral("pingpong");
    case Mode::PingPongSmooth:
        return QStringLiteral("pingpong-smooth");
    case Mode::Crossfade:
        break;
    }
    return QStringLiteral("crossfade");
}

LoopSettings::Mode LoopSettings::modeFromId(const QString& id) {
    for (Mode m : {Mode::Simple, Mode::Crossfade, Mode::PingPong, Mode::PingPongSmooth})
        if (modeId(m) == id) return m;
    return Mode::Crossfade;
}

QString LoopSettings::curveId(Curve c) {
    return c == Curve::Linear ? QStringLiteral("linear") : QStringLiteral("smooth");
}

LoopSettings::Curve LoopSettings::curveFromId(const QString& id) {
    return id == QLatin1String("linear") ? Curve::Linear : Curve::Smooth;
}

QString LoopSettings::methodId(Method m) {
    switch (m) {
    case Method::FrameBlend:
        return QStringLiteral("blend");
    case Method::Motion:
        return QStringLiteral("motion");
    case Method::Nearest:
        break;
    }
    return QStringLiteral("nearest");
}

LoopSettings::Method LoopSettings::methodFromId(const QString& id) {
    for (Method m : {Method::Nearest, Method::FrameBlend, Method::Motion})
        if (methodId(m) == id) return m;
    return Method::Nearest;
}

const QList<Rational>& OutputSettings::commonRates() {
    static const QList<Rational> rates = {{15, 1}, {24000, 1001}, {24, 1},       {25, 1}, {30000, 1001},
                                          {30, 1}, {50, 1},       {60000, 1001}, {60, 1}};
    return rates;
}

Rational OutputSettings::customRate(double fps) {
    const double v = std::clamp(fps, 1.0, 60.0);
    return Rational{int(std::lround(v * 1000)), 1000}.reduced();
}

Rational OutputSettings::effectiveFps(Rational source, bool vfr) const {
    if (!sourceFps) return fps.isValid() && fps.value() <= 60.0 + 1e-9 ? fps : Rational{60, 1};
    if (!source.isValid()) return {30, 1};
    if (vfr) {
        // A variable rate takes the nearest common one.
        Rational best = commonRates().first();
        for (const Rational& r : commonRates())
            if (std::abs(r.value() - source.value()) < std::abs(best.value() - source.value())) best = r;
        return best;
    }
    return source.value() > 60.0 + 1e-9 ? Rational{60, 1} : source.reduced();
}

QString OutputSettings::fpsId() const {
    return sourceFps ? QStringLiteral("source") : QStringLiteral("%1/%2").arg(fps.num).arg(fps.den);
}

OutputSettings OutputSettings::fromFpsId(const QString& id) {
    OutputSettings o;
    const qsizetype slash = id.indexOf(QLatin1Char('/'));
    if (id == QLatin1String("source") || slash < 0) return o;
    const Rational r{id.left(slash).toInt(), id.mid(slash + 1).toInt()};
    if (!r.isValid()) return o;
    o.sourceFps = false;
    o.fps = r.value() > 60.0 + 1e-9 ? Rational{60, 1} : r;
    return o;
}

} // namespace avloop
