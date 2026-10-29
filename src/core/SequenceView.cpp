#include "core/SequenceView.h"

#include "core/Geometry.h"

#include <QSet>

#include <functional>

#include <algorithm>

namespace avloop::sequenceview {

namespace {

void sortTrack(Track& t) {
    std::sort(t.clips.begin(), t.clips.end(), [](const Clip& a, const Clip& b) { return a.start < b.start; });
}

Clip clipOf(const VideoLayer& l, const Clip& was) {
    Clip c = was;
    c.name = l.name;
    c.path = l.path;
    c.size = l.size;
    c.duration = l.duration;
    c.start = l.start;
    c.range = l.range;
    c.transform = l.transform;
    c.keys = l.keys;
    c.opacity = l.opacity;
    c.effects = l.effects;
    c.enabled = l.visible;
    c.locked = l.locked;
    return c;
}

} // namespace

const Clip* mainClip(const Sequence& s) {
    if (s.tracks.isEmpty() || s.tracks[0].clips.isEmpty()) return nullptr;
    for (const Clip& c : s.tracks[0].clips)
        if (c.id == s.main) return &c;
    return &s.tracks[0].clips.first();
}

Clip* mainClip(Sequence& s) {
    // Through the lists' own non-const access, so a copy detaches from the sequence it was copied from (never a
    // const_cast into shared data: the original would change with it).
    if (s.tracks.isEmpty() || s.tracks[0].clips.isEmpty()) return nullptr;
    QList<Clip>& v1 = s.tracks[0].clips;
    for (Clip& c : v1)
        if (c.id == s.main) return &c;
    return &v1.first();
}

QRectF frameOf(const Sequence& s) {
    const Clip* m = mainClip(s);
    if (!m) return s.matte.isEmpty() || s.matte.invert ? s.frame : s.matte.rect;
    return geometry::exportFrame(s.matte, m->look(m->range.in + std::max(0.0, s.inOut.in - m->start)).transform,
                                 m->size);
}

void settle(Sequence& s) {
    const Clip* m = mainClip(s);
    s.main = m ? m->id : 0;
    s.frame = frameOf(s);
}

Keyframes keyframesOf(const Clip& c) {
    Keyframes keys;
    for (const LayerKey& k : c.keys)
        keys.append({c.start + (k.t - c.range.in), k.x, k.y, k.scale, k.ease});
    return keys;
}

void setKeyframes(Clip& c, const Keyframes& keys) {
    QList<LayerKey> next;
    for (const Keyframe& k : keys) {
        const double own = c.range.in + (k.t - c.start);
        double opacity = c.look(own).opacity;
        for (const LayerKey& was : c.keys)
            if (std::abs(was.t - own) < 1e-6) opacity = was.opacity;
        next.append({own, k.x, k.y, k.scale, opacity, k.ease});
    }
    c.keys = next;
}

QList<VideoLayer> layersOf(const Sequence& s) {
    QList<VideoLayer> layers;
    for (qsizetype t = 1; t < s.tracks.size(); ++t)
        for (const Clip& c : s.tracks[t].clips) {
            VideoLayer l;
            l.name = c.name;
            l.path = c.path;
            l.size = c.size;
            l.duration = c.duration;
            l.track = int(t) + 1;
            l.start = c.start;
            l.range = c.range;
            l.transform = c.transform;
            l.opacity = c.opacity;
            l.visible = c.enabled;
            l.locked = c.locked;
            l.keys = c.keys;
            l.effects = c.effects;
            l.id = c.id;
            layers.append(l);
        }
    std::sort(layers.begin(), layers.end(), [](const VideoLayer& a, const VideoLayer& b) { return a.id < b.id; });
    return layers;
}

void setLayers(Sequence& s, const QList<VideoLayer>& layers) {
    // The clips above V1 as they were, by id; then emptied, and filled again from the layers.
    QHash<int, Clip> was;
    for (qsizetype t = 1; t < s.tracks.size(); ++t) {
        for (const Clip& c : s.tracks[t].clips)
            was.insert(c.id, c);
        s.tracks[t].clips.clear();
    }
    QSet<int> seen;
    for (const VideoLayer& l : layers) {
        const bool known = l.id > 0 && was.contains(l.id) && !seen.contains(l.id);
        Clip c = clipOf(l, known ? was.value(l.id) : Clip{});
        c.id = known ? l.id : s.nextId++;
        seen.insert(c.id);
        const int index = std::clamp(l.track, 2, Sequence::kMaxTracks) - 1;
        while (s.tracks.size() <= index)
            s.tracks.append({QStringLiteral("V%1").arg(s.tracks.size() + 1), true, false, {}});
        s.tracks[index].clips.append(c);
    }
    for (qsizetype t = 1; t < s.tracks.size(); ++t)
        sortTrack(s.tracks[t]);
}

namespace {

QList<MaskShape> mapped(const QList<MaskShape>& masks, const std::function<QPointF(const QPointF&)>& map, double k) {
    QList<MaskShape> out;
    for (MaskShape m : masks) {
        const QPointF a = map(m.rect.topLeft());
        m.rect = QRectF(a, m.rect.size() * k);
        m.radius *= k;
        for (PathPoint& pt : m.points) {
            pt.p = map(pt.p);
            pt.in = map(pt.in);
            pt.out = map(pt.out);
        }
        out.append(m);
    }
    return out;
}

} // namespace

QList<MaskShape> masksToScene(const Clip& c, double own) {
    const QRectF r = c.rectAt(own);
    const double k = c.size.width() > 0 ? r.width() / c.size.width() : 1.0;
    return mapped(c.masks, [&](const QPointF& p) { return r.topLeft() + p * k; }, k);
}

QList<MaskShape> masksFromScene(const Clip& c, double own, const QList<MaskShape>& scene) {
    const QRectF r = c.rectAt(own);
    const double k = c.size.width() > 0 && r.width() > 0 ? r.width() / c.size.width() : 1.0;
    return mapped(scene, [&](const QPointF& p) { return (p - r.topLeft()) / k; }, 1.0 / k);
}

} // namespace avloop::sequenceview
