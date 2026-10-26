// The Loop, Output and Quality sections of a project file (spec §11.1), on their own so a preset (§11.3) writes and
// reads the same keys. A key missing on read takes its default; keys are never renamed (§2.7).
#include "core/ProjectJson.h"

#include <QColor>
#include <QSize>

#include <algorithm>

namespace avloop::projectjson {

QJsonObject loopToJson(const LoopSettings& l) {
    return {
        {QStringLiteral("mode"), LoopSettings::modeId(l.mode)},
        {QStringLiteral("fade"), l.fade},
        {QStringLiteral("curve"), LoopSettings::curveId(l.curve)},
        {QStringLiteral("turn"), l.turn},
        {QStringLiteral("speed"), l.speed},
        {QStringLiteral("fpsMethod"), LoopSettings::methodId(l.method)},
    };
}

LoopSettings loopFromJson(const QJsonObject& l) {
    LoopSettings loop;
    if (l.contains(QStringLiteral("mode")))
        loop.mode = LoopSettings::modeFromId(l.value(QStringLiteral("mode")).toString());
    loop.fade = l.value(QStringLiteral("fade")).toDouble(loop.fade);
    if (l.contains(QStringLiteral("curve")))
        loop.curve = LoopSettings::curveFromId(l.value(QStringLiteral("curve")).toString());
    loop.turn = l.value(QStringLiteral("turn")).toDouble(loop.turn);
    loop.speed = l.value(QStringLiteral("speed")).toDouble(loop.speed);
    loop.method = LoopSettings::methodFromId(l.value(QStringLiteral("fpsMethod")).toString());
    return loop.clamped();
}

QJsonObject outputToJson(const OutputSettings& o) {
    return {
        {QStringLiteral("format"), formatId(o.format)}, {QStringLiteral("size"), o.sizeId()},
        {QStringLiteral("w"), o.custom.width()},        {QStringLiteral("h"), o.custom.height()},
        {QStringLiteral("lock"), o.lockRatio},          {QStringLiteral("alpha"), OutputSettings::alphaId(o.alpha)},
        {QStringLiteral("background"), o.background},   {QStringLiteral("fps"), o.fpsId()},
    };
}

OutputSettings outputFromJson(const QJsonObject& o) {
    OutputSettings out = OutputSettings::fromFpsId(o.value(QStringLiteral("fps")).toString());
    out.format = formatFromId(o.value(QStringLiteral("format")).toString());
    out.setSizeId(o.value(QStringLiteral("size")).toString());
    out.custom = QSize(std::clamp(o.value(QStringLiteral("w")).toInt(640), 16, 9999),
                       std::clamp(o.value(QStringLiteral("h")).toInt(640), 16, 9999));
    out.lockRatio = o.value(QStringLiteral("lock")).toBool(true);
    out.alpha = OutputSettings::alphaFromId(o.value(QStringLiteral("alpha")).toString());
    const QString background = o.value(QStringLiteral("background")).toString();
    out.background = QColor::isValidColorName(background) ? QColor(background).name() : QStringLiteral("#000000");
    return out;
}

QJsonObject qualityToJson(const QualitySettings& q) {
    return {
        {QStringLiteral("preset"), QualitySettings::presetId(q.preset)},
        {QStringLiteral("crf"), q.crf},
        {QStringLiteral("speed"), q.speed},
        {QStringLiteral("maxBitrate"), q.maxBitrate},
        {QStringLiteral("twoPass"), q.twoPass},
        {QStringLiteral("targetMB"), q.targetMB},
    };
}

QualitySettings qualityFromJson(const QJsonObject& q) {
    QualitySettings quality;
    if (q.contains(QStringLiteral("preset")))
        quality.preset = QualitySettings::presetFromId(q.value(QStringLiteral("preset")).toString());
    quality.crf = q.value(QStringLiteral("crf")).toInt(quality.crf);
    quality.speed = q.value(QStringLiteral("speed")).toInt(quality.speed);
    quality.maxBitrate = q.value(QStringLiteral("maxBitrate")).toInt(0);
    quality.twoPass = q.value(QStringLiteral("twoPass")).toBool(false);
    quality.targetMB = q.value(QStringLiteral("targetMB")).toDouble(0);
    return quality;
}

} // namespace avloop::projectjson
