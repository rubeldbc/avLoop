# AS-005 — Execution Record

**Phase:** AS-005 — The Effect Controls (owner item 5)
**Status:** DONE
**Date:** 2026-10-30
**Phase start commit:** 623cbf0 (`[AS-004]`)
**ADR:** none owed — the clip's keys are D-029's, its masks D-022 … D-024's, every edit one value step (D-033)
**Environment class:** local-disposable (this PC: the build in `build/`, the samples in `samples/`, the walk's folder)

## The owner's words

*"video layer will have all the facility like adobe premiere pro with keyframes functionality also"* and *"Make this
application like mini adobe premiere pro … cutting like hole a video and export as transparent."*

## What was built

1. `src/ui/panels/EffectControlsPanel.cpp` (new) — `SidePanel::bindControls`: the Effect Controls section for the
   selected clip (else the main clip): Position X, Y, Scale, Opacity at the playhead; the stopwatch; ◀ ◆ ▶; the masks'
   list, Rectangle, Ellipse, Delete, Mode, Feather, Strength. Walk probes `controls.clip`, `controls.look`,
   `controls.keyCount`, `controls.maskList`. `SidePanel` — the section first.
2. `src/core/Project.*`, `ProjectEdit.cpp` — one selected clip (`selectedClip`, `setSelectedClip`; the layer selection
   its view); `changeClip`; `panelEffects` / `changePanelEffects` the selected clip's.
3. `src/core/Sequence.*` — `Clip::keyAt`, `Clip::setLook` (the stopwatch's write, D-029's).
4. `src/ui/timeline/SequenceLanes.cpp` — one clip selected is the project's; it follows the project's.
5. `src/ui/MainWindowLayers.cpp` — the Effect Controls bound; the Clip effects title the selected clip's.
6. `src/ui/MainWindowMonitor.cpp` — the composite drawn at its place in the view (`sceneToView`).
7. `tests/test-sequence.cpp` — `selectedClip`: the effects on the selected clip alone, the stopwatch off and on, a step
   undone, the selection gone with its clip.

## Decisions recorded

- **D-035** — the Effect Controls; spec §0. **RS-20, RS-21** — one stopwatch for motion and opacity, the keys' lane
  the clip's bar; a mask added is Select over the middle half.

## Specification delta

- §0: `D-035`. README: the Effect Controls; screenshots retaken (11 PNGs, the largest 203 KB). The lore: a picture
  read as expected is not a picture read; an underlay paints in view px.

## Verification

| Command | Result |
| --- | --- |
| `scripts\build.ps1` | Built Release, 0 warnings (`/W4 /WX`) |
| `ctest --test-dir build --output-on-failure --timeout 300 -R avloop` | **27 passed**, 0 failed |
| `scripts\format.ps1 -Check`; `node tools/icons/check-icons.mjs` | PASS 269 files; PASS 61 icons |
| `node tools/check-secrets.mjs` | PASS (on its own, exit code 0) |
| `check-loc.mjs` | PASS: the largest code file `src/core/ClipEffects.cpp`, 425 lines |
| `validate-phase-system.mjs` (both programmes) | PASS: 33 phases; PASS: 9 phases |
| `git push origin main`; CI | see the commit `[AS-005]` and its run |

## The walk

```
node control/lab/walk-phase.mjs execution/AS-005-walk walk-effect-controls.json walk-effect-controls-render.mjs → PASSED
  walk-effect-controls.json (1440 × 900): the counter — controls.clip counter-640p30 (the main clip); import the 1080p
    counter, Track V2, Overwrite at 0 s → V1:0.00-10.00 V2:0.00-10.00
  a click on V2's bar → controls.clip counter-1080p30; Scale 25, X 160, Y 90 → 160.0,90.0,25.0,100.0, no keys
  0 s, Keyframes on → 1 key; 2 s, X 480, Y 270, Opacity 0 → 2 keys; 1 s → 320.0,180.0,25.0,50.0 (half way);
    ◀ → 0 s, ▶ → 2 s
  Ellipse → ellipse:add; Mode Cut → ellipse:cut; Invert on (Clip effects): effects.main false (the main clip's
    untouched); the monitor the composite; pictures; Save As controls.avloop
  walk-effect-controls-render.mjs: V2 two keys, a Cut mask, Invert, the main clip without it; 0 s: (20, 20) 255,0,254
    (V2's green inverted); 1 s: (180, 110) 128,127,127 (magenta half over V1, wanted ≈ 128,127,128); (320, 180) through
    the hole = V1 alone's 0,0,254; 2 s: byte for byte V1 alone (opacity 0); both projects removed
No avloop process, session cache, lab folder or temp file left (walk-phase.mjs).
```

## The pictures

| Picture | What | What it shows — and what was seen in it |
| --- | --- | --- |
| `AS-005-walk/effect-controls.png` | the side panel at 2 s | "Effect Controls — counter-1080p30": X 480, Y 270, Scale 25 %, Opacity 0 %; Keyframes on, "Keyframe 2 of 2"; "Ellipse 1 · Cut (a hole)", Mode Cut, Feather 0, Strength 100 %; the Crop section below |
| `AS-005-walk/monitor-1s.png` | the monitor at 1 s | V1's counter (frame 30); V2 inverted at 25 % and 50 %, centred, an elliptical hole showing V1's yellow and blue; the main video's outline over the frame (first taken before the fix: the composite at view px under the outline — the bug mended) |
| `AS-005-walk/render-1s.png` | avloop-cli at 1 s | the same as the monitor, in the frame |

## The regressions

| Walk of before | Result | Mended |
| --- | --- | --- |
| AS-003 `walk-shell.json`, AS-004 `walk-timeline-edit.json` + render | PASSED (their monitor pictures retaken at the right scale) | none |
| AV-019 effects, AV-030 layers, AV-031 tracks, AV-033 layer keys, AV-004 crop, AV-028 resets, AV-020 theme, AV-001 shell, AV-011 keyframes, AV-023 inverse, AV-024 shapes, AV-029 align, AV-008 preview, AV-010 settings, AV-009 export UI (GUI walks) | 15 PASSED | none |

Their logs: `AS-005-walk/regressions/`; AvLoop's walk folders put back.

## External freshness

- Not applicable — no dependency changed.

## Files and line counts

| File | Before | After |
| --- | --- | --- |
| `src/ui/panels/EffectControlsPanel.cpp` | — | **315** |
| `src/ui/panels/SidePanel.cpp` / `.h` | 75 / — | **76** / + `bindControls`, `controls_` |
| `src/core/Project.cpp`; `ProjectEdit.cpp` | 288; 182 | **305**; **195** |
| `src/core/Sequence.cpp`; `tests/test-sequence.cpp` | 246; 276 | **271**; **318** |
| `src/ui/MainWindowLayers.cpp`; `MainWindowMonitor.cpp`; `timeline/SequenceLanes.cpp` | 211; 117; 247 | **214**; **118**; **239** |

## Recovery log

- Attempt 1 RESOLVED — the render check at 1 s read V1's yellow: the place was miscomputed (25 % of 1920 × 1080 is
  480 × 270, from (80, 45)); the expectation now half of V2's inverted green over V1 alone's pixel there.
- Attempt 2 RESOLVED — the monitor's picture showed the composite at view px under the scene-px outline (AS-003's
  too, misread then): drawn through `sceneToView`; AS-003's and AS-004's walks re-run, their pictures retaken.

## Deviations from the phase contract

- One stopwatch for motion and opacity; the key lane is the clip's bar (RS-20).

## Staged paths

- `src/ui/panels/EffectControlsPanel.cpp`, `SidePanel.h`, `SidePanel.cpp`; `src/core/Project.h`, `Project.cpp`,
  `ProjectEdit.cpp`, `Sequence.h`, `Sequence.cpp`; `src/ui/timeline/SequenceLanes.cpp`, `src/ui/MainWindowLayers.cpp`,
  `src/ui/MainWindowMonitor.cpp`; `src/CMakeLists.txt`; `tests/test-sequence.cpp`; `docs/SPEC.md`,
  `docs/lore/avloop.md`, `README.md`, `docs/screenshots/*.png`; the Studio's `READINGS.md`, `AUTONOMOUS_STATE.md`,
  `PROGRESS.json`, `control/00_PHASE_INDEX.md`, `phases/AS-005-*.md`, `phases/AS-006-*.md`, `execution/AS-003-walk/`,
  `execution/AS-004-walk/`, `execution/AS-004-EXECUTION.md`, `execution/AS-005-EXECUTION.md`, `execution/AS-005-walk/`.

## Next

AS-006 — the masks on the monitor, promoted to IN_PROGRESS in the same turn. No avloop process, session cache or walk
temp file left.
