---
phase: AV-003
title: "The canvas and the Selection"
status: DONE
owner_item: 3
track: A
checkpoint_id: CP-A1
is_checkpoint: false
risk: High
permission_class: VERIFY_ENVIRONMENT_FIRST
code_budget: 550
depends_on: [AV-002]
unlocks: [AV-004, AV-015]
requirements: [REQ-AV-003, REQ-AV-017]
read_scope:
  - phases/26-10-26-034934-AvLoop/control/06_SHAPES_THE_CANVAS.md
  - docs/SPEC.md
  - spec: §3.2, §3.3, §6, §6.2, §6.3, §11.2
  - after AV-002: src/media/Decoder.h
  - after AV-002: src/media/FrameCache.h
---

# The canvas and the Selection

Canonical current execution state: `../AUTONOMOUS_STATE.md`

**Goal.** The video stands on an unbounded canvas in scene space and the Selection tool moves and scales it: the GL
canvas (`QOpenGLWidget`) with zoom and pan, the `Tool` interface and `ToolManager`, Zoom (Z), Hand (H), Selection (V)
with its options bar, the project model and Undo's base (`../README.md` §2 item 3; D-003).

**Ground truth**

- Spec §3.3: scene space, `VideoTransform` (centre, scale 0.05–10, flips), the video rectangle; §6.2, §6.3.
- The decoder and cache of AV-002 give normalized RGBA frames; the model changes only through Undo commands.

**Do**

1. `core/Geometry`, `core/VideoTransform`, `core/Project` (aspects, `changed(Aspect)`), `core/Commands` (SetTransform
   with `mergeWith` in a 500 ms window; the stack limit 200), `core/ProjectJson` (version 1 read/write of what the
   model holds, unknown keys kept in `extra`) — the file's UX is AV-010's.
2. `ui/canvas/Canvas` + `CanvasView.cpp` (scene↔view, zoom steps, Ctrl+wheel around the cursor, Ctrl+0/1/=/-, wheel
   and Shift+wheel pan, middle-drag pan) + `CanvasGl.cpp` (the frame as a texture; the checkerboard where nothing is
   drawn) + `CanvasOverlay.cpp` (the video's quiet blue-white dashed border, its four handles).
3. `ui/tools/Tool.h`, `ToolManager`, `SelectionTool.cpp` (drag moves; corner handles scale about the opposite corner,
   Alt from the centre), `ZoomTool.cpp` (click, Alt+click, drag rectangle), `HandTool.cpp`; the tool rail's buttons
   with tooltips "Name (Key)"; the active tool orange.
4. `ui/options/OptionsBar` + `SelectionOptions.cpp` (X, Y, Scale %, W, H, Lock, Fit, Fill, Center, Flip H, Flip V,
   Reset; the upscale warning icon) + `ZoomOptions.cpp`; `ui/widgets/StepSpinBox` (wheel, arrows, Shift ×10).
5. `tests/test-geometry.cpp`, `test-commands.cpp`, `test-project-json.cpp`.

**Do not**

Crop (AV-004) — Fit/Fill/Center act on the whole video rectangle until a crop exists; keyframes (AV-011).

**Done when**

Build, ctest, gates PASS; S4 — a walk moves and scales the video, presses Fit/Fill/Center/Flip/Reset and reads the
options bar's values against the model; the upscale warning shows above 100%; Undo/Redo of each; Zoom and Hand never
enter Undo; screenshots of the canvas at 100% and 150% DPI.

**Decisions this phase must record**

D-003 (spec §0); reading R-2 (W/H unlocked never distorts), R-3 (Space is Play, pan by middle mouse or H).

**Validation**

`scripts\build.ps1`; `ctest … -R "geometry|commands|project"`; `node control/lab/walk-phase.mjs
execution/AV-003-walk walk-selection.json`; format; check-icons; check-loc; check-secrets.

**Seam if this splits.** AV-003a — the options bar and Zoom/Hand tools — after the canvas, the model and the
Selection's drag.
