---
phase: AV-016
title: "CP-B1 the whole walked"
status: IN_PROGRESS
owner_item: 0
track: B
checkpoint_id: CP-B1
is_checkpoint: true
risk: High
permission_class: VERIFY_ENVIRONMENT_FIRST
code_budget: 200
depends_on: [AV-011, AV-012, AV-013, AV-014, AV-015]
unlocks: []
requirements: [REQ-AV-014, REQ-AV-019, REQ-AV-021]
read_scope:
  - phases/26-10-26-034934-AvLoop/control/02_MAJOR_CHECKPOINTS.md
  - phases/26-10-26-034934-AvLoop/control/04_THE_LAB_AND_THE_WALK.md
  - docs/SPEC.md
  - spec: §14, §2.8, §2.9
---

# CP-B1 the whole walked

Canonical current execution state: `../AUTONOMOUS_STATE.md`

**Goal.** Every situation S1–S15 on one fresh Release build installed from the installer; the GitHub Release
`v1.0.0` with the installer (reading R-5); the final README; the Completion record written.

**Ground truth**

- `control/02_MAJOR_CHECKPOINTS.md`; every record AV-001 … AV-015; `OWNER-ACTIONS.md` (the agent's decisions).

**Do**

1. `scripts\build.ps1 -Fresh -Config Release`; `scripts\deploy.ps1`; the installer built and installed silently into a
   temp folder; every walk run against the installed app where it can be (S1–S15), the rest against the build.
2. ASan and leak legs again; the memory and GPU walks; CI green.
3. `gh release create v1.0.0` with the installer and its SHA-256; README's Download link to it; final screenshots.
4. `AUTONOMOUS_STATE.md`'s Completion record: what was built, the readings taken, what was dropped and why; every
   `BLOCKED_EXTERNAL` cleared.

**Do not**

Build a feature; weaken a gate; leave an install, a process or a temp file behind.

**Done when**

S1–S15 each with its observed values; the Release exists with the installer; README final; the programme COMPLETE.

**Decisions this phase must record**

R-5 taken (the Release); the Completion record.

**Validation**

`control/02` §1 entire; `node control/lab/walk-phase.mjs execution/AV-016-walk <every walk>`; `gh release view
v1.0.0`.

**Seam if this splits.** AV-016a — the Release and the final README — after the walks on the installed build.
