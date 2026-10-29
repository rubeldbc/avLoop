# 03 Shapes — the editor (items 1 … 8)

Canonical current execution state: `../AUTONOMOUS_STATE.md`

The owner's words of 2026-10-30 (`../README.md` §1). Readings `../READINGS.md` (RS-1 …).

## 1. The sequence (AS-001; `src/core/Sequence`, `SequenceEdit`, `SequenceJson`)

```
Sequence { QSize frame; Rational fps; TimeRange inOut; QList<Track> tracks; }          // V1 = tracks[0], at most 8
Track    { QString name; bool visible = true, locked = false; QList<Clip> clips; }     // sorted by start, never overlapping
Clip     { int id; QString name, path; QSizeF size; double duration;                   // the media
           double start; TimeRange range;                                               // where, and which part
           VideoTransform transform; QList<LayerKey> keys; double opacity = 100;        // motion and opacity, keyed
           ClipEffects effects; QList<MaskShape> masks; bool enabled = true; }          // effects; masks (holes)
```

- Scene space is the sequence frame's: (0, 0) its top-left, `frame` its size (RS-2). A clip's transform as AvLoop's
  (its centre, its scale), keyed by `LayerKey` over its own seconds (D-029's maths).
- Masks are AvLoop's `MaskShape` in the clip's own pixels (RS-3): Add, Cut (subtract), KeepOnly (intersect), Exclude
  (difference); a mask's `invert` turns it inside out — the hole; feather and strength (opacity) as D-024.
- Edits (`sequenceedit::`, pure, tested): `split(seq, track, at)`, `trimIn/trimOut`, `move(clip, dt, toTrack)` refused
  over another clip, `remove` (gap), `rippleRemove` (later clips move left), `insert`/`overwrite` a clip at a time,
  `clipAt(seq, track, t)`, `topClipAt(seq, scene, t)`. A clip's own time at sequence t: `range.in + (t − start)`.
- Undo: one `SetSequence` value command a gesture (a drag's moves merge; a typed field merges its own) (RS-4).
- The file, version 4 (RS-5): `"sequence": {frame, fps, in, out, tracks: [{name, visible, locked, clips: [...]}]}`; the
  loop, output and quality sections as AvLoop's. `ProjectMigrate` 3 → 4: the main video → V1's clip (its transform,
  keyframes as keys, effects; the crop's shapes → its masks; an inverted crop → an inverted mask), the layers → clips
  on V2 …; the crop's rectangle → the frame (none: the video's size); the range → In/Out.

## 2. The composite (AS-002; `src/render/SequenceRenderer`)

- Output frame k → (the loop engine over In/Out: LoopMap, TimeMap, Speed, rate) → sequence time(s) → for each shown
  track bottom to top, the clip present → its frame (a decoder pool keyed by file, at most 8 open, AVADR-0002) → its
  effects (EffectsPass) → its masks (MaskBuilder at its own size, cached by masks and size) → placed by its keyed
  motion → over the accumulator at its keyed opacity: acc = L·α_mask + acc·(1 − αL·α_mask). Empty: transparent.
- One CPU pass for every output (RS-6): the Preview, the export, the batch, `avloop-cli` — GPU = CPU by construction;
  the GL back end stays for the monitor's live look (RS-7). The memory walk flat.

## 3. The shell (AS-003)

- Layout (QSplitters, remembered): left the **Project panel** (media: import by dialog or drop, a list with thumbnail,
  name, size, length; drag one to a track or press Insert / Overwrite at the playhead); centre the **Program monitor**
  (the canvas: the composite at the playhead, AvLoop's zoom, hand, dim outside the frame); right the **Effect
  Controls** (AS-005); bottom the **Timeline** (AS-004). The top bar's Open, Preview, Export as AvLoop's.
- The **Frame** tool (F, AvLoop's crop tools re-aimed): the sequence frame's size and ratio (presets 1920 × 1080, 1080 ×
  1920, 1080 × 1080 …, a custom W × H); the clips keep their scene place.

## 4. The timeline editor (AS-004; `src/ui/timeline/SequenceTimeline*`)

Ruler and playhead (AvLoop's), the sequence In/Out (I, O), tracks top V8 … V1 bottom with headers (name, eye, lock;
right click Add track above, Remove), clip bars (name, thumbnails at the ends, selected outline). Selection tool (V):
click, Shift+click, marquee; drag moves (snap to the playhead, In/Out, clip edges; Ctrl off), across tracks refused
over another; edges trim; Alt+drag copies. Razor (C): a click splits the clip there; Ctrl+K splits every selected clip
(or all on unlocked tracks) at the playhead. Delete leaves a gap; Shift+Delete ripples. Zoom (+/−, Ctrl+wheel), scroll.

## 5. The Effect Controls (AS-005)

For the selected clip: Motion — Position X, Y, Scale (%); Opacity (%); Masks (one row each: mode, Inverted, feather,
opacity; add Rectangle, Ellipse, Pen); the clip effects (AvLoop's section's rows, now the clip's). Motion and Opacity
each a stopwatch: on, an edit at the playhead keys (D-029's maths); ◀ ◆ ▶ (previous, add/remove, next); a lane of
the clip's keys under the rows.

## 6. The masks on the monitor (AS-006)

The S and P tools draw masks on the selected clip (in its own pixels, its motion folded in); the handles, anchors,
Combine, Align of AvLoop's AV-024 … AV-029 over the clip's masks; Inverted makes the hole; the monitor dims what the
masks cut. The Selection on the monitor: a click picks the topmost clip at the playhead; its handles move and scale it.

## 7. The export (AS-007)

The Export window as AvLoop's: format, size (the frame's, a long side, custom), Alpha (Auto: on when anything is clear),
rate, quality, loop mode (None by default: the sequence as cut — RS-8). The written file checked as AvLoop's; the batch;
`avloop-cli export <project>`.

## 8. The project (AS-008)

Save, Save As, Open (any version), autosave and recovery, Recent — AvLoop's; the title "AvLoop by Rubel — <file>".

## Walks

Each phase its own (`execution/AS-NNN-walk/`): a GUI walk (`avloop.exe --walk`) and a render walk (`avloop-cli`
frames, ffprobe, the alpha read back), as AvLoop's lab (`control/lab`, `AVLOOP_BIN_DIR`).
