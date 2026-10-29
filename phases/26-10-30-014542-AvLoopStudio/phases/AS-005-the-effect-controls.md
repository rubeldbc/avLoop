---
phase: AS-005
title: "The Effect Controls"
status: DONE
owner_item: 5
track: A
checkpoint_id: CP-A1
is_checkpoint: false
risk: Medium
permission_class: VERIFY_ENVIRONMENT_FIRST
code_budget: 800
depends_on: [AS-004]
unlocks: [AS-006, AS-009]
requirements: [REQ-AS-005]
read_scope:
  - phases/26-10-30-014542-AvLoopStudio/control/03_SHAPES_THE_EDITOR.md
  - src/ui/panels/LayersPanel.cpp
  - src/ui/panels/EffectsPanel.cpp
  - spec: §3, §6, §11
---

# The Effect Controls

Canonical current execution state: `../AUTONOMOUS_STATE.md`

**Goal.** Premiere's Effect Controls for the selected clip: Motion, Opacity, Masks, the clip effects; stopwatches and keyframes on a small lane. (`../README.md` §2 item 5; `control/03` §5).

**Ground truth**

- The owner's words (`../README.md` §1, 2026-10-30).
- AvLoop's delivered behaviour (`docs/SPEC.md` §0, D-001 … D-029) and its readings, which stand unless `../READINGS.md` says otherwise.

**Do**

1. `ui/panels/EffectControls`: Motion, Opacity with stopwatches, ◀ ◆ ▶, the key lane; Masks' rows; the clip effects' rows (AvLoop's Clip effects section re-aimed at the clip).
2. Walk `walk-effect-controls.json` and its render walk (a keyed move and fade, an effect on one clip).

**Do not**

Let an edit with the stopwatch off key; let one clip's edit change another's.

**Done when**

Build 0 warnings; ctest; format, icons, secrets, LOC PASS; the phase's walks PASSED; README current; pushed with CI green.

**Decisions this phase must record**

D-035 (`docs/SPEC.md` §0); its readings in `../READINGS.md`.

**Validation**

walk-effect-controls.json and its render walk. `node control/lab/walk-phase.mjs execution/AS-005-walk …`; check-loc; check-secrets; `validate-phase-system.mjs`.

**Seam if this splits.** AS-005a — the second half of its **Do** after the first.
