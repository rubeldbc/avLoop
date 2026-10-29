---
phase: AS-007
title: "The export"
status: IN_PROGRESS
owner_item: 7
track: A
checkpoint_id: CP-A1
is_checkpoint: false
risk: Medium
permission_class: VERIFY_ENVIRONMENT_FIRST
code_budget: 500
depends_on: [AS-006]
unlocks: [AS-008, AS-009]
requirements: [REQ-AS-007]
read_scope:
  - phases/26-10-30-014542-AvLoopStudio/control/03_SHAPES_THE_EDITOR.md
  - src/export/ExportJob.cpp
  - src/ui/MainWindowExport.cpp
  - src/cli/CliExport.cpp
  - spec: §3, §6, §11
---

# The export

Canonical current execution state: `../AUTONOMOUS_STATE.md`

**Goal.** The sequence's In → Out (or a loop of it) exported as WebM VP9/VP8 with alpha, ProRes 4444, APNG, GIF or MP4; transparent where nothing covers or a mask cuts; the batch and avloop-cli. (`../README.md` §2 item 7; `control/03` §7).

**Ground truth**

- The owner's words (`../README.md` §1, 2026-10-30).
- AvLoop's delivered behaviour (`docs/SPEC.md` §0, D-001 … D-029) and its readings, which stand unless `../READINGS.md` says otherwise.

**Do**

1. The export of the sequence through AS-002's renderer; Alpha Auto from the composite; the loop mode None by default (RS-8).
2. Walk `walk-export.mjs`: each format, the alpha read back by libvpx and ffprobe.

**Do not**

Write a file the check has not read back.

**Done when**

Build 0 warnings; ctest; format, icons, secrets, LOC PASS; the phase's walks PASSED; README current; pushed with CI green.

**Decisions this phase must record**

D-037 (`docs/SPEC.md` §0); its readings in `../READINGS.md`.

**Validation**

walk-export.mjs; AvLoop's export walks on migrated projects. `node control/lab/walk-phase.mjs execution/AS-007-walk …`; check-loc; check-secrets; `validate-phase-system.mjs`.

**Seam if this splits.** AS-007a — the second half of its **Do** after the first.
