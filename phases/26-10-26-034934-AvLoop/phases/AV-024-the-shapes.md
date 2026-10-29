---
phase: AV-024
title: "The shapes"
status: NOT_STARTED
owner_item: 22
track: B
checkpoint_id: CP-B1
is_checkpoint: false
late: true
risk: High
permission_class: VERIFY_ENVIRONMENT_FIRST
code_budget: 1300
depends_on: [AV-023]
unlocks: [AV-025]
requirements: [REQ-AV-029]
read_scope:
  - phases/26-10-26-034934-AvLoop/control/25_SHAPES_THE_SHAPES_AND_THE_PEN.md
  - phases/26-10-26-034934-AvLoop/control/07_SHAPES_THE_CROP.md
  - docs/SPEC.md
  - src/core/CropShape.h
  - src/render/MaskBuilder.cpp
  - src/ui/tools/CropTool.cpp
  - src/ui/tools/CropHandles.cpp
  - src/ui/tools/Tool.h
  - src/ui/canvas/CanvasOverlay.cpp
  - src/ui/options/CropOptions.cpp
  - src/ui/panels/CropPanel.cpp
  - src/ui/MainWindow.cpp
  - spec: §3.3, §5.3, §6, §6.1, §11.1, §11.2
---

# The shapes

Canonical current execution state: `../AUTONOMOUS_STATE.md`

**Goal.** Shapes inside the crop's frame that select or cut: rectangles, rounded rectangles and ellipses, polygons of
clicked points and freehand outlines — drawn, selected, moved, resized and deleted by the Shape tool (S), listed in
the Crop section, masked in the Preview and every export (`../README.md` §1 item 22; D-022). A late phase: the
owner's wish of 2026-10-29, [3]; the Pen is AV-025.

**Ground truth**

- The owner's words (`../README.md` §1, 2026-10-29).
- D-004 and `control/07` (the crop tools, the handles, the snapping, the mask); D-021 (the inverse, the file's
  version 3); AVADR-0001 (the one mask both back ends use); AVADR-0002 (at most 64 shapes).

**Do**

1. `core/MaskShape` and `CropShape::shapes` (Rect, Rounded, Ellipse, Polygon; Select or Cut); `needsAlpha`; the
   file's `"shapes"` under `crop`, version 3 (R-82, R-83); one Undo step an edit.
2. `MaskBuilder`: each shape a QPainterPath raster into the running mask, the frame's coverage over it; no shapes
   byte for byte the old mask (R-84).
3. `tools/ShapeTool.cpp` and `ShapeEdit.cpp` (the drawing of the five kinds, the selection, the move, the handles,
   the polygon's anchors, Delete, arrows, Esc; the crop's snapping) and `options/ShapeOptions.cpp` (R-85).
4. The overlay's dim by the hard path booleans and the outlines; the Crop section's Shapes list and the selected
   shape's rows; the icons shape, polygon, freehand, cut (R-87); the rail's S button; the shortcut overlay.
5. Tests; `walk-shapes.json`, `walk-shapes-render.mjs`; the README's feature list and a picture.

**Do not**

Let a project without shapes render differently by a byte; draw the mask by anything but `MaskBuilder`; keyframe a
shape; let the dim show feather (R-22); pass 600 lines — `CropTool.cpp` (274) and `CanvasOverlay.cpp` (63) are read
before they grow; the tool's files aim under 450 each.

**Done when**

Build 0 warnings; ctest; format, icons, secrets, LOC PASS; `walk-shapes.json` and `walk-shapes-render.mjs`
PASSED; the walks of the shell (AV-001, the icon count), the crop (AV-004), the renderer (AV-007), the project file
(AV-010) and the inverse (AV-023) PASSED again; README current; pushed with CI green.

**Decisions this phase must record**

D-022 (spec §0, with AV-025); R-82 … R-85, R-87.

**Validation**

`node control/lab/walk-phase.mjs execution/AV-024-walk walk-shapes.json walk-shapes-render.mjs`; the regressions;
check-loc; check-secrets; `validate-phase-system.mjs`.

**Seam if this splits.** AV-024a — the Polygon and Freehand kinds — after the standard shapes, the mask, the list
and the file.
