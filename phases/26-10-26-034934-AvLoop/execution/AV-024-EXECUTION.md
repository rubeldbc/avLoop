# AV-024 — Execution Record

**Phase:** AV-024 — The shapes (owner item 22, late; the Pen is AV-025)
**Status:** DONE
**Date:** 2026-10-29
**Phase start commit:** `27fa05e` (*[AV-023] The inverse selection: the chosen area cut out, the rest kept*)
**ADR:** none owed (AVADR-0001 kept: one mask; AVADR-0002: at most 64 shapes, one running coverage)
**Environment class:** local-disposable (this PC: `build-lab/`, the samples, the walk's folder; the owner's AvLoop open
from `build\bin`, untouched)

## The owner's words

*"[3] Users should be able to create custom shapes, which can then be used to define a selection or an inverse
selection. Additionally, standard shapes—such as rectangles and ellipses—can also be created and used for selection or
inverse selection."*

## What was built

1. `src/core/MaskShape.{h,cpp}` (new) — `PathPoint` (anchor and handles, for AV-025), `MaskShape` (Rect, Rounded,
   Ellipse, Polygon, Path; Add, Cut, KeepOnly, Exclude; name, visible, locked, feather, strength), its outline
   (`QPainterPath`), its file form, `maskshapes::join`/`start`/`simplified` (Douglas–Peucker)/`nextName`.
2. `src/core/CropShape.{h,cpp}` — `shapes` (≤ 64); `Geometry.cpp` — alpha with a visible shape; `ProjectJson.cpp` —
   `"shapes"` under `crop`; `Project.{h,cpp}` — the selected shapes (not in Undo), `Aspect::Shapes`.
3. `src/render/MaskBuilder.{h,cpp}` — `shapeCoverage` (QPainter, anti-aliased) and the running coverage, the crop's
   own multiplying it; `FrameRenderer.cpp` — a mask with shapes even without a crop.
4. `src/ui/tools/ShapeTool.cpp`, `ShapeEdit.{h,cpp}` (new) — the S tool (drawing the five kinds, selecting, moving,
   handles, anchors, Delete, arrows, Esc); `src/ui/options/ShapeOptions.cpp` (new) — Draw, Role, X/Y/W/H/Radius,
   Delete; `src/ui/panels/ShapesPanel.cpp` (new) — the Crop section's list, Role, Delete.
5. `src/ui/canvas/CanvasOverlay.cpp`, `Canvas.h` — the dim by the shapes' path booleans, their outlines,
   `sceneToViewTransform`. `MainWindow.cpp`, `MainWindowCanvas.cpp`, `Tool.h`, `OptionsBar.h`, `SidePanel.h` — the
   rail's S, the tool, the bar, the list.
6. Icons `tool-shape`, `shape-polygon`, `shape-freehand`, `shape-cut` (46 … 49); `test-icons` 49; AV-001's shell walk
   49 icons and 10 rail tools.
7. `src/ui/WalkSteps.cpp` — the drag's `"via"` points.
8. Tests: `test-shapes` (new: outlines, the file form, simplify, names and hits, the joins), `test-mask` (`shapes`),
   `test-project-json` (`shapesRoundTrip`).
9. Walks: `AV-024-walk/walk-shapes.json`, `walk-shapes-render.mjs`.

## Decisions recorded

- **D-022** — the shapes (with AV-025's Pen); spec §0.
- **R-82** shapes in the one frame, Select or Cut; **R-83** the file's form, roles `"add"`/`"cut"`; **R-84** one running
  coverage, QPainterPath in core; **R-85** the tool and its bar, no snapping; **R-87** icons 46 … 49.

## Specification delta

- §0: `D-022`. The lore: *The shapes (late)* — six rules.

## Verification

| Command | Result |
| --- | --- |
| `scripts\build.ps1 -BuildDir build-lab` | built Release, 0 warnings |
| `ctest --test-dir build-lab --output-on-failure --timeout 300 -R avloop` | **25 passed**, 0 failed, 76 s (test-shapes new) |
| `scripts\format.ps1 -Check` · `check-icons` · `check-secrets` | PASS: 233 files formatted · PASS: 49 icons, 49 listed, avloop.ico with 7 sizes · PASS: 733 files, nothing secret or personal |
| `node control/phase-tools/check-loc.mjs` | PASS: 293 code files, none over 600 lines (the largest: src/core/ClipEffects.cpp, 425 lines) |
| `node control/phase-tools/validate-phase-system.mjs` | PASS: 31 phases, 34 requirement ids |
| README pictures | `readme-shots.json` by `build-lab`: 11 taken, the largest `main-window.png` 201 KB (the rail's S) |
| AV-023's CI run (AV-022's code with it) | green, 8 m 59 s (run 40919386856); AV-022's own run cancelled by the next push |

## The walk

```
AVLOOP_BIN_DIR=build-lab/bin node control/lab/walk-phase.mjs execution/AV-024-walk walk-shapes.json
  walk-shapes-render.mjs → PHASE WALKS PASSED
walk-shapes.json (13 s, 38 PASS): S; a rectangle drawn → rect:add:101,60,200,140, selected, one Undo step, the bar's
  W 200, Output "640 × 360 · alpha"; moved → 141,80; resized by its handle → 240 × 160 (three steps); Esc; Ellipse,
  Cut → ellipse:cut:401,160,160,160; Polygon: five clicks (shape.points 5), a click on the first closes →
  polygon:add:n5; Esc cancels a polygon begun; Freehand: a drag round through five via points → a fourth shape;
  the list "Polygon 2 · Select|Polygon 1 · Select|Ellipse 1 · Cut|Rectangle 1 · Select"; a click selects the
  rectangle, the section's Role makes it cut, Ctrl+Z; Delete → 3, Ctrl+Z → 4; F5 → 640x360.
walk-shapes-render.mjs (5 s, 13 PASS): no crop 640x360; the Select rectangle 255, 255; the Cut ellipse 0; its box
  corner 255; outside 0, 0; the edge 0 → 0 → 255 → 255 (x 98 … 101); a Cut alone 0 inside, 255 outside; a triangle
  255 inside, 0 beyond its slope; with a crop 200x200, the shape's part 255, beyond it 0; GPU vs CPU max diff 1; no
  shapes, the crop's own mask; exported two.webm, libvpx alpha 255 / 0 / 0.
```

## The pictures

| Picture | What | What it shows — and what was seen in it |
| --- | --- | --- |
| `canvas-shapes.png` | the canvas, four shapes | the rectangle over the green, yellow and blue bars bright, the five-sided polygon bottom-left bright, the freehand outline top-right; the cut ellipse dashed (outside every selecting shape, so it cuts nothing); the rest dimmed |
| `panel-shapes.png` | the Crop section | "No crop: the whole video is exported."; Shapes: Polygon 2 · Select, Polygon 1 · Select, Ellipse 1 · Cut, Rectangle 1 · Select with their icons; Role Select; Delete |
| `preview-shapes.png` | the Preview | the checker everywhere but the rectangle, the polygon (the red bar, a white strip under it) and the freehand outline (magenta and cyan); "VP9 alpha" |
| `render-shapes.png` | avloop-cli frame | the Select rectangle with a round hole (the Cut ellipse) in it, clear around |

## The regressions

| Walk of before | Result | Mended |
| --- | --- | --- |
| AV-001 `walk-shell.json` | PASSED (7 s) | 49 icons and 10 rail tools written first |
| AV-004 `walk-crop.json` | PASSED (24 s) | — |
| AV-023 `walk-inverse.json`, `walk-inverse-render.mjs` | PASSED (9, 4 s) | — |
| AV-007 `walk-render.mjs` | PASSED (18 s) | — |
| AV-010 `walk-project.mjs` | PASSED (16 s) | — |

Logs in `AV-024-walk/regressions/`; AV-007's, AV-010's and AV-023's folders put back as their phases left them.

## External freshness

- Not applicable — no dependency changed.

## Files and line counts

| File | Before | After |
| --- | --- | --- |
| `src/core/MaskShape.cpp` / `.h` | — | **226 / 89** |
| `src/ui/tools/ShapeTool.cpp` · `ShapeEdit.cpp` | — | **340 · 78** |
| `src/ui/options/ShapeOptions.cpp` · `panels/ShapesPanel.cpp` | — | **129 · 134** |
| `src/render/MaskBuilder.cpp` · `ui/canvas/CanvasOverlay.cpp` | 84 · 73 | **152 · 135** |
| `src/ui/WalkSteps.cpp` | 378 | **390** |
| `tests/test-shapes.cpp` · `test-mask.cpp` | — · 146 | **81 · 192** |

Hand-written lines about 1,250 against a budget of 1,300.

## Recovery log

Attempt 1 RESOLVED — test-shapes: (100, 100) is the rectangle's excluded corner; the test's point moved inside.
Attempt 1 RESOLVED — walk-shapes: the freehand drag made no shape — the walk ran on a build before `"via"`; rebuilt,
the shape made (37 points dragged, 7 kept). Attempt 1 RESOLVED — the summary's x read 101 for a drag from 100 (the
canvas's ½ px), the expectations as observed. Found on the way: a polygon's rubber segment did not follow a buttonless
move — the hover now taken in `cursor()`.

## Deviations from the phase contract

- The roles are written `"add"`/`"cut"`, not `"select"` (R-83) — no written value renamed later.
- X / Y / W / H / Radius on the bar, not in the Crop section; no snapping for shapes (R-85).

## Blocked legs (if any)

None.

## Staged paths

The paths above, `docs/SPEC.md`, `docs/lore/avloop.md`, `README.md`, `docs/screenshots/`, the programme's files.

## Next

AV-025 — the pen tool.
