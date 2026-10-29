#include "core/SequenceEdit.h"

#include <algorithm>

namespace avloop::sequenceedit {

namespace {

void sortTrack(Track& t) {
    std::sort(t.clips.begin(), t.clips.end(), [](const Clip& a, const Clip& b) { return a.start < b.start; });
}

bool editable(const Sequence& s, int track) {
    return track >= 0 && track < s.tracks.size() && !s.tracks[track].locked;
}

} // namespace

Where find(const Sequence& s, int id) {
    for (qsizetype t = 0; t < s.tracks.size(); ++t)
        for (qsizetype i = 0; i < s.tracks[t].clips.size(); ++i)
            if (s.tracks[t].clips[i].id == id) return {int(t), int(i)};
    return {};
}

const Clip* clip(const Sequence& s, int id) {
    const Where w = find(s, id);
    return w.valid() ? &s.tracks[w.track].clips[w.index] : nullptr;
}

bool fits(const Sequence& s, int track, double start, double length, int except) {
    if (track < 0 || track >= s.tracks.size() || start < -1e-9 || length < kMinLength - 1e-12) return false;
    for (const Clip& c : s.tracks[track].clips)
        if (c.id != except && start < c.end() - 1e-9 && start + length > c.start + 1e-9) return false;
    return true;
}

const Clip* clipAt(const Sequence& s, int track, double t) {
    if (track < 0 || track >= s.tracks.size()) return nullptr;
    for (const Clip& c : s.tracks[track].clips)
        if (c.ownTime(t, nullptr)) return &c;
    return nullptr;
}

int topClipAt(const Sequence& s, const QPointF& scene, double t) {
    for (qsizetype track = s.tracks.size() - 1; track >= 0; --track) {
        const Track& tr = s.tracks[track];
        if (!tr.visible || tr.locked) continue;
        double own = 0;
        for (const Clip& c : tr.clips)
            if (c.ownTime(t, &own) && c.rectAt(own).contains(scene)) return c.id;
    }
    return 0;
}

bool split(Sequence& s, int id, double t, int* made) {
    const Where w = find(s, id);
    if (!w.valid() || !editable(s, w.track) || s.clipCount() >= Sequence::kMaxClips) return false;
    Clip& left = s.tracks[w.track].clips[w.index];
    if (t <= left.start + kMinLength || t >= left.end() - kMinLength) return false;
    Clip right = left;
    right.id = s.nextId++;
    right.start = t;
    right.range.in = left.range.in + (t - left.start);
    left.range.out = right.range.in;
    s.tracks[w.track].clips.insert(w.index + 1, right);
    if (made) *made = right.id;
    return true;
}

int splitAll(Sequence& s, double t) {
    QList<int> under;
    for (qsizetype track = 0; track < s.tracks.size(); ++track)
        if (const Clip* c = clipAt(s, int(track), t); c && !s.tracks[track].locked) under << c->id;
    int cuts = 0;
    for (int id : under)
        cuts += split(s, id, t) ? 1 : 0;
    return cuts;
}

bool trimIn(Sequence& s, int id, double start) {
    const Where w = find(s, id);
    if (!w.valid() || !editable(s, w.track)) return false;
    Clip c = s.tracks[w.track].clips[w.index];
    const double in = c.range.in + (start - c.start);
    if (in < -1e-9 || c.range.out - in < kMinLength) return false;
    c.range.in = std::max(0.0, in);
    c.start = start;
    if (!fits(s, w.track, c.start, c.length(), id)) return false;
    s.tracks[w.track].clips[w.index] = c;
    return true;
}

bool trimOut(Sequence& s, int id, double end) {
    const Where w = find(s, id);
    if (!w.valid() || !editable(s, w.track)) return false;
    Clip c = s.tracks[w.track].clips[w.index];
    const double out = c.range.in + (end - c.start);
    if (out > c.duration + 1e-9 || out - c.range.in < kMinLength) return false;
    c.range.out = std::min(out, c.duration);
    if (!fits(s, w.track, c.start, c.length(), id)) return false;
    s.tracks[w.track].clips[w.index] = c;
    return true;
}

bool move(Sequence& s, int id, double start, int track) {
    const Where w = find(s, id);
    if (!w.valid() || !editable(s, w.track) || !editable(s, track)) return false;
    Clip c = s.tracks[w.track].clips[w.index];
    if (!fits(s, track, start, c.length(), id)) return false;
    c.start = start;
    s.tracks[w.track].clips.removeAt(w.index);
    s.tracks[track].clips.append(c);
    sortTrack(s.tracks[track]);
    return true;
}

bool remove(Sequence& s, int id) {
    const Where w = find(s, id);
    if (!w.valid() || !editable(s, w.track)) return false;
    s.tracks[w.track].clips.removeAt(w.index);
    return true;
}

bool rippleRemove(Sequence& s, int id) {
    const Where w = find(s, id);
    if (!w.valid() || !editable(s, w.track)) return false;
    const Clip gone = s.tracks[w.track].clips[w.index];
    s.tracks[w.track].clips.removeAt(w.index);
    for (Clip& c : s.tracks[w.track].clips)
        if (c.start >= gone.end() - 1e-9) c.start -= gone.length();
    return true;
}

bool insert(Sequence& s, int track, double t, Clip c, int* made) {
    if (!editable(s, track) || s.clipCount() >= Sequence::kMaxClips || c.length() < kMinLength) return false;
    // A clip under t is cut there first, so the new one goes between its halves.
    if (const Clip* under = clipAt(s, track, t); under && t > under->start + kMinLength) split(s, under->id, t);
    for (Clip& other : s.tracks[track].clips)
        if (other.start >= t - 1e-9) other.start += c.length();
    c.id = s.nextId++;
    c.start = t;
    s.tracks[track].clips.append(c);
    sortTrack(s.tracks[track]);
    if (made) *made = c.id;
    return true;
}

bool overwrite(Sequence& s, int track, double t, Clip c, int* made) {
    if (!editable(s, track) || s.clipCount() >= Sequence::kMaxClips || c.length() < kMinLength) return false;
    const double from = t, to = t + c.length();
    // What it covers is cut away: the clips crossing its ends cut there, those inside removed.
    for (const double edge : {from, to})
        if (const Clip* under = clipAt(s, track, edge); under && edge > under->start + kMinLength)
            split(s, under->id, edge);
    QList<Clip>& clips = s.tracks[track].clips;
    clips.erase(std::remove_if(clips.begin(), clips.end(),
                               [&](const Clip& o) { return o.start >= from - 1e-9 && o.end() <= to + 1e-9; }),
                clips.end());
    c.id = s.nextId++;
    c.start = t;
    clips.append(c);
    sortTrack(s.tracks[track]);
    if (made) *made = c.id;
    return true;
}

bool addTrack(Sequence& s) {
    if (s.tracks.size() >= Sequence::kMaxTracks) return false;
    s.tracks.append({QStringLiteral("V%1").arg(s.tracks.size() + 1), true, false, {}});
    return true;
}

bool removeTrack(Sequence& s, int track) {
    if (track <= 0 || track >= s.tracks.size()) return false;
    s.tracks.removeAt(track);
    for (qsizetype i = 0; i < s.tracks.size(); ++i)
        if (s.tracks[i].name.startsWith(QLatin1Char('V'))) s.tracks[i].name = QStringLiteral("V%1").arg(i + 1);
    return true;
}

} // namespace avloop::sequenceedit
