# AV-027 — Execution Record

**Phase:** AV-027 — The soft parts (owner item 24, late)
**Status:** DONE
**Date:** 2026-10-29
**Phase start commit:** `5032c97` (*[AV-026] The shape combinations: Pathfinder live and Combine*)
**ADR:** none owed (AVADR-0002: the distance transform's grids bounded by the output size)
**Environment class:** local-disposable (this PC: `build-lab/`, the samples, the walk's folder; the owner's AvLoop open
from `build\bin`, untouched)

## The owner's words

*"[5] It should be possible to select specific areas within a shape and apply custom feathering or transparency,
allowing any part of the selection to be made transparent."*

## What was built

1. `src/render/DistanceField.{h,cpp}` (new) — the exact Euclidean distance transform (Felzenszwalb–Huttenlocher) and
   a raster's signed distance.
2. `src/render/MaskBuilder.{h,cpp}` — a shape's coverage feathered inward from its signed distance (analytic for the
   standard kinds, the transform for custom outlines); Strength already multiplies where shapes join (AV-024).
3. `src/ui/options/ShapeOptions.cpp`, `OptionsBar.h` — Feather and Strength on the S bar, `setShapesSoftness` for every
   selected shape. `src/ui/panels/ShapesPanel.cpp` — the Crop section's Feather and Strength, the list's note, probe
   `shapes.soft`. `src/ui/canvas/CanvasOverlay.cpp` — a weak Cut dimmed by its strength.
4. `tests/test-mask.cpp` — `distanceTransform` (brute force, exact), `softShapes` (Strength 50 → 128, a feather's
   monotonic ramp, the defaults' bytes, a feathered triangle by the transform).
5. Walks: `AV-027-walk/walk-soft.json`, `walk-soft-render.mjs`.

## Decisions recorded

- **D-024** — the soft parts; spec §0.
- **R-90** Feather and Strength a shape, a weak Cut the transparency; **R-91** the exact transform, the canvas a hint.

## Specification delta

- §0: `D-024`. The lore: two rules (the most vexing parse; an edge on the frame's edge).

## Verification

| Command | Result |
| --- | --- |
| `scripts\build.ps1 -BuildDir build-lab` | built Release, 0 warnings (after C2109, see Recovery) |
| `ctest --test-dir build-lab --output-on-failure --timeout 300 -R avloop` | **25 passed**, 0 failed, 76 s |
| `scripts\format.ps1 -Check` · `check-icons` · `check-secrets` | PASS: 240 files formatted · PASS: 53 icons, 53 listed, avloop.ico with 7 sizes · PASS: 799 files, nothing secret or personal |
| `node control/phase-tools/check-loc.mjs` | PASS: 303 code files, none over 600 lines (the largest: src/core/ClipEffects.cpp, 425 lines) |
| `node control/phase-tools/validate-phase-system.mjs` | PASS: 31 phases, 34 requirement ids |
| README pictures | not retaken: the default window unchanged |
| AV-026's CI run | green, 7 m 53 s (run 40925466663) |

## The walk

```
AVLOOP_BIN_DIR=build-lab/bin node control/lab/walk-phase.mjs execution/AV-027-walk walk-soft.json walk-soft-render.mjs
  → PHASE WALKS PASSED
walk-soft.json (8 s, 19 PASS): a Select rectangle (60, 40) – (580, 320); Ellipse, Cut drawn beside it and placed by
  the bar at 220, 100, 200 × 160; the bar's Strength 40 → "0/100;0/40", Feather 30 → "0/100;30/40", the list
  "Ellipse 1 · Cut · 30 px · 40 %"; F5; Ctrl+Z → feather 0; the section's Strength 100; Ctrl+A and the section's
  Feather 12 → "12/100;12/100".
walk-soft-render.mjs (3 s, 7 PASS): Strength 40 → the cut's centre 153, outside it 255, outside the rectangle 0;
  Feather 30 → x 219 … 260: 255 → 245 → 218 → 184 → 153 → 153, the centre 153; a feathered polygon down from its top
  edge 13 → 77 → 140 → 204 → 255 → 255; the defaults byte for byte (centre 0); GPU vs CPU max diff 1.
```

## The pictures

| Picture | What | What it shows — and what was seen in it |
| --- | --- | --- |
| `canvas-soft.png` | the canvas | the rectangle bright, the cut ellipse inside it dimmed by its 40 % (a hint), its outline dashed and handles |
| `panel-soft.png` | the Crop section | "Ellipse 1 · Cut · 30 px · 40 %", Feather 30 px, Strength 40 % |
| `preview-soft.png` | the Preview | the bars over the checker, the ellipse's middle showing the checker through at 40 %, its rim fading softly into the bars |
| `render-soft.png` | avloop-cli frame | the same: a soft, partly clear oval in the opaque rectangle |

## The regressions

| Walk of before | Result | Mended |
| --- | --- | --- |
| AV-024 `walk-shapes.json`, `walk-shapes-render.mjs` | PASSED (12, 4 s) | — |
| AV-025 `walk-pen-render.mjs` | PASSED (1 s) | — |
| AV-026 `walk-combine.json`, `walk-combine-render.mjs` | PASSED (11, 3 s) | — |
| AV-004 `walk-crop.json` | PASSED (23 s) | — |

Logs in `AV-027-walk/regressions/`; AV-024's, AV-025's and AV-026's folders put back as their phases left them.

## External freshness

- Not applicable — no dependency changed.

## Files and line counts

| File | Before | After |
| --- | --- | --- |
| `src/render/DistanceField.cpp` / `.h` | — | **100 / 19** |
| `src/render/MaskBuilder.cpp` | 152 | **177** |
| `src/ui/options/ShapeOptions.cpp` · `panels/ShapesPanel.cpp` · `canvas/CanvasOverlay.cpp` | 192 · 151 · 135 | **232 · 186 · 140** |
| `tests/test-mask.cpp` | 218 | **289** |

## Recovery log

Attempt 1 RESOLVED — build: C2109/C2660, `std::vector<float> f(size_t(longest))` read as a function (the most vexing
parse); plain sizes. Attempt 1 RESOLVED — test-mask: a feathered triangle's top edge lay on the frame's edge, nothing
outside it to measure; the triangle inset. Also mended: AV-026's record's CI line.

## Deviations from the phase contract

- The transform keeps two float grids of the output size, not "two rows" as `control/27` §2 first wrote: an exact 2-D
  transform needs the grid (≤ 1920 × 1920 × 4 B × 2 while it runs, R-91).

## Blocked legs (if any)

None.

## Staged paths

The paths above, `docs/SPEC.md`, `docs/lore/avloop.md`, `README.md`, the programme's files.

## Next

AV-028 — the resets.
