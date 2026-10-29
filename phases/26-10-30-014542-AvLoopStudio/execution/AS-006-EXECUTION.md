# AS-006 — Execution Record

**Phase:** AS-006 — The masks on the monitor (owner item 6)
**Status:** DONE
**Date:** 2026-10-30
**Phase start commit:** ffa1cd2 (`[AS-005]`)
**ADR:** none owed — the shapes' tools and maths are AvLoop's (D-022 … D-026), re-aimed at a clip's masks
**Environment class:** local-disposable (this PC: the build in `build/`, the samples in `samples/`, the walk's folder)

## The owner's words

*"My final goal that I have to flexible edit and cutting like hole a video and export as transparent."*

## What was built

1. `src/core/SequenceView.*` — `masksToScene`, `masksFromScene`: a clip's masks between its own px and the scene's.
   `src/render/SequenceRenderer.*` — the composite uses them (its own `placed` gone).
2. `src/core/Project.*`, `ProjectEdit.cpp` — `shapeHost()` (the crop with the selected clip's masks in the scene at
   the playhead, else the crop) and `changeShapeHost()` (back into the clip's masks, one step, else `changeCrop`);
   `selectedShapes` counts the host's; a new selected clip lets the shapes go.
3. `src/ui/tools/ShapeTool.cpp`, `PenTool.cpp`, `src/ui/options/ShapeOptions.cpp`, `src/ui/panels/ShapesPanel.cpp` —
   every read and write of the shapes through the host.
4. `src/ui/tools/SelectionTool.cpp` — another clip on V1 under the pointer: picked and dragged (`Mode::Clip`).
5. `src/ui/panels/EffectControlsPanel.cpp` — the probe `controls.posX`.

## Decisions recorded

- **D-036** — the masks on the monitor; spec §0. **RS-22, RS-23** — masks drawn where the clip stands, stored in its
  own px; with none selected the shape tools stay the crop's; the flips aside.

## Specification delta

- §0: `D-036`. README: the S and P tools draw a selected clip's masks.

## Verification

| Command | Result |
| --- | --- |
| `scripts\build.ps1` | Built Release, 0 warnings (`/W4 /WX`) |
| `ctest --test-dir build --output-on-failure --timeout 300 -R avloop` | **27 passed**, 0 failed |
| `scripts\format.ps1 -Check`; `node tools/icons/check-icons.mjs` | PASS 269 files; PASS 61 icons |
| `node tools/check-secrets.mjs` | PASS (on its own, exit code 0) |
| `check-loc.mjs` | PASS: the largest code file `src/core/ClipEffects.cpp`, 425 lines |
| `validate-phase-system.mjs` (both programmes) | PASS: 33 phases; PASS: 9 phases |
| `git push origin main`; CI | see the commit `[AS-006]` and its run |

## The walk

```
node control/lab/walk-phase.mjs execution/AS-006-walk walk-masks.json walk-masks-render.mjs → PASSED
  walk-masks.json (1440 × 900): the counter; the 1080p counter overwritten on V2, selected, Scale 25 →
    320.0,180.0,25.0,100.0; Ctrl+0
  S, Ellipse, Cut; a drag on the canvas (260, 140) → (380, 220): controls.maskList ellipse:cut, shapes.count 1 (the
    clip's), undo.count 3
  V; a drag on V2 (150, 80) → (230, 80): controls.posX ≈ 400 (within 1), the mask still ellipse:cut
  a click on an empty lane: nothing selected — shapes.count 0 (the crop's); Save As masks.avloop
  walk-masks-render.mjs: V2's mask in its own px — ellipse cut 721.3,380.4 477.4×319.3 (centred on 960, 540 within
    4 px); x 399.8; the crop no shapes; at 0 s (400, 180) = V1 alone 0,0,254 (the hole moved with the clip); (400, 60)
    15,15,244 ≠ V1's 0,0,254 (V2 there); both projects removed
No avloop process, session cache, lab folder or temp file left (walk-phase.mjs).
```

## The pictures

| Picture | What | What it shows — and what was seen in it |
| --- | --- | --- |
| `AS-006-walk/mask-drawn.png` | the monitor after the drag with S | V2 at 25 % centred, the ellipse drawn over its middle, cut through to V1 |
| `AS-006-walk/mask-moved.png` | after the Selection's drag | V2 moved right to x 400 (its right part past the frame), the elliptical hole at its middle showing V1's blue and magenta — the mask went with the clip |
| `AS-006-walk/render-0s.png` | avloop-cli at 0 s | the same, in the frame |

## The regressions

| Walk of before | Result | Mended |
| --- | --- | --- |
| AV-024 shapes, AV-025 pen, AV-026 combine, AV-027 soft, AV-029 align, AV-022 cut, AV-023 inverse, AV-003 selection, AV-030 layers, AV-031 tracks, AV-033 layer keys (GUI) | 11 PASSED | none |
| AV-024, AV-026, AV-027, AV-030 render walks; AV-031 memory | 5 PASSED | none |
| AS-002 `walk-composite-render.mjs`, AS-005 walks | PASSED | none |

Their logs: `AS-006-walk/regressions/`; AvLoop's walk folders put back.

## External freshness

- Not applicable — no dependency changed.

## Files and line counts

| File | Before | After |
| --- | --- | --- |
| `src/core/SequenceView.cpp`; `src/render/SequenceRenderer.cpp` | 133; 281 | **167**; **264** |
| `src/core/Project.cpp`; `ProjectEdit.cpp` | 305; 195 | **317**; **217** |
| `src/ui/tools/SelectionTool.cpp`; `panels/EffectControlsPanel.cpp` | 152; 315 | **173**; **319** |

## Recovery log

- Attempt 1 RESOLVED — the walk's drag at the fitted zoom moved the clip to 399.8, not 400 (view px): read with `near`
  (`controls.posX`) and within 1 px in the render walk.

## Deviations from the phase contract

- "The monitor dims what the masks cut": the composite shows them cut (every track below seen through), no extra dim.
- The Selection picks and moves other V1 clips; their scale handles are the Effect Controls' Scale.

## Staged paths

- `src/core/SequenceView.h`, `SequenceView.cpp`, `Project.h`, `Project.cpp`, `ProjectEdit.cpp`;
  `src/render/SequenceRenderer.h`, `SequenceRenderer.cpp`; `src/ui/tools/ShapeTool.cpp`, `PenTool.cpp`,
  `SelectionTool.cpp`; `src/ui/options/ShapeOptions.cpp`; `src/ui/panels/ShapesPanel.cpp`, `EffectControlsPanel.cpp`;
  `docs/SPEC.md`, `README.md`; the Studio's `READINGS.md`, `AUTONOMOUS_STATE.md`, `PROGRESS.json`,
  `control/00_PHASE_INDEX.md`, `phases/AS-006-*.md`, `phases/AS-007-*.md`, `execution/AS-002-walk/`,
  `execution/AS-005-walk/`, `execution/AS-005-EXECUTION.md`, `execution/AS-006-EXECUTION.md`, `execution/AS-006-walk/`.

## Next

AS-007 — the export, promoted to IN_PROGRESS in the same turn. No avloop process, session cache or walk temp file left.
