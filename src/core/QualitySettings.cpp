#include "core/QualitySettings.h"

#include <algorithm>

namespace avloop {

QualitySettings QualitySettings::resolved(bool vp8) const {
    QualitySettings q = *this;
    switch (preset) {
    case Preset::Low:
        q.crf = vp8 ? 30 : 40;
        q.speed = 4;
        break;
    case Preset::Medium:
        q.crf = vp8 ? 15 : 31;
        q.speed = 2;
        break;
    case Preset::High:
        q.crf = vp8 ? 6 : 24;
        q.speed = 1;
        break;
    case Preset::Custom:
        q.crf = std::clamp(crf, 4, 63);
        q.speed = std::clamp(speed, 0, 8);
        break;
    }
    return q;
}

int QualitySettings::vp8Bitrate() const {
    switch (preset) {
    case Preset::Low:
        return 2000;
    case Preset::High:
        return 12000;
    case Preset::Medium:
    case Preset::Custom:
        break;
    }
    return maxBitrate > 0 && preset == Preset::Custom ? maxBitrate : 6000;
}

QString QualitySettings::presetId(Preset p) {
    switch (p) {
    case Preset::Low:
        return QStringLiteral("low");
    case Preset::High:
        return QStringLiteral("high");
    case Preset::Custom:
        return QStringLiteral("custom");
    case Preset::Medium:
        break;
    }
    return QStringLiteral("medium");
}

QualitySettings::Preset QualitySettings::presetFromId(const QString& id) {
    for (Preset p : {Preset::Low, Preset::Medium, Preset::High, Preset::Custom})
        if (presetId(p) == id) return p;
    return Preset::Medium;
}

QString formatId(OutputFormat f) {
    return f == OutputFormat::WebmVp8 ? QStringLiteral("webm-vp8") : QStringLiteral("webm-vp9");
}

OutputFormat formatFromId(const QString& id) {
    return id == QLatin1String("webm-vp8") ? OutputFormat::WebmVp8 : OutputFormat::WebmVp9;
}

QString formatName(OutputFormat f, bool alpha) {
    const QString codec = f == OutputFormat::WebmVp8 ? QStringLiteral("VP8") : QStringLiteral("VP9");
    return alpha ? codec + QStringLiteral(" alpha") : codec;
}

} // namespace avloop
