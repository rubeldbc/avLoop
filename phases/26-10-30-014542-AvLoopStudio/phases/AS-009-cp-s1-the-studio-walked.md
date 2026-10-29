---
phase: AS-009
title: "CP-S1 the studio walked"
status: IN_PROGRESS
owner_item: 0
track: A
checkpoint_id: CP-A1
is_checkpoint: true
risk: High
permission_class: VERIFY_ENVIRONMENT_FIRST
code_budget: 150
depends_on: [AS-001, AS-002, AS-003, AS-004, AS-005, AS-006, AS-007, AS-008]
unlocks: []
requirements: [REQ-AS-009]
read_scope:
  - phases/26-10-30-014542-AvLoopStudio/control/02_CHECKPOINT.md
  - phases/26-10-26-034934-AvLoop/execution/AV-032-EXECUTION.md
  - spec: §2.6, §3.7.8
---

# CP-S1 the studio walked

Canonical current execution state: `../AUTONOMOUS_STATE.md`

**Goal.** The restructured app walked whole on one fresh build: every walk of this programme, AvLoop's walks that
still mean something, the memory walk, the Release, ASan and Debug-leak suites; AvLoop's deferred AV-032 finished.

**Ground truth**

- `control/02_CHECKPOINT.md`; AvLoop's CP-A1, CP-B1 and AV-032's sweep (`AV-032-walk/sweep.mjs`).

**Do**

1. `scripts\build.ps1 -Fresh`; every walk by a sweep; the three suites.
2. README with new pictures; the record.

**Do not**

Weaken a walk; install over the owner's AvLoop (R-75).

**Done when**

Every walk PASSED but those the record names with their reason; the three suites pass; pushed with CI green.

**Decisions this phase must record**

The checkpoint's line in `docs/SPEC.md` §0.

**Validation**

The sweep's log; the three ctest runs; `validate-phase-system.mjs`.

**Seam if this splits.** AS-009a — ASan and Debug after the Release sweep.
