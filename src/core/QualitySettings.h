#pragma once

#include <QString>

namespace avloop {

// How hard the encoder works and how small the file comes out (spec §10.3, §11.1 "quality"). The presets fill CRF and
// Speed (and VP8's bitrate); Custom keeps what was typed. AV-009's Quality panel edits it; Encoded preview uses it.
struct QualitySettings {
    enum class Preset { Low, Medium, High, Custom };

    Preset preset = Preset::Medium;
    int crf = 31;       // VP9 4–63 (VP8 4–63 as well, its own scale)
    int speed = 2;      // libvpx cpu-used 0–8
    int maxBitrate = 0; // kbit/s, 0 = none
    bool twoPass = false;
    double targetMB = 0; // 0 = none (AV-009)

    friend bool operator==(const QualitySettings&, const QualitySettings&) = default;

    // The preset's values for a format (VP9 or VP8); Custom returns itself.
    QualitySettings resolved(bool vp8) const;
    // VP8's bitrate for the preset, kbit/s (§10.3: 2M, 6M, 12M); 0 for VP9.
    int vp8Bitrate() const;

    static QString presetId(Preset p); // "low", "medium", "high", "custom"
    static Preset presetFromId(const QString& id);
};

// The output's format (spec §10.2; the others arrive with item 13).
enum class OutputFormat { WebmVp9, WebmVp8 };
QString formatId(OutputFormat f); // "webm-vp9", "webm-vp8"
OutputFormat formatFromId(const QString& id);
QString formatName(OutputFormat f, bool alpha); // "VP9 alpha", "VP8"

} // namespace avloop
