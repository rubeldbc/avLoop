---
phase: AS-008
title: "The project"
status: DONE
owner_item: 8
track: A
checkpoint_id: CP-A1
is_checkpoint: false
risk: Medium
permission_class: VERIFY_ENVIRONMENT_FIRST
code_budget: 400
depends_on: [AS-007]
unlocks: [AS-009]
requirements: [REQ-AS-008]
read_scope:
  - phases/26-10-30-014542-AvLoopStudio/control/03_SHAPES_THE_EDITOR.md
  - src/ui/MainWindowProject.cpp
  - src/core/ProjectJson.cpp
  - spec: §3, §6, §11
---

# The project

Canonical current execution state: `../AUTONOMOUS_STATE.md`

**Goal.** The project of a sequence: save, open (any version), autosave and recovery, Recent, presets and settings — AvLoop's, carried over. (`../README.md` §2 item 8; `control/03` §8).

**Ground truth**

- The owner's words (`../README.md` §1, 2026-10-30).
- AvLoop's delivered behaviour (`docs/SPEC.md` §0, D-001 … D-029) and its readings, which stand unless `../READINGS.md` says otherwise.

**Do**

1. Save/Open of version 4; an AvLoop 1.x file opened as a sequence (AS-001's migration) with a toast saying so; autosave and recovery.
2. Walk `walk-project.mjs`: save → open the same frames; an AvLoop file of each kind opened.

**Do not**

Overwrite an AvLoop file without the user's Save.

**Done when**

Build 0 warnings; ctest; format, icons, secrets, LOC PASS; the phase's walks PASSED; README current; pushed with CI green.

**Decisions this phase must record**

D-038 (`docs/SPEC.md` §0); its readings in `../READINGS.md`.

**Validation**

walk-project.mjs. `node control/lab/walk-phase.mjs execution/AS-008-walk …`; check-loc; check-secrets; `validate-phase-system.mjs`.

**Seam if this splits.** AS-008a — the second half of its **Do** after the first.
