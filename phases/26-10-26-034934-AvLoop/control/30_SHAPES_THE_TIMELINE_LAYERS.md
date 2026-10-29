# 30 Shapes — the timeline layers (item 27, AV-030 and AV-031, late)

Canonical current execution state: `../AUTONOMOUS_STATE.md`

The owner's words of 2026-10-29 (`../README.md` §1, item 27, the last [1]): *"as well as make multiple layer system
in the time line like adobe premiere pro"*. D-027. Readings R-97 … R-103. Two phases: AV-030 the layers' model and
the composite; AV-031 the tracks on the timeline and the canvas.

## 1. The reading

A project becomes a **sequence**: video tracks V1 (bottom) … Vn, each holding clips; a **clip** is a source with a
place on the sequence (its start) and its own In/Out in source time, and everything the one video had — the transform
and its keyframes, the clip effects, the shapes. The sequence's In/Out (the range), the loop, the crop frame, the
output and the quality stay the sequence's: the loop is made of the composite (R-97). Where a clip is absent the
track is transparent; the composite is each track drawn over the one below (premultiplied, R-7); the crop's mask
and the shapes apply to the composite — the shapes move from the crop to the clip at this phase: each clip has its
own shapes (a mask a clip, as Premiere's) and the crop keeps the frame (R-98; the files of AV-023 … AV-029 on main
read `crop.shapes` into the first clip). "Like Premiere Pro": tracks, clip bars with names and thumbnails, drag to
move and to another track, trim at the ends, snap, the track header with the eye and the lock; no transitions,
audio, nesting, speed ramps or markers (not asked, R-99). At most 8 tracks and 64 clips (AVADR-0002).

## 2. The model (AV-030; `src/core/Sequence`, `Clip`, `Project`)

- `Clip { SourceInfo source; double start; TimeRange range; VideoTransform transform; Keyframes keyframes;
  ClipEffects effects; QList<MaskShape> shapes; QString name; bool visible, locked; }` and `Track { QList<Clip>
  clips; QString name; bool visible, locked; }`; `Project` holds the tracks and `selectedClip` (track, index; not in
  Undo, not in the file). The one-video API (`source()`, `transform()`, `effects()`, `changeTransform` …) reads and
  writes the **selected clip**; the panels and tools bind as they do (R-100). `Project.cpp` splits: `ProjectClips.cpp`.
- A clip's source time at sequence time t: in + (t − start); it shows for start ≤ t < start + (out − in). Keyframes
  stay in the clip's source time (D-011); the timeline draws them at their sequence time.
- The file, version 3: `"tracks": [ { "name", "visible", "locked", "clips": [ { "name", "start", "source", "range",
  "transform", "keyframes", "effects", "shapes", "visible", "locked" } ] } ]`; the keys inside a clip are the keys
  the top level had, unchanged. `ProjectMigrate` 2 → 3: the top-level `source`, `transform`, `keyframes`, `effects`
  become the one clip of V1 at start 0 with the project's `range`; the sequence's `range` stays at the top (R-101).
  A version-3 file of AV-023 … AV-029 (no `tracks`) is read the same way.
- Undo: `AddClip`, `RemoveClip`, `MoveClip` (start and track, merged within a drag), `TrimClip` (in/out and start),
  `AddTrack`, `RemoveTrack`, the track's eye/lock/name; one step each.
- Open: a video opened into an empty project is V1's clip (as today); "Add as layer…" (Ctrl+Shift+O, the ☰ menu,
  the drop zone's second half "Drop here to add a layer") puts a video on a new track above at the playhead; the
  batch's snapshot and avloop-cli read the whole.

## 3. The composite (AV-030; `src/render`)

- `RenderJob` carries the clips; `FrameRenderer::compose(k)` gathers, for each visible clip present at τ(k), its
  source frame through its own decoders (two lanes a clip, opened on first use, closed when the clip leaves the
  job; at most 8 clips' decoders open, the oldest closed first) with its EffectsPass, its placement, its shapes'
  mask at the output size (MaskBuilder a clip, cached); `Composite` becomes a list of layers bottom to top, each
  {frame, placement, mask, crossfade pair}; the crop's frame mask over the result.
- GlRenderer: the output pass draws the layers in order into the one FBO (blend premultiplied over); CpuRenderer the
  same loop; parity by construction (AVADR-0001). A frame's `key` takes the clip's id.
- Alpha Auto: on when any output pixel can be clear — no clip under the frame everywhere, a cut, a feather, an
  opacity (R-102).
- The thumbnails and the Loop Finder: the selected clip's source (R-69's rule a clip; R-103).

## 4. The timeline (AV-031; `src/ui/timeline`)

- Rows: the ruler; the tracks Vn … V1 from the top, each 48 px: a header (the name, the eye, the lock; a right-click
  menu Add track above/below, Remove track, Rename) and its lane; the keyframe row of the selected clip; the
  transport as before. The timeline grows to fit up to four tracks, then scrolls.
- A clip bar: the clip's name, its thumbnails (its own ThumbnailWorker; the budget of 400 shared, the selected clip
  first), a solid outline when selected. Drag moves it in time (snapping to In, Out, the playhead, other clips'
  edges within 8 px; Ctrl off); drag up/down moves it to another track (refused where it would overlap — a clip
  never overlaps another on its track); its ends trim (the source's limits; the start moves with the in-trim, as
  Premiere's); Alt+drag copies it (as a keyframe's, R-64); Delete removes it; a double-click selects it and takes
  the playhead to its start; the range's In/Out handles unchanged.
- The canvas: a click on the Selection tool selects the topmost visible clip under the pointer; the Selection, the
  Keyframe tool, the shapes and the panels follow the selected clip; the status bar's left says "V2 · name".
- The side panel: a "Clip" line above the sections (the selected clip's name and track; the eye and the lock).

## 5. Tests and walks

AV-030: `test-sequence` (new): a clip's source time, presence, the migration of a version-2 file, the overlap rule,
the json round trip; `test-renderer-cpu` / `test-renderer`: two clips over each other — the top's pixel where
present, the bottom's where not, a cut shape on the top showing the bottom; the loop's L unchanged; GPU = CPU within
2/255. `walk-layers-render.mjs`: avloop-cli frames of a two-clip project; a version-2 project byte for byte AV-020's.
AV-031 `walk-tracks.json`: Add as layer; the second track; the clip dragged in time (snap to Out), to V1 refused
over the first, trimmed at both ends; Alt+drag copy; the eye and the lock; the canvas selecting the top clip; the
Preview; Undo through it all; the keyframe row following the selection. `walk-tracks-memory.mjs`: three clips, the
memory walk flat (growth < 50 MB over the §3.7.8 cycle).
