# AV-031 — Execution Record

**Phase:** AV-031 — The timeline layers II — the tracks (owner item 27, late; with AV-030)
**Status:** DONE
**Date:** 2026-10-30
**Phase start commit:** `8bc4034` (*[AV-030] The timeline layers I: video layers and their composite*)
**ADR:** none owed (AVADR-0002: the memory walk with three layers flat)
**Environment class:** local-disposable (this PC: `build-lab/`, the samples, the walk's folder; the owner's AvLoop open
from `build\bin`, untouched)

## The owner's words

*"[1] as well as make multiple layer system in the time line like adobe premiere pro"*

## What was built

1. `src/ui/timeline/TrackLanes.{h,cpp}` (new) — the lanes on the timeline's time scale: bars, selection, a drag in
   time with snapping and across tracks with the overlap rule, trims at both ends, Alt+drag copies, Delete, a
   double-click seeks; the layout frozen under a drag; the walk mapper (seconds, track), probe `lanes.tracks`.
2. `src/ui/tools/SelectionTool.cpp` — a click on a layer at the playhead picks it, its drag moves it.
3. `src/ui/MainWindowLayers.cpp` — the lanes placed above the timeline; `src/ui/panels/LayersPanel.cpp` — the selected
   layer's timing probes.
4. Walks: `AV-031-walk/walk-tracks.json`, `walk-tracks-memory.mjs`.

## Decisions recorded

- **D-027** — the timeline layers, part II; spec §0.
- **R-104** the lanes' gestures; names, no thumbnails; eye and lock in the Layers section.

## Specification delta

- §0: `D-027` (part II). The lore: one rule (a layout frozen under a drag).

## Verification

| Command | Result |
| --- | --- |
| `scripts\build.ps1 -BuildDir build-lab` | built Release, 0 warnings |
| `ctest --test-dir build-lab --output-on-failure --timeout 300 -R avloop` | **25 passed**, 0 failed, 75 s |
| `scripts\format.ps1 -Check` · `check-icons` · `check-secrets` | PASS: 253 files formatted · PASS: 61 icons, 61 listed, avloop.ico with 7 sizes · PASS: 912 files, nothing secret or personal |
| `node control/phase-tools/check-loc.mjs` | PASS: 319 code files, none over 600 lines (the largest: src/core/ClipEffects.cpp, 425 lines) |
| `node control/phase-tools/validate-phase-system.mjs` | PASS: 31 phases, 34 requirement ids |
| README pictures | `readme-shots.json` by `build-lab`: 11 taken, the largest `main-window.png` 205 KB (the Layers header) |
| AV-029's CI; AV-030's and this phase's | green, 9 m 9 s (run 40930909417); AV-030 and AV-031 pushed together, one run on AV-031 (below) |

## The walk

```
AVLOOP_BIN_DIR=build-lab/bin node control/lab/walk-phase.mjs execution/AV-031-walk walk-tracks.json
  walk-tracks-memory.mjs → PHASE WALKS PASSED
walk-tracks.json (10 s, 26 PASS): the counter; a layer → one lane, V2; its right end dragged 9.98 → 6 s (out 6); its
  body 3 → 5 s (start 2); its left end 2 → 3 s (start 3, in 1); four Undo steps; a second layer from 0 s → V3 (it
  overlaps on V2), dragged down → refused, V3; Alt+drag of the first up to V4 → three layers, the copy on V4; Delete
  on the lanes → two; V, a click on the canvas at (300, 200) → the V3 layer picked, dragged +50, +20 →
  "…:V3:0.00:370,200,100:100", the main video's x 320 unmoved; Ctrl+Z ×7 back to the first layer at V2, 0 s.
walk-tracks-memory.mjs (5 s, 3 PASS): avloop-cli frames 600 of a 105-frame Crossfade loop with three layers, 138 fps
  on the GPU; Private Bytes 349.0 → peak 354.7 MB after the first second (+5.7).
```

## The pictures

| Picture | What | What it shows — and what was seen in it |
| --- | --- | --- |
| `timeline-tracks.png` | the timeline area | lanes V5 (empty), V4 "counter-640p30 copy" (selected, white outline), V3 "counter-640p30" over the whole 10 s, V2 "counter-640p30" from 3 s to 8 s; under them the main video's thumbnails |
| `canvas-tracks.png` | the canvas | the V3 layer moved right and down over the main video, outlined and named |

## The regressions

| Walk of before | Result | Mended |
| --- | --- | --- |
| AV-005 `walk-timeline.json`, AV-003 `walk-selection.json` | PASSED (24, 12 s) | — |
| AV-011 `walk-keyframes.json`, AV-018 `walk-key-copy.json` | PASSED (8, 6 s) | — |
| AV-024 `walk-shapes.json`, AV-030 `walk-layers.json`, `walk-layers-render.mjs` | PASSED (13, 9, 6 s) | — |

Logs in `AV-031-walk/regressions/`; AV-030's folder put back.

## External freshness

- Not applicable — no dependency changed.

## Files and line counts

| File | Before | After |
| --- | --- | --- |
| `src/ui/timeline/TrackLanes.cpp` / `.h` | — | **228 / 55** |
| `src/ui/tools/SelectionTool.cpp` · `MainWindowLayers.cpp` · `panels/LayersPanel.cpp` | 130 · 162 · 164 | **147 · 170 · 177** |

## Recovery log

Attempt 1 RESOLVED — walk-tracks: an Alt+drag into the empty top lane landed a track too high — the lanes grew under
the drag and the rows moved under the pointer (a real bug): the empty lane placed on top and the layout frozen from
press to release.

## Deviations from the phase contract

- R-104: no thumbnails in the bars, no track header (eye and lock in the Layers section), no track menu; the side
  panel's "Clip" line and the status bar's "V2 · name" are the Layers section's selected row.

## Blocked legs (if any)

None.

## Staged paths

The paths above, `docs/SPEC.md`, `docs/lore/avloop.md`, `README.md`, `docs/screenshots/`, the programme's files.

## Next

None — every phase closed (items 20 … 27 done). No Release (R-78, R-80: version 3 unreleased until the owner asks).
