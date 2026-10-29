---
phase: AS-006
title: "The masks on the monitor"
status: IN_PROGRESS
owner_item: 6
track: A
checkpoint_id: CP-A1
is_checkpoint: false
risk: High
permission_class: VERIFY_ENVIRONMENT_FIRST
code_budget: 900
depends_on: [AS-005]
unlocks: [AS-007, AS-009]
requirements: [REQ-AS-006]
read_scope:
  - phases/26-10-30-014542-AvLoopStudio/control/03_SHAPES_THE_EDITOR.md
  - src/ui/tools/ShapeTool.cpp
  - src/ui/tools/PenTool.cpp
  - src/ui/canvas/CanvasOverlay.cpp
  - spec: §3, §6, §11
---

# The masks on the monitor

Canonical current execution state: `../AUTONOMOUS_STATE.md`

**Goal.** Masks drawn on the selected clip on the monitor — rectangles, ellipses, polygons, freehand, the Pen — Add, Subtract, Intersect, Difference, Inverted (a hole in any clip), feather, opacity; the monitor's selection moving and scaling clips. (`../README.md` §2 item 6; `control/03` §6).

**Ground truth**

- The owner's words (`../README.md` §1, 2026-10-30).
- AvLoop's delivered behaviour (`docs/SPEC.md` §0, D-001 … D-029) and its readings, which stand unless `../READINGS.md` says otherwise.

**Do**

1. AvLoop's S and P tools, Combine and Align over the selected clip's masks in its own pixels (its motion folded in); the dim of what its masks cut.
2. The Selection on the monitor: a click picks the topmost clip, its handles move and scale it (keyed when its stopwatch runs).
3. Walk `walk-masks.json` and its render walk (a hole in V2 showing V1, an inverted ellipse, a feathered cut).

**Do not**

Draw a mask anywhere but the selected clip; keep AvLoop's crop-frame masks once migrated.

**Done when**

Build 0 warnings; ctest; format, icons, secrets, LOC PASS; the phase's walks PASSED; README current; pushed with CI green.

**Decisions this phase must record**

D-036 (`docs/SPEC.md` §0); its readings in `../READINGS.md`.

**Validation**

walk-masks.json and its render walk. `node control/lab/walk-phase.mjs execution/AS-006-walk …`; check-loc; check-secrets; `validate-phase-system.mjs`.

**Seam if this splits.** AS-006a — the second half of its **Do** after the first.
