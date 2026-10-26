#include "core/LoopSettings.h"

#include "core/Geometry.h"

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

QSize OutputSettings::sizeFor(const QSizeF& frame, bool* scaled) const {
    QSizeF want = frame;
    if (size == Size::LongSide && frame.width() > 0 && frame.height() > 0)
        want = frame * (double(longSide) / std::max(frame.width(), frame.height()));
    if (size == Size::Custom) want = QSizeF(custom);
    if (scaled) *scaled = std::max(want.width(), want.height()) > geometry::kMaxLongSide + 0.5;
    return geometry::outputSize(want);
}

const QList<int>& OutputSettings::longSides() {
    static const QList<int> sides = {1920, 1280, 1080, 720, 512, 256};
    return sides;
}

QString OutputSettings::sizeId() const {
    if (size == Size::LongSide) return QStringLiteral("long-%1").arg(longSide);
    return size == Size::Custom ? QStringLiteral("custom") : QStringLiteral("crop");
}

void OutputSettings::setSizeId(const QString& id) {
    size = Size::Crop;
    if (id == QLatin1String("custom")) size = Size::Custom;
    if (id.startsWith(QLatin1String("long-"))) {
        const int n = id.mid(5).toInt();
        if (longSides().contains(n)) {
            size = Size::LongSide;
            longSide = n;
        }
    }
}

QString OutputSettings::alphaId(Alpha a) {
    return a == Alpha::On ? QStringLiteral("on") : a == Alpha::Off ? QStringLiteral("off") : QStringLiteral("auto");
}

OutputSettings::Alpha OutputSettings::alphaFromId(const QString& id) {
    return id == QLatin1String("on") ? Alpha::On : id == QLatin1String("off") ? Alpha::Off : Alpha::Auto;
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
    // GIF's frame delays are hundredths: at most 50 fps (spec §10.4).
    const Rational rate = sourceRate(source, vfr);
    const int cap = formatMaxFps(format);
    return rate.value() > cap + 1e-9 ? Rational{cap, 1} : rate;
}

Rational OutputSettings::sourceRate(Rational source, bool vfr) const {
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
