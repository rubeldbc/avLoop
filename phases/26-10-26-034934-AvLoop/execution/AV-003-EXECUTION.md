# AV-003 — Execution Record

**Phase:** AV-003 — The canvas and the Selection (owner item 3)
**Status:** DONE
**Date:** 2026-10-26
**Phase start commit:** `2711480` (*[AV-002] The media*)
**ADR:** none owed (AVADR-0001's shared transform shader begins here; it is accepted at AV-007)
**Environment class:** local-disposable (this PC: `build/`, `samples/`, the walk's folder; lab windows never take focus)

## The owner's words

*"The Crop tool decides which part is exported. The Selection tool moves and scales the source video on the canvas."*
— *"Tools instead of buttons wherever possible."* — *"Use the GPU."*

## What was built

1. `src/core/VideoTransform` (centre, one scale 0.05–10, flips; rect, initial, Fit/Fill/Center, clamp),
   `core/Geometry` (output size rule, export frame, effective scale, corners), `core/Project` (aspects, source, view,
   extra; snapshot/restore; the Undo stack, limit 200), `core/Commands.h` (`ValueCommand<T>`: drag keys merge, field
   keys merge within 500 ms), `core/ProjectJson` (version 1 read/write; unknown keys kept at every level; relativePath;
   newer version refused; QSaveFile).
2. `src/render/Shaders` — the GLSL 3.30 transform, texture and checkerboard shaders (shared with AV-007).
3. `src/ui/canvas/Canvas` (+ `CanvasView.cpp` scene↔view, zoom steps, fit mode; `CanvasGl.cpp` OpenGL 3.3 drawing and
   the QPainter fallback; `CanvasGlState.h`; `CanvasOverlay.cpp`).
4. `src/ui/tools/Tool.h`, `ToolManager`, `SelectionTool.cpp`, `ZoomTool.cpp`, `HandTool.cpp`.
5. `src/ui/options/OptionsBar` (`OptionsRow`), `SelectionOptions.cpp`, `ZoomOptions.cpp`; `src/ui/widgets/StepSpinBox`.
6. `src/ui/MainWindowCanvas.cpp` — tools, Undo/Redo actions, the view's keys, canvas probes, the walk's `project` step;
   `MainWindow`/`MainWindowActions` — the canvas replaces the frame preview; `.avloop` files open (`openProject`,
   `--project`, a dropped file).
7. The lab: `WalkRunner::grabWidget` (pictures include QPainter on QOpenGLWidget), `placeQuietly` (no-focus sizing),
   `ensureActive` (Qt's active window only), the end-of-walk check "the owner's focus untouched"; lab windows show
   without activating. `scripts/build.ps1` runs windeployqt every build.
8. Tests: `test-geometry` (7), `test-commands` (8), `test-project-json` (6).

## Decisions recorded

- **D-003** — the canvas and the Selection as delivered; spec §0.
- **R-17** QUndoStack in core (Qt Gui linked for it alone); **R-18** load/Reset puts the top-left at (0,0); **R-19**
  Fit/Fill/Center without a crop act on the video's 100% frame; **R-20** the video's border and handles show with the
  Selection.
- **The canvas's GL context is 3.3 core** (spec §3.6); QPainter overlays draw into the same framebuffer.
- **The view's fit holds** through resizes until the user zooms or pans; a new source fits its first frame.
- **Lab windows never take the focus** (the owner works on this desktop): `WA_ShowWithoutActivating` + `SetWindowPos`
  with `SWP_NOACTIVATE` to the work area; the probe `window.maximized` is true when maximized or covering the work area
  (within Windows' invisible 8 px borders). The product still calls `showMaximized()` — AV-001's walk saw it maximized
  (1920×1009, `isMaximized` true) before lab windows stopped activating.

## Specification delta

- §0: `D-003`.
- Lore: nine rules (grab and QPainter on QOpenGLWidget; windeployqt every build; Python writes CRLF; QUndoStack in Qt
  Gui; walks never take focus; one view pixel of drag precision; invisible resize borders; `near`/`far` macros).

## Verification

| Command | Result |
| --- | --- |
| `scripts\build.ps1` | built, **0 warnings** |
| `ctest --test-dir build --output-on-failure --timeout 300 -R avloop` | **8 passed** (theme, icons, probe, decoder-seek, memory, geometry, commands, project-json), 0 failed, 54 s |
| `scripts\format.ps1 -Check` | PASS: 86 files |
| `node tools/icons/check-icons.mjs` | PASS: 43 icons |
| `node tools/check-secrets.mjs` | PASS: 251 files |
| `node control/phase-tools/check-loc.mjs` | PASS: 118 code files, the largest `src/ui/MainWindow.cpp` 348 lines |
| AV-002's CI run 40243381737 | green, 3 m 52 s (samples made on the runner) |

## The walk

```
node control/lab/walk-phase.mjs execution/AV-003-walk walk-selection.json walk-dpi.mjs
walk-selection.json (75 steps, real events on the canvas in scene px through the canvas mapper):
  counter-640p30 opens on the GL canvas (canvas.gl true), Selection active, x 320, scale 1
  drag (320,180)→(420,230): x 420 ±1, y 230 ±1, one Undo step, options X 420
  bottom-right handle (740,410)→(1060,590): scale 1.5 ±0.01 about the top-left, x 580, y 320, options 150%, the
  upscale warning shown, 2 steps
  Alt + top-left handle to (340,185): scale 0.75 about the centre (580,320), the warning gone
  Ctrl+Z → 1.5, Ctrl+Y → 0.75; Scale typed 200 → 2.0; Fit → 1.0 at (320,180); X typed 100; Center → 320; Flip H,
  Flip V on (picture); Reset → 100%, no flips; ≥ 9 Undo steps
  Z, Ctrl+1 → 100%; click → 150%; Alt+click → 100%; Ctrl+wheel → 150%; H drag 100 px → pan x 386.7; middle-drag; Ctrl+0
  → fit (pan x 320); Undo index unchanged at 10 through all of it
  "the owner's focus untouched: another program holds the focus"
walk-dpi.mjs: the canvas with alpha-vp9 at 100% and 150% scale, pictured.
Regressions on the same build: AV-001 walk-shell.json + walk-shell.mjs PASSED (S1 shots 1920×1009/1010); AV-002
walk-open.json PASSED; walk-memory-decode PASSED (earlier in the phase).
```

One run of walk-selection ended with "AvLoop is the foreground window" (the focus check's first day); the next run did
not reproduce it — most likely the owner's own click on the window that appeared. Recorded, not hidden.

## The pictures

| Picture | What | What it shows — and what was seen in it |
| --- | --- | --- |
| `AV-003-walk/selection-start.png` | the canvas, Selection active | the frame fitted in the canvas, the blue-white dashed border and four white handles at its corners; the options bar's row (X 320, Y 180, Scale 100%, W 640, H 360, the lock orange) |
| `AV-003-walk/selection-scaled.png` | after the handle drag | the video at 150% past the canvas's edge, the yellow warning triangle beside Scale, W 960.3 × H 540.1 |
| `AV-003-walk/selection-flipped.png` | Flip H + Flip V | the picture turned upside down and mirrored (testsrc2's clock bottom-right, reversed); both flip buttons orange |
| `AV-003-walk/canvas-100.png`, `canvas-150.png` | alpha WebM, 100% and 150% | the checkerboard shows through the transparent area around the disc; at 150% icons and text sharp, the border dashed |
| `docs/screenshots/main-window.png` | README | the demo gradient on the canvas with the Selection's border (1440 px, 266 KB) |

## The regressions

| Walk of before | Result | Mended |
| --- | --- | --- |
| AV-001 walk-shell.json / .mjs | PASSED | `window.maximized` reads "covers the work area" in lab runs; a `window.frame` reading added |
| AV-002 walk-open.json, walk-memory-decode.mjs | PASSED | none (`frame.size` now the canvas's frame) |

## External freshness

Not applicable — no dependency changed (Qt 6.8.3, FFmpeg 7.1.5, the toolchain as at AV-001).

## Files and line counts

New: `src/core/{VideoTransform,Geometry,Project,ProjectJson}.{h,cpp}`, `Commands.h`; `src/render/Shaders.{h,cpp}`;
`src/ui/canvas/{Canvas.h,Canvas.cpp,CanvasView.cpp,CanvasGl.cpp,CanvasGlState.h,CanvasOverlay.cpp}`;
`src/ui/tools/{Tool.h,ToolManager.h,ToolManager.cpp,SelectionTool.cpp,ZoomTool.cpp,HandTool.cpp}`;
`src/ui/options/{OptionsBar.h,OptionsBar.cpp,SelectionOptions.cpp,ZoomOptions.cpp}`;
`src/ui/widgets/StepSpinBox.{h,cpp}`; `src/ui/MainWindowCanvas.cpp`; three tests. Changed: `MainWindow.*` (348
lines, the largest code file), `MainWindowActions.cpp`, `WalkRunner.*`, `WalkSteps.cpp`, `app/main.cpp`,
`scripts/build.ps1`, the CMake files. check-loc: every code file under 600, all new files under 300.

## Recovery log

Attempt 1 RESOLVED — gate: build; causes: `unique_ptr<Gl>` of an incomplete type (the GL state moved to
`CanvasGlState.h`), a `data` name hiding `QWidget::data`, `lineEdit()` without its header.
Attempt 2 RESOLVED — gate: the app exited 0xC0000135 — windeployqt had not copied the OpenGL modules; it now runs
every build.
Attempt 3 RESOLVED — gate: pictures without the overlay — a minimal repro showed `grabFramebuffer()` holds QPainter's
drawing and `QWidget::grab()` does not; `WalkRunner::grabWidget` draws GL widgets in from their framebuffers.
Attempt 4 RESOLVED — gate: S1/S4 keys and menus failed when another program held the focus, and one run caught the
owner's keystrokes; walks no longer activate windows (Qt's active window only), sized without activation.

## Deviations from the phase contract

- **Code budget**: 550 planned; roughly 2,000 hand-written lines (model, JSON, canvas with GL and fallback, three tools,
  options, spin box, three tests) came. The named split (AV-003a: options bar + Zoom/Hand) would move ~400 lines.
  Recorded instead of split.
- **Lab-only**: walk windows are not truly maximized (focus); see Decisions.

## Blocked legs (if any)

None.

## Staged paths

- `src/`, `tests/`, `scripts/build.ps1`, `tools/walks/readme-shots.json`, `README.md`, `docs/SPEC.md`,
  `docs/lore/avloop.md`, `docs/screenshots/main-window.png`, `phases/26-10-26-034934-AvLoop/`.

## Next

AV-004 — The crop tools, promoted to IN_PROGRESS in the same turn. No avloop process, session cache or walk temp file
left; the owner's focus untouched.
