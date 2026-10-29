#include "core/Project.h"

#include "core/Commands.h"
#include "core/Geometry.h"
#include "core/ProjectJson.h"
#include "core/SequenceView.h"
#include "core/Timecode.h"

#include <QFileInfo>

#include <algorithm>

namespace avloop {

namespace sv = sequenceview;

namespace {

bool sameSource(const SourceInfo& a, const SourceInfo& b) {
    return a.path == b.path && a.width == b.width && a.height == b.height && a.duration == b.duration &&
           a.fps == b.fps && a.vfr == b.vfr && a.hasAlpha == b.hasAlpha;
}

} // namespace

Project::Project(QObject* parent) : QObject(parent) {
    undo_.setUndoLimit(kUndoLimit);
}

SourceInfo Project::mediaOf(const QString& path) const {
    for (const SourceInfo& m : media_)
        if (m.path == path) return m;
    return {};
}

void Project::addMedia(const SourceInfo& info) {
    for (SourceInfo& m : media_)
        if (m.path == info.path) {
            m = info;
            emit changed(Aspect::Media);
            return;
        }
    media_.append(info);
    emit changed(Aspect::Media);
}

void Project::setSource(const SourceInfo& source) {
    // A new document: the source the bin's first, one clip of it on V1 at its initial place.
    media_ = {source};
    Sequence s;
    s.tracks.append({QStringLiteral("V1"), true, false, {}});
    {
        Clip c; // V1 always holds the main clip (without a path: a placeholder the views still edit)
        c.id = s.nextId++;
        c.name = QFileInfo(source.path).completeBaseName();
        c.path = source.path;
        c.size = source.size();
        c.duration = source.duration;
        c.range = {0, source.duration};
        c.transform = VideoTransform::initial(source.size());
        s.tracks[0].clips.append(c);
        s.main = c.id;
    }
    s.inOut = timerange::whole(source.duration, output_.effectiveFps(source.fps, source.vfr));
    sv::settle(s);
    seq_ = s;
    selectedShapes_.clear();
    selectedLayer_ = -1;
    undo_.clear();
    sync(true);
}

QList<int> Project::selectedShapes() const {
    QList<int> valid;
    for (int i : selectedShapes_)
        if (i >= 0 && i < crop_.shapes.size() && !valid.contains(i)) valid.append(i);
    std::sort(valid.begin(), valid.end());
    return valid;
}

void Project::setSelectedShapes(const QList<int>& indices) {
    if (indices == selectedShapes_) return;
    selectedShapes_ = indices;
    emit changed(Aspect::Shapes);
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
    d.layers = layers_;
    d.sequence = seq_;
    d.media = media_;
    d.view = view_;
    d.exportPath = exportPath_;
    d.original = extra_;
    return d;
}

void Project::restore(const ProjectData& data) {
    loop_ = data.loop.clamped();
    output_ = data.output;
    quality_ = data.quality;
    media_ = data.media;
    if (!data.source.path.isEmpty()) {
        // The source probed: the bin's entry for it replaced, or the bin's first.
        const auto it =
            std::find_if(media_.begin(), media_.end(), [&](const SourceInfo& m) { return m.path == data.source.path; });
        if (it != media_.end())
            *it = data.source;
        else
            media_.prepend(data.source);
    }
    // A file of version 5 holds its sequence; an older one is made into one (RS-5).
    Sequence s = data.sequence.tracks.isEmpty() ? Sequence::fromAvLoop(data) : data.sequence;
    s.matte = s.matte.clamped();
    if (Clip* m = sv::mainClip(s)) {
        m->effects = m->effects.clamped();
        s.inOut = timerange::valid(s.inOut, std::max(data.source.duration, s.duration()),
                                   output_.effectiveFps(data.source.fps, data.source.vfr));
    }
    sv::settle(s);
    seq_ = s;
    selectedShapes_.clear();
    selectedLayer_ = -1;
    view_ = data.view;
    exportPath_ = data.exportPath;
    extra_ = data.original;
    undo_.clear();
    sync(true);
    emit changed(Aspect::Loop);
    emit changed(Aspect::Output);
    emit changed(Aspect::Quality);
    emit changed(Aspect::View);
}

void Project::sync(bool all) {
    const Clip* m = sv::mainClip(seq_);
    SourceInfo source = m ? mediaOf(m->path) : SourceInfo{};
    if (m && source.path.isEmpty()) {
        // A clip whose media is not in the bin: what the clip knows of it.
        source.path = m->path;
        source.width = int(m->size.width());
        source.height = int(m->size.height());
        source.duration = m->duration;
    }
    const Keyframes keys = m ? sv::keyframesOf(*m) : Keyframes{};
    const VideoTransform base = m ? m->transform : VideoTransform{};
    const ClipEffects effects = m ? m->effects : ClipEffects{};
    const QList<VideoLayer> layers = sv::layersOf(seq_);
    const bool sourceChanged = !sameSource(source, source_);
    const bool rangeChanged = !(seq_.inOut == range_);
    const bool keysChanged = !(keys == keyframes_);
    const bool baseChanged = !(base == transform_);
    const bool cropChanged = !(seq_.matte == crop_);
    const bool effectsChanged = !(effects == effects_);
    const bool layersChanged = !(layers == layers_);
    source_ = source;
    range_ = seq_.inOut;
    keyframes_ = keys;
    transform_ = base;
    crop_ = seq_.matte;
    effects_ = effects;
    layers_ = layers;
    const VideoTransform now = transformAt(time_);
    const bool currentChanged = !(now == current_);
    current_ = now;
    if (selectedLayer_ >= layers_.size()) selectedLayer_ = -1;
    if (all || sourceChanged) emit changed(Aspect::Source);
    if (all || rangeChanged) emit changed(Aspect::Range);
    if (all || keysChanged) emit changed(Aspect::Keyframes);
    if (all || baseChanged || currentChanged) emit changed(Aspect::Transform);
    if (all || cropChanged) emit changed(Aspect::Crop);
    if (all || effectsChanged) emit changed(Aspect::Effects);
    if (all || layersChanged) emit changed(Aspect::Layers);
    emit changed(Aspect::Sequence);
}

double Project::length() const {
    return std::max(source_.duration, seq_.duration());
}

bool Project::avloopShaped() const {
    if (seq_.tracks.isEmpty()) return true;
    const QList<Clip>& v1 = seq_.tracks[0].clips;
    if (v1.size() > 1 || (v1.size() == 1 && (v1[0].start != 0 || v1[0].range.in != 0))) return false;
    for (const Track& t : seq_.tracks)
        for (const Clip& c : t.clips)
            if (!c.masks.isEmpty()) return false;
    return true;
}

QRectF Project::exportFrame() const {
    // The matte's rectangle, else the main clip where it stands at In (sequenceview::frameOf, kept with every edit).
    return seq_.frame;
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

VideoTransform Project::transformAt(double seconds) const {
    const Clip* m = sv::mainClip(seq_);
    if (!m) return transform_;
    return m->look(m->range.in + (seconds - m->start)).transform;
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

Rational Project::outputFps() const {
    return output_.effectiveFps(source_.fps, source_.vfr);
}

void Project::setSelectedLayer(int index) {
    const int valid = index >= 0 && index < layers_.size() ? index : -1;
    if (valid == selectedLayer_) return;
    selectedLayer_ = valid;
    emit changed(Aspect::LayerSelection);
}

const ClipEffects& Project::panelEffects() const {
    const int i = selectedLayer();
    return i >= 0 ? layers_[i].effects : effects_;
}

void Project::applySequence(const Sequence& s) {
    seq_ = s;
    sync(false);
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

} // namespace avloop
