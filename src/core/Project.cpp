#include "core/Project.h"

#include "core/Commands.h"
#include "core/Geometry.h"
#include "core/ProjectJson.h"
#include "core/Timecode.h"

namespace avloop {

Project::Project(QObject* parent) : QObject(parent) {
    undo_.setUndoLimit(kUndoLimit);
}

void Project::setSource(const SourceInfo& source) {
    source_ = source;
    range_ = timerange::whole(source.duration, outputFps());
    transform_ = VideoTransform::initial(source.size());
    crop_ = CropShape{};
    undo_.clear();
    emit changed(Aspect::Source);
    emit changed(Aspect::Range);
    emit changed(Aspect::Transform);
    emit changed(Aspect::Crop);
}

ProjectData Project::snapshot() const {
    ProjectData d;
    d.source = source_;
    d.range = range_;
    d.loop = loop_;
    d.output = output_;
    d.quality = quality_;
    d.transform = transform_;
    d.crop = crop_;
    d.view = view_;
    d.exportPath = exportPath_;
    d.original = extra_;
    return d;
}

void Project::restore(const ProjectData& data) {
    source_ = data.source;
    loop_ = data.loop.clamped();
    output_ = data.output;
    quality_ = data.quality;
    range_ = timerange::valid(data.range, source_.duration, outputFps());
    transform_ = data.transform;
    crop_ = data.crop.clamped();
    view_ = data.view;
    exportPath_ = data.exportPath;
    extra_ = data.original;
    undo_.clear();
    emit changed(Aspect::Source);
    emit changed(Aspect::Range);
    emit changed(Aspect::Transform);
    emit changed(Aspect::Crop);
    emit changed(Aspect::Loop);
    emit changed(Aspect::Output);
    emit changed(Aspect::Quality);
    emit changed(Aspect::View);
}

QRectF Project::exportFrame() const {
    return geometry::exportFrame(crop_.rect, transform_, source_.size());
}

QSize Project::outputSize(bool* limited) const {
    return output_.sizeFor(exportFrame().size(), limited);
}

bool Project::outputAlpha() const {
    using A = OutputSettings::Alpha;
    return output_.alpha == A::On ||
           (output_.alpha == A::Auto && geometry::needsAlpha(crop_, transform_, source_.size(), source_.hasAlpha));
}

void Project::setView(const ViewState& view) {
    view_ = view;
    emit changed(Aspect::View);
}

void Project::setExtra(const QJsonObject& extra) {
    extra_ = extra;
}

void Project::changeTransform(const VideoTransform& after, const QString& mergeKey) {
    const VideoTransform target = after.clamped();
    if (target == transform_) return;
    undo_.push(new SetTransformCommand(
        SetTransformId, tr("Move or scale the video"), transform_, target,
        [this](const VideoTransform& t) { applyTransform(t); }, mergeKey));
}

void Project::changeCrop(const CropShape& after, const QString& mergeKey) {
    const CropShape target = after.clamped();
    if (target == crop_) return;
    undo_.push(new SetCropCommand(
        SetCropId, tr("Change the crop"), crop_, target, [this](const CropShape& c) { applyCrop(c); }, mergeKey));
}

Rational Project::outputFps() const {
    return output_.effectiveFps(source_.fps, source_.vfr);
}

void Project::changeLoop(const LoopSettings& after, const QString& mergeKey) {
    const LoopSettings target = after.clamped();
    if (target == loop_) return;
    undo_.push(new SetLoopCommand(
        SetLoopId, tr("Change the loop"), loop_, target, [this](const LoopSettings& l) { applyLoop(l); }, mergeKey));
}

void Project::changeOutput(const OutputSettings& after, const QString& mergeKey) {
    if (after == output_) return;
    const Rational before = outputFps();
    const Rational next = after.effectiveFps(source_.fps, source_.vfr);
    const bool regrid = source_.isValid() && !(before == next);
    if (regrid) undo_.beginMacro(tr("Change the frame rate"));
    undo_.push(new SetOutputCommand(
        SetOutputId, tr("Change the output"), output_, after, [this](const OutputSettings& o) { applyOutput(o); },
        regrid ? QString() : mergeKey));
    if (regrid) {
        changeRange(timerange::valid(range_, source_.duration, next));
        undo_.endMacro();
    }
}

void Project::changeQuality(const QualitySettings& after, const QString& mergeKey) {
    if (after == quality_) return;
    undo_.push(new SetQualityCommand(
        SetQualityId, tr("Change the quality"), quality_, after, [this](const QualitySettings& q) { applyQuality(q); },
        mergeKey));
}

void Project::applyQuality(const QualitySettings& q) {
    quality_ = q;
    emit changed(Aspect::Quality);
}

void Project::applyLoop(const LoopSettings& l) {
    loop_ = l;
    emit changed(Aspect::Loop);
}

void Project::applyOutput(const OutputSettings& o) {
    output_ = o;
    emit changed(Aspect::Output);
}

void Project::changeRange(const TimeRange& after, const QString& mergeKey) {
    if (after == range_) return;
    undo_.push(new SetRangeCommand(
        SetRangeId, tr("Change In and Out"), range_, after, [this](const TimeRange& r) { applyRange(r); }, mergeKey));
}

void Project::applyRange(const TimeRange& r) {
    range_ = r;
    emit changed(Aspect::Range);
}

void Project::applyCrop(const CropShape& c) {
    crop_ = c;
    emit changed(Aspect::Crop);
}

void Project::applyTransform(const VideoTransform& t) {
    transform_ = t;
    emit changed(Aspect::Transform);
}

} // namespace avloop
