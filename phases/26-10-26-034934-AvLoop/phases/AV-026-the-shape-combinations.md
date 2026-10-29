---
phase: AV-026
title: "The shape combinations"
status: IN_PROGRESS
owner_item: 23
track: B
checkpoint_id: CP-B1
is_checkpoint: false
late: true
risk: Medium
permission_class: VERIFY_ENVIRONMENT_FIRST
code_budget: 500
depends_on: [AV-025]
unlocks: [AV-027]
requirements: [REQ-AV-030]
read_scope:
  - phases/26-10-26-034934-AvLoop/control/26_SHAPES_THE_SHAPE_COMBINATIONS.md
  - phases/26-10-26-034934-AvLoop/control/25_SHAPES_THE_SHAPES_AND_THE_PEN.md
  - docs/SPEC.md
  - after AV-024: src/core/MaskShape.h
  - after AV-024: src/ui/tools/ShapeTool.cpp
  - after AV-024: src/ui/options/ShapeOptions.cpp
  - after AV-025: src/ui/tools/PenTool.cpp
  - src/render/MaskBuilder.cpp
  - spec: §6, §11.1, §11.2
---

# The shape combinations

Canonical current execution state: `../AUTONOMOUS_STATE.md`

**Goal.** New shapes from existing ones, as Illustrator's Pathfinder: live — each shape Add, Cut, Keep only or
Exclude against the shapes below it — and baked — the selected shapes combined into one path by Unite, Minus front,
Intersect or Exclude, then edited and used as a selection like any shape (`../README.md` §1 item 23; D-023). A late
phase: the owner's wish of 2026-10-29, [4].

**Ground truth**

- The owner's words (`../README.md` §1, 2026-10-29).
- AV-024's roles and running mask (`control/25` §3), AV-025's paths; Qt's `QPainterPath::united`, `subtracted`,
  `intersected`.

**Do**

1. `MaskShape::Role` widened to Add, Cut, KeepOnly, Exclude (`"select"` read as `"add"`); the running mask's four
   ops; the multi-selection (`CropShape::selected` a list; Shift+click, Ctrl+A, the marquee) (R-88).
2. `core/ShapeCombine` (pure): the four path booleans, subpaths kept, a standard shape as its path (R-89); the
   Combine command, one Undo step.
3. The options bars' Role and Combine ▸ menu; the Crop section's Role row, the role icons add, keep, exclude (cut
   from AV-024), the Combine… button; the overlay's dim by the four ops.
4. Tests (`test-mask`, `test-shapes`); `walk-combine.json`, `walk-combine-render.mjs`; the README's feature list.

**Do not**

Change AV-024's and AV-025's bytes for Select/Cut shapes (Add = max, Cut = ×(1 − s)); add Pathfinder's Divide,
Trim, Merge, Crop or Outline (not asked); rename `"role"`'s written values.

**Done when**

Build 0 warnings; ctest; format, icons, secrets, LOC PASS; `walk-combine.json` and `walk-combine-render.mjs`
PASSED; the walks of the shell (AV-001), the shapes (AV-024) and the pen (AV-025) PASSED again; README current;
pushed with CI green.

**Decisions this phase must record**

D-023 (spec §0); R-88 … R-89.

**Validation**

`node control/lab/walk-phase.mjs execution/AV-026-walk walk-combine.json walk-combine-render.mjs`; the
regressions; check-loc; check-secrets; `validate-phase-system.mjs`.

**Seam if this splits.** AV-026a — Combine (the baking) — after the live roles and the multi-selection.
