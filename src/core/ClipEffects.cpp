#include "core/ClipEffects.h"

#include <QColor>
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

float luma(const float c[3]) {
    return 0.2126f * c[0] + 0.7152f * c[1] + 0.0722f * c[2];
}

void rgbOf(std::uint32_t c, float out[3]) {
    out[0] = float((c >> 16) & 0xff) / 255;
    out[1] = float((c >> 8) & 0xff) / 255;
    out[2] = float(c & 0xff) / 255;
}

// Hue in degrees 0 … 360, saturation and value 0 … 1.
struct Hsv {
    float h = 0, s = 0, v = 0;
};

Hsv toHsv(const float c[3]) {
    const float mx = std::max({c[0], c[1], c[2]});
    const float d = mx - std::min({c[0], c[1], c[2]});
    Hsv o{0, mx > 0 ? d / mx : 0, mx};
    if (d > 0) {
        if (mx == c[0])
            o.h = 60 * std::fmod((c[1] - c[2]) / d + 6, 6.f);
        else if (mx == c[1])
            o.h = 60 * ((c[2] - c[0]) / d + 2);
        else
            o.h = 60 * ((c[0] - c[1]) / d + 4);
    }
    return o;
}

// A hue's colour of chroma c, its smallest channel m.
void fromHue(float h, float c, float m, float out[3]) {
    const float hp = std::fmod(h + 360, 360.f) / 60;
    const float x = c * (1 - std::abs(std::fmod(hp, 2.f) - 1));
    const int sector = std::min(5, int(hp));
    const float r[6] = {c, x, 0, 0, x, c}, g[6] = {x, c, c, x, 0, 0}, b[6] = {0, 0, x, c, c, x};
    out[0] = clamp01(r[sector] + m);
    out[1] = clamp01(g[sector] + m);
    out[2] = clamp01(b[sector] + m);
}

void fromHsv(const Hsv& p, float out[3]) {
    fromHue(p.h, p.v * p.s, p.v - p.v * p.s, out);
}

void fromHsl(float h, float s, float l, float out[3]) {
    const float c = (1 - std::abs(2 * l - 1)) * s;
    fromHue(h, c, l - c / 2, out);
}

// A colour's hue and HSL saturation.
void hueSat(std::uint32_t colour, float& h, float& s) {
    float c[3];
    rgbOf(colour, c);
    const float mx = std::max({c[0], c[1], c[2]});
    const float mn = std::min({c[0], c[1], c[2]});
    const float l = (mx + mn) / 2;
    h = toHsv(c).h;
    s = mx == mn ? 0 : (mx - mn) / (1 - std::abs(2 * l - 1));
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
    // Replace: the hue turned by the From → To turn, saturation and value scaled by To over From, by the weight of how
    // near the pixel's hue is to From's (and how coloured it is — greys stay).
    replace_ = float(e.replace / 100);
    float from[3], to[3];
    rgbOf(e.replaceFrom, from);
    rgbOf(e.replaceTo, to);
    const Hsv hf = toHsv(from), ht = toHsv(to);
    fromHue_ = hf.h;
    turn_ = std::fmod(ht.h - hf.h + 540, 360.f) - 180;
    ratioS_ = hf.s > 0.05f ? ht.s / hf.s : 1;
    ratioV_ = hf.v > 0.05f ? ht.v / hf.v : 1;
    range_ = float(e.replaceRange);
    soft_ = float(e.replaceSoftness);
    colourise_ = float(e.colourise / 100);
    hueSat(e.colouriseColour, ciHue_, ciSat_);
    // Split toning's tints: each colour at half lightness, less grey — what it adds.
    split_ = float(e.split / 100);
    float hue = 0, sat = 0;
    hueSat(e.splitShadows, hue, sat);
    fromHsl(hue, sat, 0.5f, tintS_);
    hueSat(e.splitHighlights, hue, sat);
    fromHsl(hue, sat, 0.5f, tintH_);
    for (int i = 0; i < 3; ++i) {
        tintS_[i] -= 0.5f;
        tintH_[i] -= 0.5f;
    }
    duotone_ = float(e.duotone / 100);
    rgbOf(e.duotoneDark, dark_);
    rgbOf(e.duotoneLight, light3_);
}

void Colour::operator()(float rgb[3]) const {
    if (replace_ > 0) {
        Hsv p = toHsv(rgb);
        float d = std::abs(p.h - fromHue_);
        if (d > 180) d = 360 - d;
        const float near = soft_ > 0 ? 1 - smoothstep(range_, range_ + soft_, d) : (d <= range_ ? 1.f : 0.f);
        const float w = replace_ * near * smoothstep(0.08f, 0.25f, p.s);
        if (w > 0) {
            p.h += w * turn_;
            p.s = clamp01(p.s * (1 + w * (ratioS_ - 1)));
            p.v = clamp01(p.v * (1 + w * (ratioV_ - 1)));
            fromHsv(p, rgb);
        }
    }
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
    if (colourise_ > 0) {
        // Every pixel the colour's hue and saturation at its own HSL lightness.
        const float l = (std::max({rgb[0], rgb[1], rgb[2]}) + std::min({rgb[0], rgb[1], rgb[2]})) / 2;
        float c[3];
        fromHsl(ciHue_, ciSat_, l, c);
        for (int i = 0; i < 3; ++i)
            rgb[i] += (c[i] - rgb[i]) * colourise_;
    }
    if (split_ > 0) {
        const float y = luma(rgb);
        const float ws = (1 - y) * (1 - y), wh = y * y;
        for (int i = 0; i < 3; ++i)
            rgb[i] = clamp01(rgb[i] + split_ * 0.6f * (tintS_[i] * ws + tintH_[i] * wh));
    }
    if (duotone_ > 0) {
        const float y = luma(rgb);
        for (int i = 0; i < 3; ++i)
            rgb[i] += (dark_[i] + (light3_[i] - dark_[i]) * y - rgb[i]) * duotone_;
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
    e.replaceRange = std::clamp(replaceRange, 0.0, 180.0);
    e.replaceSoftness = std::clamp(replaceSoftness, 0.0, 180.0);
    for (double* v : {&e.replace, &e.colourise, &e.split, &e.duotone})
        *v = std::clamp(*v, 0.0, 100.0);
    for (std::uint32_t* c : {&e.replaceFrom, &e.replaceTo, &e.colouriseColour, &e.splitShadows, &e.splitHighlights,
                             &e.duotoneDark, &e.duotoneLight})
        *c &= 0xffffffu;
    return e;
}

bool ClipEffects::isNeutral() const {
    // What an effect at 0 % holds (its colours, the vignette's size) changes nothing.
    ClipEffects none;
    none.on = on;
    if (vignette == 0) none.vignetteSize = vignetteSize;
    if (replace == 0) {
        none.replaceFrom = replaceFrom;
        none.replaceTo = replaceTo;
        none.replaceRange = replaceRange;
        none.replaceSoftness = replaceSoftness;
    }
    if (colourise == 0) none.colouriseColour = colouriseColour;
    if (split == 0) {
        none.splitShadows = splitShadows;
        none.splitHighlights = splitHighlights;
    }
    if (duotone == 0) {
        none.duotoneDark = duotoneDark;
        none.duotoneLight = duotoneLight;
    }
    return !on || *this == none;
}

bool ClipEffects::changesColour() const {
    return on && (exposure != 0 || contrast != 0 || highlights != 0 || shadows != 0 || whites != 0 || blacks != 0 ||
                  temperature != 0 || tint != 0 || saturation != 100 || vibrance != 0 || hue != 0 || sepia > 0 ||
                  invert || replace > 0 || colourise > 0 || split > 0 || duotone > 0);
}

std::uint64_t ClipEffects::key() const {
    if (isNeutral()) return 0;
    const std::size_t h =
        qHashMulti(0x5eed, exposure, contrast, highlights, shadows, whites, blacks, temperature, tint, saturation,
                   vibrance, hue, sepia, invert, posterize, vignette, vignetteSize, opacity, sharpen, blur);
    const std::size_t theme =
        qHashMulti(h, replaceFrom, replaceTo, replaceRange, replaceSoftness, replace, colouriseColour, colourise,
                   splitShadows, splitHighlights, split, duotoneDark, duotoneLight, duotone);
    return theme ? std::uint64_t(theme) : 1;
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

QString ClipEffects::hex(std::uint32_t colour) {
    return QStringLiteral("#%1").arg(colour & 0xffffffu, 6, 16, QLatin1Char('0'));
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
        {QStringLiteral("replaceFrom"), hex(replaceFrom)},
        {QStringLiteral("replaceTo"), hex(replaceTo)},
        {QStringLiteral("replaceRange"), replaceRange},
        {QStringLiteral("replaceSoftness"), replaceSoftness},
        {QStringLiteral("replace"), replace},
        {QStringLiteral("colouriseColour"), hex(colouriseColour)},
        {QStringLiteral("colourise"), colourise},
        {QStringLiteral("splitShadows"), hex(splitShadows)},
        {QStringLiteral("splitHighlights"), hex(splitHighlights)},
        {QStringLiteral("split"), split},
        {QStringLiteral("duotoneDark"), hex(duotoneDark)},
        {QStringLiteral("duotoneLight"), hex(duotoneLight)},
        {QStringLiteral("duotone"), duotone},
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
    read("replaceRange", e.replaceRange);
    read("replaceSoftness", e.replaceSoftness);
    read("replace", e.replace);
    read("colourise", e.colourise);
    read("split", e.split);
    read("duotone", e.duotone);
    auto colour = [&j](const char* key, std::uint32_t& c) {
        const QColor read(j.value(QLatin1String(key)).toString());
        if (read.isValid()) c = read.rgb() & 0xffffffu;
    };
    colour("replaceFrom", e.replaceFrom);
    colour("replaceTo", e.replaceTo);
    colour("colouriseColour", e.colouriseColour);
    colour("splitShadows", e.splitShadows);
    colour("splitHighlights", e.splitHighlights);
    colour("duotoneDark", e.duotoneDark);
    colour("duotoneLight", e.duotoneLight);
    e.invert = j.value(QStringLiteral("invert")).toBool(false);
    e.posterize = j.value(QStringLiteral("posterize")).toInt(0);
    return e.clamped();
}

} // namespace avloop
