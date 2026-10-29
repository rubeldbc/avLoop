---
phase: AV-032
title: "The sweep"
status: IN_PROGRESS
owner_item: 28
track: B
checkpoint_id: CP-B1
is_checkpoint: false
late: true
risk: Medium
permission_class: VERIFY_ENVIRONMENT_FIRST
code_budget: 100
depends_on: [AV-033]
unlocks: []
requirements: [REQ-AV-035]
read_scope:
  - phases/26-10-26-034934-AvLoop/execution/AV-016-EXECUTION.md
  - phases/26-10-26-034934-AvLoop/control/02_MAJOR_CHECKPOINTS.md
  - phases/26-10-26-034934-AvLoop/control/04_THE_LAB_AND_THE_WALK.md
  - spec: §2.6, §3.7.8
---

# The sweep

Canonical current execution state: `../AUTONOMOUS_STATE.md`

**Goal.** Everything walked again on one fresh build, as CP-A1 and CP-B1 did: every walk of AV-001 … AV-033, the
whole unit suite in Release, under AddressSanitizer and in Debug with the leak gate; then the build left fresh in
`build\` and pushed (`../README.md` §1 item 28; D-028). A late phase: the owner's word of 2026-10-30.

**Ground truth**

- The owner's words (`../README.md` §1, 2026-10-30): "push to main and complete all the task and test then rebuild and
  push".
- AV-015 and AV-016 (the checkpoints' sweeps: Release, ASan, Debug leaks, every walk); R-75 (no install walk over the
  owner's own AvLoop).

**Do**

1. `scripts\build.ps1 -Fresh` (Release, `build\`); every walk by `AV-032-walk/sweep.mjs`, the install and release walks
   skipped (R-75); a failure fixed (a corrective) before the close.
2. `ctest` in Release; `scripts\build.ps1 -Sanitize` and its ctest; `scripts\build.ps1 -Config Debug` and its ctest with
   `avloop-leaks`.
3. The record; the push; CI.

**Do not**

Install over the owner's AvLoop; weaken a walk to make it pass; publish a Release.

**Done when**

Every walk but the three skipped PASSED on the fresh build; Release, ASan and Debug ctest pass; gates PASS; pushed with
CI green.

**Decisions this phase must record**

D-028 (spec §0); R-105.

**Validation**

`node execution/AV-032-walk/sweep.mjs`; the three ctest runs; check-loc; check-secrets; `validate-phase-system.mjs`.

**Seam if this splits.** AV-032a — the ASan and Debug runs — after the Release sweep.
