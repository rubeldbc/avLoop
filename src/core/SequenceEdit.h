#pragma once

#include "core/Sequence.h"

#include <QList>
#include <QPointF>

namespace avloop {

// The editor's edits on a sequence (D-031), pure: each returns false and leaves the sequence as it was when refused —
// a clip never overlaps another on its track, never runs outside its media, is at least `kMinLength` long.
namespace sequenceedit {

constexpr double kMinLength = 1.0 / 60;

struct Where {
    int track = -1;
    int index = -1;
    bool valid() const { return track >= 0; }
};
Where find(const Sequence& s, int id);
const Clip* clip(const Sequence& s, int id);
// Whether `length` s from `start` on `track` is free (`except` a clip being moved).
bool fits(const Sequence& s, int track, double start, double length, int except = 0);
// The clip on `track` showing at `t`, else null; the topmost shown clip whose rectangle holds `scene` at `t`, else 0.
const Clip* clipAt(const Sequence& s, int track, double t);
int topClipAt(const Sequence& s, const QPointF& scene, double t);

// The Razor: clip `id` cut at sequence time `t` into two (the right one a new id, the same media, keys, effects and
// masks); `made` its id.
bool split(Sequence& s, int id, double t, int* made = nullptr);
// Every clip under `t` on the unlocked tracks cut there (Ctrl+K with nothing selected); the cuts made.
int splitAll(Sequence& s, double t);
// Its In moved to sequence time `start` (its start with it), or its Out to sequence time `end`.
bool trimIn(Sequence& s, int id, double start);
bool trimOut(Sequence& s, int id, double end);
// Moved to `start` on `track`.
bool move(Sequence& s, int id, double start, int track);
// Removed, leaving a gap; or rippled — the later clips on its track move left by its length.
bool remove(Sequence& s, int id);
bool rippleRemove(Sequence& s, int id);
// A new clip put on `track` at `t`: Insert pushes the later clips on that track right; Overwrite cuts away what it
// covers. Its id given here; false when the track is full or locked.
bool insert(Sequence& s, int track, double t, Clip c, int* made = nullptr);
bool overwrite(Sequence& s, int track, double t, Clip c, int* made = nullptr);
// A track added on top (at most 8); one removed with its clips (V1 always stays).
bool addTrack(Sequence& s);
bool removeTrack(Sequence& s, int track);

} // namespace sequenceedit

} // namespace avloop
