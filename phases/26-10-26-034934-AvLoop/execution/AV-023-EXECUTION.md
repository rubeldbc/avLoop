# AV-023 — Execution Record

**Phase:** AV-023 — The inverse selection (owner item 21, late)
**Status:** DONE
**Date:** 2026-10-29
**Phase start commit:** `f3d9cee` (*[AV-022] The clip as cut: export without a loop or a crossfade*)
**ADR:** none owed (AVADR-0001 kept: one mask, both back ends)
**Environment class:** local-disposable (this PC: `build-lab/`, the samples, the walk's folder; the owner's AvLoop open
from `build\bin`, untouched)

## The owner's words

*"[2] A feature should be implemented to allow for the "inverse selection" of a chosen area, ensuring that the
unselected (inverted) portion is included in the output."*

## What was built

1. `src/core/CropShape.h` — `invert`; `src/core/Geometry.{h,cpp}` — `exportFrame(const CropShape&, …)` (the video's
   rectangle when inverted), `needsAlpha` true, `coverage` Partly while the hole touches the video; the callers
   (`Project`, `RenderJob`, `MainWindowCanvas`, `SelectionOptions`) pass the crop.
2. `src/render/MaskBuilder.{h,cpp}` — `build(crop, frame, size)`: not inverted, AV-004's mask byte for byte; inverted,
   the shape at its place in the frame, clamp((d + ½) / max(f, 1)) — the feather outward; the cache keyed by the crop,
   the frame and the size; `FrameRenderer` passes the job's frame.
3. `src/core/ProjectJson.{h,cpp}`, `ProjectMigrate.cpp` — `"invert"` under `crop`; version 3 with 2 → 3 (R-80).
4. `src/ui/canvas/CanvasOverlay.cpp` — the hole dimmed, the export's frame outlined. `src/ui/panels/CropPanel.cpp` —
   the Inverse box. `src/ui/options/CropOptions.cpp` — the Inverse toggle on every crop bar, Clear clears it.
   `src/ui/tools/CropTool.cpp` — Delete clears it. `MainWindowCanvas.cpp` — probe `crop.invert`.
5. `resources/icons/invert.svg` (the 45th icon; `IconFactory`, `resources.qrc`, `test-icons`, AV-001's shell walk).
6. Tests: `test-mask` (`inverse`, the cache with the frame), `test-geometry` (`inverse`), `test-project-json`
   (version 3, a version-2 file without `invert`, `invertRoundTrip`).
7. Walks: `AV-023-walk/walk-inverse.json`, `walk-inverse-render.mjs`.

## Decisions recorded

- **D-021** — the inverse selection; spec §0.
- **R-79** the hole in the whole video, the feather outward; **R-80** the file's version 3; **R-81** Delete and Clear
  remove it, a redrawn crop keeps it, the 45th icon.

## Specification delta

- §0: `D-021`. The lore: *The inverse selection (late)* — two rules (a sed on a repeated line; a shadowed blocker).

## Verification

| Command | Result |
| --- | --- |
| `scripts\build.ps1 -BuildDir build-lab` | built Release, 0 warnings (after C3892 and C4456 mended, see Recovery) |
| `ctest --test-dir build-lab --output-on-failure --timeout 300 -R avloop` | **24 passed**, 0 failed, 77 s (test-mask `inverse`, test-geometry `inverse`, test-project-json version 3 and `invertRoundTrip`, test-icons 45) |
| `scripts\format.ps1 -Check` · `check-icons` · `check-secrets` | PASS: 225 files formatted · PASS: 45 icons, 45 listed, avloop.ico with 7 sizes · PASS: 701 files, nothing secret or personal |
| `node control/phase-tools/check-loc.mjs` | PASS: 284 code files, none over 600 lines (the largest: src/core/ClipEffects.cpp, 425 lines) |
| `node control/phase-tools/validate-phase-system.mjs` | PASS: 31 phases, 34 requirement ids |
| README pictures | `readme-shots.json` by `build-lab`: 11 taken, 16–199 KB; `main-window.png` shows the Crop section's Inverse row and the bar's Inverse toggle |

## The walk

```
AVLOOP_BIN_DIR=build-lab/bin node control/lab/walk-phase.mjs execution/AV-023-walk walk-inverse.json
  walk-inverse-render.mjs → PHASE WALKS PASSED
walk-inverse.json (10 s, 33 PASS): E, an ellipse 200 × 200 at (220, 80); Inverse clicked in the Crop section →
  crop.invert true, one Undo step, Output "640 × 360 · alpha", the status "Transparent area in output"; Feather 20
  typed on the bar; F5 → 640x360; Ctrl+Z ×2 → feather 0, not inverted, "200 × 200 · alpha"; Ctrl+Y; the bar's toggle
  off and on; M → rect, still inverted; a new rectangle drawn at (20, 20) keeps it; Delete → no crop, not inverted,
  "640 × 360".
walk-inverse-render.mjs (5 s, 12 PASS): not inverted 200x200, centre 255, corner 0; a version-2 file the same bytes;
  inverted 640x360, the hole's centre 0, the corners and both sides 255, the crop's own corner 255, the red bar
  253,0,0,255; feather 20 at x 220 … 198: 0 → 13 → 77 → 140 → 204 → 255 → 255; GPU vs CPU max diff 1; exported
  hole.webm 640x360 60 frames, its alpha by libvpx: centre 0, corner 255, mean 220.2.
```

## The pictures

| Picture | What | What it shows — and what was seen in it |
| --- | --- | --- |
| `canvas-inverse.png` | the canvas, inverted ellipse | the colour bars bright everywhere but the circle, which is dimmed; the ellipse's orange outline, its dashed box and handles; a thin orange outline round the whole video (the export's frame) |
| `panel-inverse.png` | the Crop section | Shape Ellipse; "Inverse" checked under it; X/Y 220.5/80.0, W/H 200/200; Radius greyed; Output "640 × 360 · alpha" |
| `preview-inverse.png` | the Preview, feather 20 | the whole 640 × 360 picture with a round hole showing the checker, its rim fading into the bars over ~20 px outward; "VP9 alpha" |
| `canvas-inverse-rect.png` | a new rectangle, inverted | a small dimmed rectangle at the top-left over the red and green bars, the rest bright |
| `render-inverse-feather.png` | avloop-cli frame | the same hole as the Preview, the frame's corners opaque |

## The regressions

| Walk of before | Result | Mended |
| --- | --- | --- |
| AV-001 `walk-shell.json` | PASSED (8 s) | `icons.count` 45 written first |
| AV-004 `walk-crop.json` (every crop tool, the mask) | PASSED (23 s) | — |
| AV-011 `walk-keyframes.json`, `walk-keyframes.mjs` | PASSED (8, 4 s) | — |
| AV-007 `walk-render.mjs` (GPU = CPU, the crop's mask) | PASSED (17 s) | — |
| AV-010 `walk-project.mjs` (save → open → the same render) | PASSED (16 s) | — |

Logs in `AV-023-walk/regressions/`; AV-007's, AV-010's and AV-011's folders put back as their phases left them.

## External freshness

- Not applicable — no dependency changed.

## Files and line counts

| File | Before | After |
| --- | --- | --- |
| `src/render/MaskBuilder.cpp` / `.h` | 63 / 34 | **84 / 39** |
| `src/core/Geometry.cpp` · `CropShape.h` · `ProjectMigrate.cpp` | 82 · 35 · 43 | **88 · 37 · 52** |
| `src/ui/panels/CropPanel.cpp` · `options/CropOptions.cpp` · `canvas/CanvasOverlay.cpp` | 137 · 185 · 63 | **151 · 200 · 73** |
| `tests/test-mask.cpp` | 107 | **146** |

## Recovery log

Attempt 1 RESOLVED — build: C3892 in `test-geometry` (a sed changed `VideoTransform t;` in two tests; the other
restored). Attempt 1 RESOLVED — build: C4456, a second `QSignalBlocker block` in `CropOptions`' refresh (renamed).

## Deviations from the phase contract

None.

## Blocked legs (if any)

None.

## Staged paths

The paths above, `docs/SPEC.md`, `docs/lore/avloop.md`, `README.md`, `docs/screenshots/`, the programme's files.

## Next

AV-024 — the shapes.
