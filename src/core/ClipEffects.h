#pragma once

#include <QJsonObject>
#include <QString>

#include <cstdint>

namespace avloop {

// The clip's effects (D-017, the side panel's Clip effects): colour correction and looks applied to every source frame
// before it is placed — on the canvas, in the Preview and in the export alike. Not keyframed; a new video starts with
// none. Neutral (off, or every value its default): the frames pass through untouched, byte for byte.
//
// Order: blur, sharpen (premultiplied, in the frame's pixels), then each pixel straight: the colour replaced (on the
// clip's own colours, so a colour picked from it is found), exposure and white balance in linear light, contrast, the
// four tones, hue, saturation and vibrance, colourise, split toning, duotone, sepia, invert, posterize, the vignette,
// the opacity. Values outside their ranges are clamped; colours are 0xRRGGBB.
struct ClipEffects {
    bool on = true; // off: kept in the project, not applied (before / after)
    // Light
    double exposure = 0;   // EV, −4 … 4
    double contrast = 0;   // −100 … 100
    double highlights = 0; // −100 … 100, each tone
    double shadows = 0;
    double whites = 0;
    double blacks = 0;
    // Colour
    double temperature = 0;  // −100 (cooler) … 100 (warmer)
    double tint = 0;         // −100 (greener) … 100 (more magenta)
    double saturation = 100; // %, 0 … 200
    double vibrance = 0;     // −100 … 100: the less saturated colours more
    double hue = 0;          // degrees, −180 … 180
    // Colour theme (D-018): each its colours and an amount, 0 % off
    std::uint32_t replaceFrom = 0xff8000;     // Replace: the clip's colour …
    std::uint32_t replaceTo = 0xe01010;       // … and the colour it becomes
    double replaceRange = 30;                 // degrees of hue either side taken whole, 0 … 180
    double replaceSoftness = 20;              // and the degrees beyond it fading out, 0 … 180
    double replace = 0;                       // %, 0 … 100
    std::uint32_t colouriseColour = 0xe02020; // Colourise: every pixel this hue, its lightness kept
    double colourise = 0;
    std::uint32_t splitShadows = 0x2060ff; // Split toning: the shadows' tint and the highlights'
    std::uint32_t splitHighlights = 0xffa040;
    double split = 0;
    std::uint32_t duotoneDark = 0x1a1040; // Duotone: black becomes the one, white the other
    std::uint32_t duotoneLight = 0xffb060;
    double duotone = 0;
    // Creative
    double sepia = 0; // %, 0 … 100
    bool invert = false;
    int posterize = 0;        // levels a channel, 2 … 32; 0 off
    double vignette = 0;      // −100 (lighter edges) … 100 (darker edges)
    double vignetteSize = 50; // the clear middle, 0 … 100
    double opacity = 100;     // %, 0 … 100
    // Detail
    double sharpen = 0; // 0 … 100
    double blur = 0;    // radius in source px, 0 … 100

    friend bool operator==(const ClipEffects&, const ClipEffects&) = default;

    ClipEffects clamped() const;
    bool isNeutral() const;
    // A pixel's colour changes by colour() (everything but blur, sharpen, posterize, the vignette and the opacity).
    bool changesColour() const;
    // 0 when neutral; otherwise differs between two settings that make different pictures (a texture cache's key).
    std::uint64_t key() const;

    // One pixel's colour by every per-pixel effect but posterize, the vignette and the opacity: straight (not
    // premultiplied) sRGB values 0 … 1 in and out.
    void colour(float rgb[3]) const;
    float posterized(float v) const; // posterize 2 … 32: v on its n levels
    // The vignette's weight at the clip's (u, v) (0 … 1 across it): 0 in the clear middle, rising to the corners.
    float vignetteWeight(float u, float v) const;
    // The vignette at weight w on a straight colour.
    void vignetteOn(float rgb[3], float w) const;
    // Blur's Gaussian σ and sharpen's, in source px.
    double blurSigma() const { return blur / 2; }
    static constexpr double kSharpenSigma = 1.0;

    static QString hex(std::uint32_t colour); // "#rrggbb"
    QJsonObject toJson() const;
    static ClipEffects fromJson(const QJsonObject& json);
};

namespace effects {

float toLinear(float v); // the sRGB curve, decoded
float toSrgb(float v);   // and encoded, v in 0 … 1

// ClipEffects::colour with its constants worked out once: what a pass over every pixel uses.
class Colour {
public:
    explicit Colour(const ClipEffects& e);
    void operator()(float rgb[3]) const;

private:
    bool light_, contrast_, tones_, hue_, saturate_, sepia_, invert_;
    float replace_ = 0, fromHue_ = 0, turn_ = 0, ratioS_ = 1, ratioV_ = 1, range_ = 0, soft_ = 0;
    float colourise_ = 0, ciHue_ = 0, ciSat_ = 0;
    float split_ = 0, tintS_[3] = {}, tintH_[3] = {};
    float duotone_ = 0, dark_[3] = {}, light3_[3] = {};
    float gain_[3] = {1, 1, 1};    // exposure × white balance, linear
    float k_ = 1;                  // contrast about 0.5
    float tone_[4] = {0, 0, 0, 0}; // shadows, highlights, whites, blacks
    float hueM_[9] = {};
    float sat_ = 1, vib_ = 0, sepiaP_ = 0;
};

} // namespace effects
} // namespace avloop
