# AV-033 — Execution Record

**Phase:** AV-033 — The layers' keyframes and effects (owner item 29, late; before AV-032)
**Status:** DONE
**Date:** 2026-10-30
**Phase start commit:** `7660f1c` (*[AV-031] The record: the last CI run, green*)
**ADR:** none owed (AVADR-0001 kept: the LayerPass after either back end)
**Environment class:** local-disposable (this PC: `build/` — fresh at AV-032's start, the owner's AvLoop closed —, the
samples, the walk's folder)

## The owner's words

*"video layer will have all the facility like adobe premiere pro with keyframes functionality also. do it first. then do
the test. if needed do the phase files and complete all the task"* — said while AV-032's sweep ran: the sweep stopped
(two walks run, folders put back) and put after this phase.

## What was built

1. `src/core/VideoLayer.{h,cpp}` — `LayerKey`, a layer's keys and `ClipEffects`; `at` (held, linear, eased),
   `keyAt`, `setLook` (into the key at the playhead, made when none); `rectAt`; the file's `"keys"`, `"effects"`; the
   hit by the keyed rectangle.
2. `src/render/LayerPass.{h,cpp}` — the keyed look and the layer's effects (EffectsPass) in the composite.
3. `src/core/Project.{h,cpp}` — `panelEffects`/`changePanelEffects` (the Clip effects section's target);
   `src/ui/panels/EffectsPanel.cpp` uses them and refreshes on the selection; `Section::setTitle`.
4. `src/ui/panels/LayersPanel.cpp` — values at the playhead; Key, Delete key, Ease, "Keyframe n of m"; edits into keys;
   presses as steps of their own; probes `layers.keys`, `layers.effects`, `effects.main`.
5. `src/ui/MainWindowLayers.cpp` — the canvas's keyed look and effects; the section's title. `SelectionTool.cpp` — a
   drag on a keyed layer keys. `TrackLanes.cpp` — diamonds; a double-click on one seeks.
6. `tests/test-shapes.cpp` — `layerKeys`.
7. Walks: `AV-033-walk/walk-layer-keys.json`, `walk-layer-keys-render.mjs`.

## Decisions recorded

- **D-029** — the layers' keyframes and effects; spec §0.
- **R-106** a layer's position, scale, opacity keyed, Premiere's stopwatch, R-98 vetoed; **R-107** its effects through
  the Clip effects section following the selection.

## Specification delta

- §0: `D-029`. The lore: one rule (presses never merge).

## Verification

| Command | Result |
| --- | --- |
| `scripts\build.ps1` | built Release, 0 warnings (after a missing include, C2027) |
| `ctest --test-dir build --output-on-failure --timeout 300 -R avloop` | **25 passed**, 0 failed, 74 s (test-shapes `layerKeys`) |
| `scripts\format.ps1 -Check` · `check-icons` · `check-secrets` | PASS: 253 files formatted · PASS: 61 icons, 61 listed, avloop.ico with 7 sizes · PASS: 941 files, nothing secret or personal |
| `node control/phase-tools/check-loc.mjs` | PASS: 321 code files, none over 600 lines (the largest: src/core/ClipEffects.cpp, 425 lines) |
| `node control/phase-tools/validate-phase-system.mjs` | PASS: 33 phases, 36 requirement ids |
| README pictures | not retaken here: AV-032 retakes them after the sweep |

## The walk

```
node control/lab/walk-phase.mjs execution/AV-033-walk walk-layer-keys.json walk-layer-keys-render.mjs → PHASE WALKS PASSED
walk-layer-keys.json (9 s, 21 PASS): a layer at 50 %, no keys; Key at 0 → "0.00:320,180,50,100"; at 1 s X 160 and
  Opacity 40 typed → a second key "1.00:160,180,50,40"; at 0 Ease → "…100e"; Delete key → one key; Ctrl+Z → the eased
  key back; at 1 s the Selection's drag → "1.00:200,200,50,40", the main video's x 320; Exposure 1.5 typed with the
  layer selected → the layer's effects, the main video's none; F5; a click beside the layer → none selected, the
  section the main video's (no effects).
walk-layer-keys-render.mjs (6 s, 8 PASS): frame 0 the layer's green at (100, 125); frame 30 the main red back there and
  the layer's red at (200, 150); opacity keyed 100 → 0: 127,127,0 at 1 s; Invert on the layer: 255,1,255, the main
  video beside it 254,0,253 as it was; Ease at 0.5 s elsewhere than linear; no keys, no effects byte for byte; GPU vs
  CPU max diff 0.
```

## The pictures

| Picture | What | What it shows — and what was seen in it |
| --- | --- | --- |
| `preview-layer-keys.png` | the Preview at 0.5 s | the half-size counter between its two keyed places, partly faded, brighter (its own Exposure), over the main counter |
| `panel-layer-keys.png` | the Layers section | X 200, Y 200, Scale 50 %, Opacity 40 %; Key, Delete key, Ease; "Keyframe 2 of 2" |
| `timeline-diamonds.png`, `canvas-layer-keys.png` | the lanes; the canvas | two diamonds on the V2 bar at 0 s and 1 s; the layer at its 1 s key, outlined |

## The regressions

| Walk of before | Result | Mended |
| --- | --- | --- |
| AV-030 `walk-layers.json`, `walk-layers-render.mjs`; AV-031 `walk-tracks.json`, `walk-tracks-memory.mjs` | PASSED (9, 6, 10, 5 s) | — |
| AV-019 `walk-effects.json`, `walk-effects-render.mjs`; AV-020 `walk-theme.json`; AV-028 `walk-resets.json` | PASSED (8, 4, 8, 13 s) | — |

Logs in `AV-033-walk/regressions/`; the earlier folders put back.

## External freshness

- Not applicable — no dependency changed.

## Files and line counts

| File | Before | After |
| --- | --- | --- |
| `src/core/VideoLayer.cpp` · `ui/panels/LayersPanel.cpp` · `ui/MainWindowLayers.cpp` | 85 · 177 · 170 | **149 · 276 · 194** |
| `src/render/LayerPass.cpp` · `ui/timeline/TrackLanes.cpp` · `tests/test-shapes.cpp` | 122 · 228 · 241 | **125 · 244 · 278** |

## Recovery log

Attempt 1 RESOLVED — build: C2027, `Section` not included in `MainWindowLayers.cpp`. Attempt 1 RESOLVED — walk: the
Ease click and the Delete key merged into one Undo step (a real slip): presses no longer merge. Attempt 1 RESOLVED — the
render walk's invert check carried a needless clause of mine (the invert itself right, 255,1,255).

## Deviations from the phase contract

- Rotation is not keyed (no rotation tool, README §4); a layer's effects are not keyframed; dragging a diamond in the
  lanes is not built (a key is moved by deleting and keying again).

## Blocked legs (if any)

None.

## Staged paths

The paths above, `docs/SPEC.md`, `docs/lore/avloop.md`, `README.md`, the programme's files.

## Next

AV-032 — the sweep.
