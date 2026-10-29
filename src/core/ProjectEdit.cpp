// The project's edits (spec §11.2): each one step over the whole sequence (D-033) — the main clip's transform and keys,
// the matte, In/Out, the effects, the layers, the sequence's own edits — and the Loop, Output and Quality sections.
#include "core/Commands.h"
#include "core/Project.h"
#include "core/SequenceView.h"

namespace avloop {

namespace sv = sequenceview;

void Project::pushSequence(int id, const QString& text, Sequence after, const QString& mergeKey) {
    sv::settle(after);
    if (after == seq_) return;
    undo_.push(
        new SetSequenceCommand(id, text, seq_, after, [this](const Sequence& s) { applySequence(s); }, mergeKey));
}

void Project::changeSequence(const Sequence& after, const QString& text, const QString& mergeKey) {
    pushSequence(SetSequenceId, text, after, mergeKey);
}

void Project::changeTransform(const VideoTransform& after, const QString& mergeKey) {
    const VideoTransform target = after.clamped();
    Sequence next = seq_;
    Clip* m = sv::mainClip(next);
    if (!m) return;
    if (keyframes_.isEmpty()) {
        if (target == transform_) return;
        m->transform = target;
        pushSequence(SetTransformId, tr("Move or scale the video"), next, mergeKey);
        return;
    }
    // With keyframes: the flips on the one transform (they hold), the position and scale into the keyframe at the
    // playhead — made there if there is none (spec §6.2). One step either way.
    if (target == current_) return;
    const bool flips = target.flipH != transform_.flipH || target.flipV != transform_.flipV;
    const bool moved = target.x != current_.x || target.y != current_.y || target.scale != current_.scale;
    m->transform.flipH = target.flipH;
    m->transform.flipV = target.flipV;
    if (moved) {
        const int at = keyframeAtTime();
        const Keyframe k{time_, target.x, target.y, target.scale, at >= 0 && keyframes_[at].ease};
        sv::setKeyframes(*m, keyframes::withKey(keyframes_, k, keyTolerance()));
    }
    const QString text = flips && moved ? tr("Move or scale the video")
                         : flips        ? tr("Flip the video")
                                        : tr("Set a keyframe");
    pushSequence(moved ? AddKeyframeId : SetTransformId, text, next, mergeKey);
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
    Sequence next = seq_;
    Clip* m = sv::mainClip(next);
    if (!m) return;
    m->transform = current_;
    m->keys.clear();
    pushSequence(DeleteKeyframeId, tr("Delete a keyframe"), next);
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
    Sequence next = seq_;
    Clip* m = sv::mainClip(next);
    if (!m) return;
    sv::setKeyframes(*m, after);
    pushSequence(id, text, next, mergeKey);
}

void Project::changeCrop(const CropShape& after, const QString& mergeKey) {
    const CropShape target = after.clamped();
    if (target == crop_) return;
    Sequence next = seq_;
    next.matte = target;
    pushSequence(SetCropId, tr("Change the crop"), next, mergeKey);
}

void Project::changeRange(const TimeRange& after, const QString& mergeKey) {
    if (after == range_) return;
    Sequence next = seq_;
    next.inOut = after;
    pushSequence(SetRangeId, tr("Change In and Out"), next, mergeKey);
}

void Project::changeEffects(const ClipEffects& after, const QString& mergeKey) {
    const ClipEffects target = after.clamped();
    if (target == effects_) return;
    Sequence next = seq_;
    Clip* m = sv::mainClip(next);
    if (!m) return;
    m->effects = target;
    pushSequence(SetEffectsId, tr("Change the clip effects"), next, mergeKey);
}

void Project::changeLayers(const QList<VideoLayer>& after, const QString& mergeKey) {
    if (after == layers_) return;
    Sequence next = seq_;
    sv::setLayers(next, after.mid(0, VideoLayer::kMaxLayers));
    pushSequence(SetLayersId, tr("Change the layers"), next, mergeKey);
}

void Project::changePanelEffects(const ClipEffects& after, const QString& mergeKey) {
    const int i = selectedLayer();
    if (i < 0) return changeEffects(after, mergeKey);
    QList<VideoLayer> l = layers_;
    l[i].effects = after.clamped();
    changeLayers(l, mergeKey);
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
        changeRange(timerange::valid(range_, length(), next));
        undo_.endMacro();
    }
}

void Project::changeQuality(const QualitySettings& after, const QString& mergeKey) {
    if (after == quality_) return;
    undo_.push(new SetQualityCommand(
        SetQualityId, tr("Change the quality"), quality_, after, [this](const QualitySettings& q) { applyQuality(q); },
        mergeKey));
}

} // namespace avloop
