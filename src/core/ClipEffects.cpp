#include "core/ClipEffects.h"

#include <QHashFunctions>

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace avloop {

namespace {

constexpr int kTable = 4096; // the transfer curves' table: 4097 points, read with linear interpolation

float exactLinear(float v) {
    return v <= 0.04045f ? v / 12.92f : std::pow((v + 0.055f) / 1.055f, 2.4f);
}

float exactSrgb(float v) {
    return v <= 0.0031308f ? v * 12.92f : 1.055f * std::pow(v, 1 / 2.4f) - 0.055f;
}

struct Curve {
    std::array<float, kTable + 1> y{};
    explicit Curve(float (*f)(float)) {
        for (int i = 0; i <= kTable; ++i)
            y[std::size_t(i)] = f(float(i) / kTable);
    }
    float operator()(float v) const {
        const float x = std::clamp(v, 0.f, 1.f) * kTable;
        const int i = std::min(int(x), kTable - 1);
        return y[std::size_t(i)] + (x - float(i)) * (y[std::size_t(i) + 1] - y[std::size_t(i)]);
    }
};

float clamp01(float v) {
    return std::clamp(v, 0.f, 1.f);
}

float smoothstep(float e0, float e1, float x) {
    const float t = clamp01((x - e0) / (e1 - e0));
    return t * t * (3 - 2 * t);
}

} // namespace

namespace effects {

float toLinear(float v) {
    static const Curve curve(exactLinear);
    return curve(v);
}

float toSrgb(float v) {
    static const Curve curve(exactSrgb);
    return curve(v);
}

Colour::Colour(const ClipEffects& e)
    : light_(e.exposure != 0 || e.temperature != 0 || e.tint != 0), contrast_(e.contrast != 0),
      tones_(e.shadows != 0 || e.highlights != 0 || e.whites != 0 || e.blacks != 0), hue_(e.hue != 0),
      saturate_(e.saturation != 100 || e.vibrance != 0), sepia_(e.sepia > 0), invert_(e.invert) {
    // White balance: warmer lifts red and lowers blue, more magenta lowers green.
    const float gain = std::exp2(float(e.exposure));
    const auto t = float(e.temperature / 100);
    const auto m = float(e.tint / 100);
    gain_[0] = gain * (1 + 0.25f * t);
    gain_[1] = gain * (1 - 0.2f * m);
    gain_[2] = gain * (1 - 0.25f * t);
    const auto c = float(e.contrast / 100);
    k_ = c >= 0 ? 1 + 2 * c : 1 + c;
    tone_[0] = float(e.shadows / 400);
    tone_[1] = float(e.highlights / 400);
    tone_[2] = float(e.whites / 400);
    tone_[3] = float(e.blacks / 400);
    // The hue turned about the grey axis (the W3C filter's hue-rotate matrix).
    const double a = std::cos(e.hue * std::numbers::pi / 180);
    const double b = std::sin(e.hue * std::numbers::pi / 180);
    const double h[9] = {0.213 + 0.787 * a - 0.213 * b, 0.715 - 0.715 * a - 0.715 * b, 0.072 - 0.072 * a + 0.928 * b,
                         0.213 - 0.213 * a + 0.143 * b, 0.715 + 0.285 * a + 0.140 * b, 0.072 - 0.072 * a - 0.283 * b,
                         0.213 - 0.213 * a - 0.787 * b, 0.715 - 0.715 * a + 0.715 * b, 0.072 + 0.928 * a + 0.072 * b};
    for (int i = 0; i < 9; ++i)
        hueM_[i] = float(h[i]);
    sat_ = float(e.saturation / 100);
    vib_ = float(e.vibrance / 100);
    sepiaP_ = float(e.sepia / 100);
}

void Colour::operator()(float rgb[3]) const {
    if (light_)
        for (int i = 0; i < 3; ++i)
            rgb[i] = toSrgb(std::min(1.f, toLinear(rgb[i]) * gain_[i]));
    if (contrast_)
        for (int i = 0; i < 3; ++i)
            rgb[i] = clamp01((rgb[i] - 0.5f) * k_ + 0.5f);
    if (tones_)
        for (int i = 0; i < 3; ++i) {
            // Shadows peak at a third, highlights at two thirds; whites lift the top, blacks the bottom.
            const float x = rgb[i];
            const float y = 1 - x;
            rgb[i] = clamp01(x + tone_[0] * 6.75f * x * y * y + tone_[1] * 6.75f * x * x * y + tone_[2] * x * x +
                             tone_[3] * y * y);
        }
    if (hue_) {
        const float r = rgb[0], g = rgb[1], b = rgb[2];
        for (int i = 0; i < 3; ++i)
            rgb[i] = clamp01(hueM_[3 * i] * r + hueM_[3 * i + 1] * g + hueM_[3 * i + 2] * b);
    }
    if (saturate_) {
        const float luma = 0.2126f * rgb[0] + 0.7152f * rgb[1] + 0.0722f * rgb[2];
        const float spread = std::max({rgb[0], rgb[1], rgb[2]}) - std::min({rgb[0], rgb[1], rgb[2]});
        const float f = sat_ * std::max(0.f, 1 + vib_ * (1 - spread));
        for (int i = 0; i < 3; ++i)
            rgb[i] = clamp01(luma + (rgb[i] - luma) * f);
    }
    if (sepia_) {
        const float r = rgb[0], g = rgb[1], b = rgb[2];
        const float s[3] = {0.393f * r + 0.769f * g + 0.189f * b, 0.349f * r + 0.686f * g + 0.168f * b,
                            0.272f * r + 0.534f * g + 0.131f * b};
        for (int i = 0; i < 3; ++i)
            rgb[i] = rgb[i] + (std::min(1.f, s[i]) - rgb[i]) * sepiaP_;
    }
    if (invert_)
        for (int i = 0; i < 3; ++i)
            rgb[i] = 1 - rgb[i];
}

} // namespace effects

ClipEffects ClipEffects::clamped() const {
    ClipEffects e = *this;
    e.exposure = std::clamp(exposure, -4.0, 4.0);
    for (double* v : {&e.contrast, &e.highlights, &e.shadows, &e.whites, &e.blacks, &e.temperature, &e.tint,
                      &e.vibrance, &e.vignette})
        *v = std::clamp(*v, -100.0, 100.0);
    e.saturation = std::clamp(saturation, 0.0, 200.0);
    e.hue = std::clamp(hue, -180.0, 180.0);
    for (double* v : {&e.sepia, &e.vignetteSize, &e.opacity, &e.sharpen, &e.blur})
        *v = std::clamp(*v, 0.0, 100.0);
    e.posterize = posterize < 2 ? 0 : std::min(posterize, 32);
    return e;
}

bool ClipEffects::isNeutral() const {
    ClipEffects none;
    none.on = on;
    return !on || *this == none;
}

bool ClipEffects::changesColour() const {
    return on &&
           (exposure != 0 || contrast != 0 || highlights != 0 || shadows != 0 || whites != 0 || blacks != 0 ||
            temperature != 0 || tint != 0 || saturation != 100 || vibrance != 0 || hue != 0 || sepia > 0 || invert);
}

std::uint64_t ClipEffects::key() const {
    if (isNeutral()) return 0;
    const std::size_t h =
        qHashMulti(0x5eed, exposure, contrast, highlights, shadows, whites, blacks, temperature, tint, saturation,
                   vibrance, hue, sepia, invert, posterize, vignette, vignetteSize, opacity, sharpen, blur);
    return h ? std::uint64_t(h) : 1;
}

void ClipEffects::colour(float rgb[3]) const {
    effects::Colour (*this)(rgb);
}

float ClipEffects::posterized(float v) const {
    if (posterize < 2) return v;
    const auto steps = float(posterize - 1);
    return std::round(clamp01(v) * steps) / steps;
}

float ClipEffects::vignetteWeight(float u, float v) const {
    // r: 0 in the middle, 1 in the corners; the clear middle reaches 0.7 × size, the edge darkens over 0.65 more.
    const float dx = (u - 0.5f) * 2;
    const float dy = (v - 0.5f) * 2;
    const float r = std::sqrt((dx * dx + dy * dy) / 2);
    const auto inner = float(0.7 * vignetteSize / 100);
    return smoothstep(inner, inner + 0.65f, r);
}

void ClipEffects::vignetteOn(float rgb[3], float w) const {
    const auto a = float(vignette / 100);
    for (int i = 0; i < 3; ++i)
        rgb[i] = a >= 0 ? rgb[i] * (1 - a * w) : rgb[i] + (-a) * w * (1 - rgb[i]);
}

QJsonObject ClipEffects::toJson() const {
    return {
        {QStringLiteral("on"), on},
        {QStringLiteral("exposure"), exposure},
        {QStringLiteral("contrast"), contrast},
        {QStringLiteral("highlights"), highlights},
        {QStringLiteral("shadows"), shadows},
        {QStringLiteral("whites"), whites},
        {QStringLiteral("blacks"), blacks},
        {QStringLiteral("temperature"), temperature},
        {QStringLiteral("tint"), tint},
        {QStringLiteral("saturation"), saturation},
        {QStringLiteral("vibrance"), vibrance},
        {QStringLiteral("hue"), hue},
        {QStringLiteral("sepia"), sepia},
        {QStringLiteral("invert"), invert},
        {QStringLiteral("posterize"), posterize},
        {QStringLiteral("vignette"), vignette},
        {QStringLiteral("vignetteSize"), vignetteSize},
        {QStringLiteral("opacity"), opacity},
        {QStringLiteral("sharpen"), sharpen},
        {QStringLiteral("blur"), blur},
    };
}

ClipEffects ClipEffects::fromJson(const QJsonObject& j) {
    ClipEffects e;
    e.on = j.value(QStringLiteral("on")).toBool(true);
    auto read = [&j](const char* key, double& v) { v = j.value(QLatin1String(key)).toDouble(v); };
    read("exposure", e.exposure);
    read("contrast", e.contrast);
    read("highlights", e.highlights);
    read("shadows", e.shadows);
    read("whites", e.whites);
    read("blacks", e.blacks);
    read("temperature", e.temperature);
    read("tint", e.tint);
    read("saturation", e.saturation);
    read("vibrance", e.vibrance);
    read("hue", e.hue);
    read("sepia", e.sepia);
    read("vignette", e.vignette);
    read("vignetteSize", e.vignetteSize);
    read("opacity", e.opacity);
    read("sharpen", e.sharpen);
    read("blur", e.blur);
    e.invert = j.value(QStringLiteral("invert")).toBool(false);
    e.posterize = j.value(QStringLiteral("posterize")).toInt(0);
    return e.clamped();
}

} // namespace avloop
