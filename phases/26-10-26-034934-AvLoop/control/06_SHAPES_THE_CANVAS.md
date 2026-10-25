# 06 Shapes — the canvas and the Selection (item 3, AV-003)

Canonical current execution state: `../AUTONOMOUS_STATE.md`

Spec §3.2, §3.3, §6 (the tool table), §6.2, §6.3, §11.2. D-003.

## 1. The model (`core/`)

- `Project` (a `QObject`): source path and `MediaInfo` summary, range, `VideoTransform`, `CropShape` (AV-004),
  `LoopSettings` (AV-006), `OutputSettings`, `QualitySettings`, keyframes (AV-011), view, `extra` (unknown keys);
  `changed(Aspect)` with Aspect = Source, Range, Transform, Crop, Loop, Output, Quality, Keyframes, View.
- `VideoTransform`: centre (x, y), scale (0.05–10.0), flipH, flipV; `rect(videoSize)`; Fit/Fill/Center/Reset as
  pure functions of the crop (or the video rect when no crop).
- `Commands`: `QUndoCommand`s holding small values only (never images); SetTransform merges within 500 ms for one
  drag or repeated typing (`id()` + `mergeWith`); the stack's limit 200.
- `ProjectJson`: version 1 (spec §11.1's keys), read and write; unknown keys kept in `extra` and written back.

## 2. The canvas (`ui/canvas/`)

- `Canvas` (`QOpenGLWidget`): owns the view (zoom, pan) and the frame texture; `sceneToView` / `viewToScene`; the
  zoom steps 10 … 800%; Fit = the video (or crop) in view with a margin; `CanvasGl.cpp`: a shader drawing the frame
  quad with the transform (the same transform shader the renderer uses at AV-007) over a checkerboard where nothing
  is drawn, the outside of the scene `bg`; `CanvasOverlay.cpp`: QPainter over GL — the video's dashed border
  (`#C8D4FF` at 70%), its four corner handles when the Selection is active, the tool's overlay.
- Input routing: the active `Tool` gets press/move/release/key/wheel in scene coordinates; middle-drag, wheel,
  Shift+wheel and Ctrl+wheel are the canvas's own in any tool.
- The canvas shows the frame at the playhead (AV-005 moves it); until then the first frame.

## 3. The tools (`ui/tools/`)

- `Tool` interface: `id`, `activate`, `deactivate`, `mousePress/Move/Release`, `keyPress`, `paintOverlay`,
  `cursor`, `optionsWidget`. `ToolManager`: one active; shortcuts V, M, U, E, H, Z, K (I, O set In/Out without
  changing the tool); the rail's buttons checkable, the active orange.
- Selection (V): drag inside the video moves it; a corner handle scales about the opposite corner keeping the ratio
  (Alt = about the centre); with keyframes (AV-011) writes into the keyframe at the playhead.
- Zoom (Z): click in, Alt+click out, drag a rectangle; Hand (H): drag pans. Neither enters Undo.

## 4. The options bar (`ui/options/`)

`OptionsBar` hosts the active tool's widget in the top bar's slot. Selection: X, Y (video centre, scene px), Scale %
(5–1000, step 0.1), W, H (locked: one changes the other; unlocked: display only — R-2), Lock, Fit, Fill, Center,
Flip H, Flip V, Reset; the yellow `warning` icon beside Scale when the effective output scale > 100% ("Upscaled:
output may look soft"). Zoom: Fit, 100%, 200%, zoom %. `StepSpinBox`: wheel and ↑↓, Shift ×10, Consolas digits; edits
become one Undo step each burst.

## 5. Tests and walk

`test-geometry` (rects, fit/fill/center, scene↔view), `test-commands` (merge window, limit 200, undo/redo of each),
`test-project-json` (round trip, `extra` kept). Walk `walk-selection.json`: open a sample, drag the video, drag a
corner, Alt+drag, Fit/Fill/Center/Flip/Reset, read X/Y/Scale against the model, Ctrl+Z each, zoom/pan never in Undo,
pictures at 100% and 150%.
