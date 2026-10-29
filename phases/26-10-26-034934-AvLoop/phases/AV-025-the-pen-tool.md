---
phase: AV-025
title: "The pen tool"
status: NOT_STARTED
owner_item: 22
track: B
checkpoint_id: CP-B1
is_checkpoint: false
late: true
risk: High
permission_class: VERIFY_ENVIRONMENT_FIRST
code_budget: 800
depends_on: [AV-024]
unlocks: [AV-026]
requirements: [REQ-AV-029]
read_scope:
  - phases/26-10-26-034934-AvLoop/control/25_SHAPES_THE_SHAPES_AND_THE_PEN.md
  - docs/SPEC.md
  - after AV-024: src/core/MaskShape.h
  - after AV-024: src/ui/tools/ShapeTool.cpp
  - after AV-024: src/ui/tools/ShapeEdit.cpp
  - src/render/MaskBuilder.cpp
  - src/ui/tools/Tool.h
  - spec: §3.3, §5.3, §6, §11.1, §11.2
---

# The pen tool

Canonical current execution state: `../AUTONOMOUS_STATE.md`

**Goal.** Custom shapes as Illustrator's Pen makes them: a path of anchors with handles — corner and smooth anchors
drawn by clicks and drags, closed on the first anchor; anchors moved, added on a segment, deleted, converted; a
handle's pair turned together or broken with Alt — used as a selection or a cut like any shape (`../README.md` §1
item 22; D-022). A late phase: the owner's later word of 2026-10-29, [1].

**Ground truth**

- The owner's words (`../README.md` §1, 2026-10-29: "custom shape creation like illustrator … full pen tool").
- AV-024's model, tool and mask (`control/25` §2 … §5); Illustrator's Pen: click, click-drag, Alt on a handle,
  Alt+click to convert, click on a segment to add, Delete, Ctrl as the direct selection.

**Do**

1. `MaskShape::Kind::Path` and `PathPoint` (anchor, in and out handles, smooth); the file's `"points"` with handles;
   a polygon or a freehand shape edited by the Pen becomes a Path (R-86).
2. `tools/PenTool.cpp` and `PathEdit.cpp` (pure hit-testing and geometry: the nearest segment and its parameter,
   the mirrored handle, the conversion, the insertion by de Casteljau) — the drawing and the editing of `control/25`
   §4; the overlay's anchors and handles.
3. `MaskBuilder`: a cubic path's raster (QPainterPath::cubicTo, the 1 px edge); the pen icon (R-87); the rail's P;
   the shortcut overlay.
4. Tests; `walk-pen.json`, `walk-pen-render.mjs`; the README's feature list and a picture.

**Do not**

Add pressure, width profiles, brushes or strokes (not asked); let AV-024's shapes render differently by a byte;
pass 600 lines — `PenTool.cpp` and `PathEdit.cpp` aim under 450 each.

**Done when**

Build 0 warnings; ctest; format, icons, secrets, LOC PASS; `walk-pen.json` and `walk-pen-render.mjs` PASSED; the
walks of the shell (AV-001), the shapes (AV-024) and the renderer (AV-007) PASSED again; README current; pushed with
CI green.

**Decisions this phase must record**

D-022 (spec §0, with AV-024); R-86.

**Validation**

`node control/lab/walk-phase.mjs execution/AV-025-walk walk-pen.json walk-pen-render.mjs`; the regressions;
check-loc; check-secrets; `validate-phase-system.mjs`.

**Seam if this splits.** AV-025a — the editing of an existing path (add, delete, convert) — after the drawing and
the mask.
