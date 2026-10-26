#include "core/Project.h"

#include "core/Commands.h"
#include "core/ProjectJson.h"

namespace avloop {

Project::Project(QObject* parent) : QObject(parent) {
    undo_.setUndoLimit(kUndoLimit);
}

void Project::setSource(const SourceInfo& source) {
    source_ = source;
    transform_ = VideoTransform::initial(source.size());
    crop_ = CropShape{};
    undo_.clear();
    emit changed(Aspect::Source);
    emit changed(Aspect::Transform);
    emit changed(Aspect::Crop);
}

ProjectData Project::snapshot() const {
    ProjectData d;
    d.source = source_;
    d.transform = transform_;
    d.crop = crop_;
    d.view = view_;
    d.original = extra_;
    return d;
}

void Project::restore(const ProjectData& data) {
    source_ = data.source;
    transform_ = data.transform;
    crop_ = data.crop.clamped();
    view_ = data.view;
    extra_ = data.original;
    undo_.clear();
    emit changed(Aspect::Source);
    emit changed(Aspect::Transform);
    emit changed(Aspect::Crop);
    emit changed(Aspect::View);
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

void Project::applyCrop(const CropShape& c) {
    crop_ = c;
    emit changed(Aspect::Crop);
}

void Project::applyTransform(const VideoTransform& t) {
    transform_ = t;
    emit changed(Aspect::Transform);
}

} // namespace avloop
