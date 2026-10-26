#include "core/QualitySettings.h"

#include <algorithm>

namespace avloop {

QualitySettings QualitySettings::resolved(bool vp8) const {
    QualitySettings q = *this;
    if (preset != Preset::Custom) {
        // Max bitrate, Two-pass and Target size are Custom's alone.
        q.maxBitrate = 0;
        q.twoPass = false;
        q.targetMB = 0;
    }
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
    switch (f) {
    case OutputFormat::WebmVp8:
        return QStringLiteral("webm-vp8");
    case OutputFormat::Gif:
        return QStringLiteral("gif");
    case OutputFormat::Apng:
        return QStringLiteral("apng");
    case OutputFormat::Mp4H264:
        return QStringLiteral("mp4-h264");
    case OutputFormat::MovProRes:
        return QStringLiteral("mov-prores");
    case OutputFormat::WebmVp9:
        break;
    }
    return QStringLiteral("webm-vp9");
}

OutputFormat formatFromId(const QString& id) {
    for (OutputFormat f :
         {OutputFormat::WebmVp8, OutputFormat::Gif, OutputFormat::Apng, OutputFormat::Mp4H264, OutputFormat::MovProRes})
        if (formatId(f) == id) return f;
    return OutputFormat::WebmVp9;
}

QString formatName(OutputFormat f, bool alpha) {
    QString codec;
    switch (f) {
    case OutputFormat::WebmVp8:
        codec = QStringLiteral("VP8");
        break;
    case OutputFormat::Gif:
        return QStringLiteral("GIF");
    case OutputFormat::Apng:
        return QStringLiteral("APNG");
    case OutputFormat::Mp4H264:
        return QStringLiteral("H.264");
    case OutputFormat::MovProRes:
        codec = QStringLiteral("ProRes 4444");
        break;
    case OutputFormat::WebmVp9:
        codec = QStringLiteral("VP9");
        break;
    }
    return alpha ? codec + QStringLiteral(" alpha") : codec;
}

bool formatHasAlpha(OutputFormat f) {
    return f != OutputFormat::Mp4H264;
}

int formatMaxFps(OutputFormat f) {
    return f == OutputFormat::Gif ? 50 : 60;
}

} // namespace avloop
