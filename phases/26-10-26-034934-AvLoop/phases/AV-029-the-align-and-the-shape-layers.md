---
phase: AV-029
title: "The align tools and the shape layers"
status: IN_PROGRESS
owner_item: 26
track: B
checkpoint_id: CP-B1
is_checkpoint: false
late: true
risk: Medium
permission_class: VERIFY_ENVIRONMENT_FIRST
code_budget: 700
depends_on: [AV-028]
unlocks: [AV-030]
requirements: [REQ-AV-033]
read_scope:
  - phases/26-10-26-034934-AvLoop/control/29_SHAPES_THE_ALIGN_AND_THE_SHAPE_LAYERS.md
  - phases/26-10-26-034934-AvLoop/control/25_SHAPES_THE_SHAPES_AND_THE_PEN.md
  - docs/SPEC.md
  - after AV-024: src/core/MaskShape.h
  - after AV-024: src/ui/options/ShapeOptions.cpp
  - after AV-024: src/ui/tools/ShapeTool.cpp
  - src/ui/panels/CropPanel.cpp
  - src/render/MaskBuilder.cpp
  - spec: §5.1, §5.3, §6, §11.1, §11.2
---

# The align tools and the shape layers

Canonical current execution state: `../AUTONOMOUS_STATE.md`

**Goal.** The shapes as objects with Illustrator's Align — left, centre, right, top, middle, bottom, distribute, to
the selection or to the frame — and a layers panel for them: order, visibility, lock, names, multi-selection,
duplicate (`../README.md` §1 item 26; D-026). A late phase: the owner's later word of 2026-10-29, [2].

**Ground truth**

- The owner's words (`../README.md` §1, 2026-10-29: "align tool set for multiple object … layer system for
  multiple objects").
- AV-024 … AV-027's shapes, their order in the mask, the multi-selection of AV-026; Illustrator's Align and Layers
  panels.

**Do**

1. `core/ShapeAlign` (pure): the six aligns and the two distributes on bounding boxes, to the selection or the
   frame; the command, one Undo step (R-94, R-96).
2. `ui/options/AlignBar` on the S and P bars (two or more selected; one with To: Frame); the eight icons.
3. `ui/panels/ShapeLayers`: the rows (eye, lock, icon, name, role), the selection modes, the drag reorder, Up,
   Down, Duplicate, Delete, Combine…; `MaskShape::name`, `visible`, `locked` in the file, version 3; a hidden shape
   out of the mask and the dim, a locked one not selectable on the canvas (R-95).
4. Tests (`test-shapes`); `walk-align.json`, `walk-align-render.mjs`; the README's feature list and a picture.

**Do not**

Nest groups or sublayers (not asked); resize on align; let a hidden shape leave a byte in the mask; pass 600 lines —
`CropPanel.cpp` is split before the list grows (`ShapeLayers.cpp` its own file).

**Done when**

Build 0 warnings; ctest; format, icons, secrets, LOC PASS; `walk-align.json` and `walk-align-render.mjs` PASSED;
the walks of the shell (AV-001), the shapes (AV-024), the combinations (AV-026) and the soft parts (AV-027) PASSED
again; README current; pushed with CI green.

**Decisions this phase must record**

D-026 (spec §0); R-94 … R-96.

**Validation**

`node control/lab/walk-phase.mjs execution/AV-029-walk walk-align.json walk-align-render.mjs`; the regressions;
check-loc; check-secrets; `validate-phase-system.mjs`.

**Seam if this splits.** AV-029a — the layers panel — after the align tools.
