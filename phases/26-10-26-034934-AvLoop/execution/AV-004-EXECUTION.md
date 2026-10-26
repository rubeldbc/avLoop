# AV-004 — Execution Record

**Phase:** AV-004 — The crop tools (owner item 4)
**Status:** DONE
**Date:** 2026-10-26
**Phase start commit:** `6973465` (*[AV-003] The canvas and the Selection*)
**ADR:** none owed (the mask is the CPU half AVADR-0001 names; it is accepted at AV-007)
**Environment class:** local-disposable (this PC: `build/`, `samples/`, the walk's folder; lab windows never take focus)

## The owner's words

*"The Crop tool decides which part is exported."* — *"Several aspect ratios can be chosen while cropping."*

## What was built

1. `src/core/CropShape` (kind, rect, radius, feather, ratio; clamped: radius ≤ half the short side, feather 0–200; ids
   `rect`/`rounded`/`ellipse`, tools `crop-rect`/`crop-round`/`crop-ellipse`), `core/AspectRatio` (the list, values,
   Custom W:H, `reduced` to lowest terms, `apply` keeping centre and area), `core/Geometry` (`needsAlpha`, `coverage`),
   `Project::changeCrop` with `SetCropCommand`; a new source clears the crop; `.avloop`'s `crop` read and written.
2. `src/ui/tools/CropHandles` (handles, hit test, drawing and resizing with a ratio and from the centre, snapping) and
   `CropTool.cpp` (the three tools: draw, move, eight handles, replace by a drag outside, Shift, Alt, Ctrl, arrows, Esc,
   Delete, double-click, under 16 px cancelled, switching kind keeps the rectangle; thirds, size label and snap lines).
3. `src/ui/canvas/CanvasOverlay.cpp`: 60% black outside the crop (or the drawing), the 1.5 px orange border in the
   shape, the dashed bounding box for Rounded and Ellipse; `CanvasView`: Fit fits the crop.
4. `src/render/MaskBuilder` — the shape's signed distance at each pixel centre, a 1 px edge at Feather 0, Feather
   inward; built at the output size and cached until its inputs change.
5. `src/ui/options/CropOptions.cpp` (Ratio with Custom W:H, X, Y, W, H, Swap W/H, Clear, Radius, Feather; the ratio
   box and `applyRatio` shared with the panel), `src/ui/panels/CropPanel.cpp` (Shape, Ratio, X / Y, W / H, Radius,
   Feather, Output with "· alpha"); the status bar's two notices (`MainWindowCanvas.cpp`).
6. The lab: the walk steps `choose` (a combo item by data or text) and the drag's `keyBeforeRelease`; options probes
   hold `QPointer`s (`probeValue`) and carry their row's name; crop, panel and notice probes.
7. Tests: `test-aspect` (9: the list, values, `reduced`, centre and area kept, limits, Alpha Auto, drawing, resizing,
   handles, snapping), `test-mask` (6: Ellipse corners 0 / centre 255, a Rect opaque, the 1 px band, the Feather ramp,
   the Rounded radius, the cache); `test-project-json` gained the crop round trip.

## Decisions recorded

- **D-004** — the crop tools as delivered; spec §0.
- **R-8** confirmed in code: one crop; none = the whole video (`crop.isEmpty()` everywhere).
- **R-21** Custom… from the crop's own proportions in lowest terms; typed W/H keep the top-left. **R-22** the canvas
  shows the outline, Feather is seen in the Preview and the export. **R-23** what snaps.
- **The mask is sampled at pixel centres** with `coverage = clamp((0.5 − d) / max(feather, 1), 0, 1)`: half a pixel at
  the edge; a Feather of F px reaches full inside F px. The radius is scaled from scene px to output px.
- **Snapping tolerance is 8 view px** (`8 / zoom` scene px), so it feels the same at any zoom.

## Specification delta

- §0: `D-004`.
- Lore: a new section *The crop* — five rules (masks at pixel centres; options rows all exist; Qt forward
  declarations at global scope; a test's output can vanish — `-o file,txt`; Custom ratios reduced).

## Verification

| Command | Result |
| --- | --- |
| `scripts\build.ps1` | built, **0 warnings** |
| `ctest -LE gpu` | **10/10 PASS** (aspect, mask, theme, icons, geometry, commands, project-json, probe, decoder-seek, memory) |
| `scripts\format.ps1 -Check` | PASS: 99 files |
| `node tools/icons/check-icons.mjs` | PASS: 43 icons |
| `node tools/check-secrets.mjs` | PASS: 269 files |
| `node control/phase-tools/check-loc.mjs` | PASS: 131 code files, the largest `src/ui/MainWindow.cpp` 348 lines |
| `node control/phase-tools/validate-phase-system.mjs` | PASS: 16 phases, 21 requirement ids |
| AV-003's CI run | green |

## The walk

```
node control/lab/walk-phase.mjs execution/AV-004-walk walk-crop.json
walk-crop.json (177 steps, 109 expectations; counter-640p30 at 100% so scene px are view px):
  M: drag (100,60)→(300,200) draws 200 × 140 in one step; the bar's W, the panel's W, Output "200 × 140", no notice
  a drag inside moves it to (150,90); the bottom-right handle +40/+20 → 240 × 160
  a left edge 5 px from the video's edge snaps to 0; Ctrl+Z; with Ctrl it lands at 5
  Right, Right, Shift+Down → +2, +10 in one Undo step
  Shift draws 120 × 120 (1:1 from Free); Alt draws 130 × 80 centred at (200,250)
  a 10 px drawing and an Esc-cancelled 150 × 60 drawing leave it as it was (Undo count unchanged)
  Ctrl+0 fits the crop (zoom ≥ 3); Ctrl+1
  Custom… → 13:8 (130 × 80 in lowest terms); W:H typed 2:1 → 144.22 wide, the centre kept; 16:9; W 320 → H 180;
  Swap → 180 × 320, 9:16
  every ratio in turn (free, original, 1:1, 4:3, 3:4, 16:9, 21:9, 3:2, 2:3, 4:5, 9:16) — the centre kept each time;
  9:16 last brings back 180 × 320 (the area kept)
  U: kind rounded, the rectangle kept; Radius 40; Feather 12; Output "… · alpha"
  E: kind ellipse; the bar's Feather 12; the panel's Feather typed 0 → the crop and the bar 0
  a double-click → the whole video (0,0,640,360)
  X 600 → "Transparent area in output"; X 2000 → "Output is fully transparent"
  Delete → no crop, no notice, the panel's hint back; Ctrl+Z → the crop back
  "the owner's focus untouched: another program holds the focus"
PHASE WALKS PASSED
```

`test-mask` with `AVLOOP_TEST_OUT` wrote the masks into the walk's folder.

## The pictures

| Picture | What | What it shows — and what was seen in it |
| --- | --- | --- |
| `AV-004-walk/crop-rect.png` | a Rect after the resize | the frame dimmed outside the crop, inside at full brightness; the orange border; eight white handles with orange borders; the bar and the panel X 150.5 · Y 90 · W 240 · H 160 |
| `AV-004-walk/crop-fit.png` | Ctrl+0 with a 130 × 80 crop | the crop fills the canvas; the dim around it |
| `AV-004-walk/crop-rounded.png` | Rounded, Radius 40, Feather 12, 9:16 | the rounded orange outline and the dashed bounding box; the lower part past the video (no picture there); the panel's Output "180 × 320 · alpha"; the status bar "Transparent area in output"; no stray ":" when the ratio is not Custom |
| `AV-004-walk/crop-ellipse.png` | Ellipse | the elliptic outline inside its dashed box; the bar without Radius; the panel's Radius disabled |
| `AV-004-walk/mask-ellipse.png` | 640 × 640, Feather 0 | white disc, black corners, a hard but smooth edge |
| `AV-004-walk/mask-ellipse-feather20.png` | 400 × 400, Feather 20 | the disc fading to black over 20 px at the rim |
| `AV-004-walk/mask-rounded30.png` | 200 × 100, Radius 30 | a white rounded rectangle, corners cut |
| `docs/screenshots/main-window.png` | README | the demo gradient with a 600 × 600 Rounded crop (Radius 96, Feather 24) and its handles; Output "600 × 600 · alpha" (1440 px, 207 KB) |

## The regressions

| Walk of before | Result | Mended |
| --- | --- | --- |
| AV-003 walk-selection.json | PASSED | the options probes now hold `QPointer`s — they pointed at rows a tool switch could delete |
| AV-001 walk-shell.json / .mjs | PASSED | none |
| AV-002 walk-open.json | PASSED | none |

Their pictures and logs were regenerated by the runs and restored from git, so the earlier phases' evidence stays as
recorded.

## External freshness

Not applicable — no dependency changed (Qt 6.8.3, FFmpeg 7.1.5, the toolchain as at AV-001).

## Files and line counts

New: `src/core/{CropShape,AspectRatio}.{h,cpp}`, `src/render/MaskBuilder.{h,cpp}`,
`src/ui/tools/{CropHandles.h,CropHandles.cpp,CropTool.cpp}`, `src/ui/options/CropOptions.cpp`,
`src/ui/panels/CropPanel.cpp`, `tests/test-{aspect,mask}.cpp`. Changed: `Project.*`, `ProjectJson.*`, `Geometry.*`,
`Commands.h`, `CanvasOverlay.cpp`, `CanvasView.cpp`, `OptionsBar.*`, `SelectionOptions.cpp`, `SidePanel.*`,
`Tool.h`, `MainWindowCanvas.cpp`, `WalkRunner.*`, `WalkSteps.cpp`, the CMake files, `test-project-json.cpp`. The
largest new file `CropTool.cpp` 267 lines; check-loc: every code file under 600, all new files under 300.

## Recovery log

Attempt 1 RESOLVED — gate: build; cause: `class QComboBox* createRatioBox(…)` inside the namespace declared
`avloop::QComboBox`; the forward declaration moved to global scope.
Attempt 2 RESOLVED — gate: test-mask (2 of 6); cause: the tests' expectations, not the mask — pixel centres put the
axis edge between pixels, and the first feathered pixel is 1/20 (13), not 1/40; the band is now tested geometrically.
Attempt 3 RESOLVED — gate: walk-crop (1 of 89); cause: every tool's options row exists at once, so a probe registered
per kind read the last row; probes now carry the row's name.

## Deviations from the phase contract

- **Code budget**: 550 planned; ~1,040 new lines in `src` + ~230 changed (comments and blank lines included). The
  options bar, the panel and the handle maths came larger than planned; the named seam (AV-004a: snapping, guides,
  the panel) would move ~350. Recorded instead of split — tests and the walk were green in one pass.
- **Not on the canvas**: the Feather's softness (R-22) — the Preview (AV-008) and the export (AV-009) apply the mask.

## Blocked legs (if any)

None.

## Staged paths

- `src/`, `tests/`, `tools/walks/readme-shots.json`, `README.md`, `docs/SPEC.md`, `docs/lore/avloop.md`,
  `docs/screenshots/main-window.png`, `phases/26-10-26-034934-AvLoop/`.

## Next

AV-005 — The timeline, promoted to IN_PROGRESS in the same turn. No avloop process, session cache or walk temp file
left; the owner's focus untouched.
