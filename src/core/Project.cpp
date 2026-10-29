#include "core/Project.h"

#include "core/Commands.h"
#include "core/Geometry.h"
#include "core/ProjectJson.h"
#include "core/Timecode.h"

#include <algorithm>

namespace avloop {

Project::Project(QObject* parent) : QObject(parent) {
    undo_.setUndoLimit(kUndoLimit);
}

void Project::setSource(const SourceInfo& source) {
    source_ = source;
    range_ = timerange::whole(source.duration, outputFps());
    transform_ = VideoTransform::initial(source.size());
    keyframes_.clear();
    current_ = transform_;
    crop_ = CropShape{};
    effects_ = ClipEffects{};
    undo_.clear();
    emit changed(Aspect::Source);
    emit changed(Aspect::Range);
    emit changed(Aspect::Transform);
    emit changed(Aspect::Crop);
    emit changed(Aspect::Effects);
}

ProjectData Project::snapshot() const {
    ProjectData d;
    d.source = source_;
    d.range = range_;
    d.loop = loop_;
    d.output = output_;
    d.quality = quality_;
    d.transform = transform_;
    d.keyframes = keyframes_;
    d.crop = crop_;
    d.effects = effects_;
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
    keyframes_ = data.keyframes;
    current_ = transformAt(time_);
    crop_ = data.crop.clamped();
    effects_ = data.effects.clamped();
    view_ = data.view;
    exportPath_ = data.exportPath;
    extra_ = data.original;
    undo_.clear();
    emit changed(Aspect::Source);
    emit changed(Aspect::Range);
    emit changed(Aspect::Transform);
    emit changed(Aspect::Keyframes);
    emit changed(Aspect::Crop);
    emit changed(Aspect::Loop);
    emit changed(Aspect::Output);
    emit changed(Aspect::Quality);
    emit changed(Aspect::Effects);
    emit changed(Aspect::View);
}

QRectF Project::exportFrame() const {
    // Without a crop the video's own rectangle, where it stands at In (the crop itself is never keyframed, §1.3).
    return geometry::exportFrame(crop_, transformAt(range_.in), source_.size());
}

QSize Project::outputSize(bool* limited) const {
    return output_.sizeFor(exportFrame().size(), limited);
}

bool Project::outputAlpha() const {
    using A = OutputSettings::Alpha;
    if (!formatHasAlpha(output_.format)) return false; // MP4: the Background under
    if (output_.alpha != A::Auto) return output_.alpha == A::On;
    if (effects_.on && effects_.opacity < 100) return true; // the clip made see-through
    // Auto: alpha when the video leaves the output partly clear at any keyframe (between them it never goes further).
    for (const VideoTransform& t : keyframes::extremes(keyframes_, transform_))
        if (geometry::needsAlpha(crop_, t, source_.size(), source_.hasAlpha)) return true;
    return false;
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
    if (keyframes_.isEmpty()) {
        if (target == transform_) return;
        undo_.push(new SetTransformCommand(
            SetTransformId, tr("Move or scale the video"), transform_, target,
            [this](const VideoTransform& t) { applyTransform(t); }, mergeKey));
        return;
    }
    // With keyframes: the flips on the one transform (they hold), the position and scale into the keyframe at the
    // playhead — made there if there is none (spec §6.2).
    if (target == current_) return;
    const bool flips = target.flipH != transform_.flipH || target.flipV != transform_.flipV;
    const bool moved = target.x != current_.x || target.y != current_.y || target.scale != current_.scale;
    if (flips && moved) undo_.beginMacro(tr("Move or scale the video"));
    if (flips) {
        VideoTransform base = transform_;
        base.flipH = target.flipH;
        base.flipV = target.flipV;
        undo_.push(new SetTransformCommand(
            SetTransformId, tr("Flip the video"), transform_, base,
            [this](const VideoTransform& t) { applyTransform(t); }, mergeKey));
    }
    if (moved) {
        const int at = keyframeAtTime();
        const Keyframe k{time_, target.x, target.y, target.scale, at >= 0 && keyframes_[at].ease};
        pushKeyframes(AddKeyframeId, tr("Set a keyframe"), keyframes::withKey(keyframes_, k, keyTolerance()), mergeKey);
    }
    if (flips && moved) undo_.endMacro();
}

VideoTransform Project::transformAt(double seconds) const {
    return keyframes::valueAt(keyframes_, transform_, seconds);
}

void Project::setTime(double seconds) {
    if (seconds == time_) return;
    time_ = seconds;
    if (keyframes_.isEmpty()) return;
    // With keyframes the playhead always says so: the keyframe at it may change even where the transform does not.
    current_ = transformAt(time_);
    emit changed(Aspect::Time);
}

double Project::keyTolerance() const {
    return 0.5 / std::max(1.0, outputFps().value());
}

int Project::keyframeAtTime() const {
    return keyframes::indexAt(keyframes_, time_, keyTolerance());
}

void Project::addKeyframe() {
    if (!source_.isValid()) return;
    const int at = keyframeAtTime();
    const double t = at >= 0 ? keyframes_[at].t : time_;
    const Keyframe k{t, current_.x, current_.y, current_.scale, at >= 0 && keyframes_[at].ease};
    pushKeyframes(AddKeyframeId, tr("Add a keyframe"), keyframes::withKey(keyframes_, k, keyTolerance()));
}

void Project::deleteKeyframe(int index) {
    if (index < 0 || index >= keyframes_.size()) return;
    Keyframes after = keyframes_;
    after.removeAt(index);
    if (!after.isEmpty()) {
        pushKeyframes(DeleteKeyframeId, tr("Delete a keyframe"), after);
        return;
    }
    // The last one: the video stays where it is shown (the one transform takes its value), as After Effects does.
    undo_.beginMacro(tr("Delete a keyframe"));
    undo_.push(new SetTransformCommand(
        SetTransformId, tr("Delete a keyframe"), transform_, current_,
        [this](const VideoTransform& t) { applyTransform(t); }, QString()));
    pushKeyframes(DeleteKeyframeId, tr("Delete a keyframe"), after);
    undo_.endMacro();
}

void Project::moveKeyframe(int index, double seconds, const QString& mergeKey) {
    const Keyframes after = keyframes::moved(keyframes_, index, std::max(0.0, seconds), keyTolerance());
    pushKeyframes(MoveKeyframeId, tr("Move a keyframe"), after, mergeKey);
}

void Project::copyKeyframe(int index, double seconds, const QString& mergeKey) {
    // A move's id: the copy's moves later in the same drag merge into this step.
    const Keyframes after = keyframes::copied(keyframes_, index, std::max(0.0, seconds), keyTolerance());
    pushKeyframes(MoveKeyframeId, tr("Copy a keyframe"), after, mergeKey);
}

void Project::setKeyframeEase(int index, bool ease) {
    if (index < 0 || index >= keyframes_.size() || keyframes_[index].ease == ease) return;
    Keyframes after = keyframes_;
    after[index].ease = ease;
    pushKeyframes(AddKeyframeId, ease ? tr("Ease a keyframe") : tr("Make a keyframe linear"), after);
}

void Project::pushKeyframes(int id, const QString& text, const Keyframes& after, const QString& mergeKey) {
    if (after == keyframes_) return;
    undo_.push(new SetKeyframesCommand(
        id, text, keyframes_, after, [this](const Keyframes& k) { applyKeyframes(k); }, mergeKey));
}

void Project::applyKeyframes(const Keyframes& k) {
    keyframes_ = k;
    emit changed(Aspect::Keyframes);
    refreshCurrent(Aspect::Transform);
}

void Project::refreshCurrent(Aspect aspect) {
    const VideoTransform now = transformAt(time_);
    if (now == current_) return;
    current_ = now;
    emit changed(aspect);
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

void Project::changeSettings(const LoopSettings& loop, const OutputSettings& output, const QualitySettings& quality,
                             const QString& name) {
    if (loop.clamped() == loop_ && output == output_ && quality == quality_) return;
    undo_.beginMacro(tr("Apply the preset %1").arg(name));
    changeLoop(loop);
    changeOutput(output);
    changeQuality(quality);
    undo_.endMacro();
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

void Project::changeEffects(const ClipEffects& after, const QString& mergeKey) {
    const ClipEffects target = after.clamped();
    if (target == effects_) return;
    undo_.push(new SetEffectsCommand(
        SetEffectsId, tr("Change the clip effects"), effects_, target,
        [this](const ClipEffects& e) { applyEffects(e); }, mergeKey));
}

void Project::applyEffects(const ClipEffects& e) {
    effects_ = e;
    emit changed(Aspect::Effects);
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
    current_ = transformAt(time_);
    emit changed(Aspect::Transform);
}

} // namespace avloop
