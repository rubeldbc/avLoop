---
phase: AV-018
title: "The wheel and the keyframe copy"
status: DONE
owner_item: 16
track: B
checkpoint_id: CP-B1
is_checkpoint: false
late: true
risk: Low
permission_class: VERIFY_ENVIRONMENT_FIRST
code_budget: 120
depends_on: [AV-017]
unlocks: []
requirements: [REQ-AV-023]
read_scope:
  - phases/26-10-26-034934-AvLoop/control/19_SHAPES_THE_WHEEL_AND_KEY_COPY.md
  - docs/SPEC.md
  - src/ui/canvas/Canvas.cpp
  - src/ui/timeline/TimelineInput.cpp
  - src/core/Keyframes.cpp
  - spec: §6.3, §6.4, §7.2, §11.2
---

# The wheel and the keyframe copy

Canonical current execution state: `../AUTONOMOUS_STATE.md`

**Goal.** The mouse wheel alone zooms the canvas in and out about the pointer, and an Alt+drag of a keyframe's
diamond on the timeline copies it to where it is dropped (`../README.md` §1 item 16; D-016). A late phase: the
owner's two wishes of one sitting, after AV-017 closed.

**Ground truth**

- The owner's words (`../README.md` §1, 2026-10-27).
- Spec §6.3 (the canvas's zoom steps, the wheel in every tool), §6.4 and §7.2 (the keyframe row), §11.2 (Undo).

**Do**

1. `Canvas::wheelEvent`: the wheel, Ctrl or not, zooms a step a notch about the pointer; finer wheels add up to a
   notch; Shift+wheel and a sideways wheel pan across (R-63).
2. `keyframes::copied`, `Project::copyKeyframe`; the timeline's Alt+drag of a diamond copies it, one Undo step; the
   copy cursor with Alt over a diamond (R-64).
3. Tests: the copy in `test-keyframes`; the walks `walk-wheel.json` and `walk-key-copy.json`.

**Do not**

Change the timeline's wheel; change the zoom steps; add an Undo step for the view.

**Done when**

Build 0 warnings; ctest; format, icons, secrets, LOC PASS; both walks PASSED, AV-003's `walk-selection.json` and
AV-011's `walk-keyframes.json` PASSED again; README current; pushed with CI green.

**Decisions this phase must record**

D-016 (spec §0); R-63 … R-65.

**Validation**

`node control/lab/walk-phase.mjs execution/AV-018-walk walk-wheel.json walk-key-copy.json`; the regressions;
check-loc; check-secrets; `validate-phase-system.mjs`.

**Seam if this splits.** AV-018a — the keyframe copy — after the wheel.
