---
phase: AS-001
title: "The sequence"
status: IN_PROGRESS
owner_item: 1
track: A
checkpoint_id: CP-A1
is_checkpoint: false
risk: High
permission_class: VERIFY_ENVIRONMENT_FIRST
code_budget: 900
depends_on: []
unlocks: [AS-002, AS-009]
requirements: [REQ-AS-001]
read_scope:
  - phases/26-10-30-014542-AvLoopStudio/control/03_SHAPES_THE_EDITOR.md
  - src/core/VideoLayer.h
  - src/core/Project.h
  - src/core/ProjectMigrate.cpp
  - src/core/MaskShape.h
  - spec: §3, §6, §11
---

# The sequence

Canonical current execution state: `../AUTONOMOUS_STATE.md`

**Goal.** The model of a mini Premiere Pro: a sequence of video tracks, each of clips equal to each other — media, place in time, In/Out, motion and opacity keyframes, effects, masks; the edits as pure functions; one Undo step an edit; the file's version 4 reading every AvLoop file. (`../README.md` §2 item 1; `control/03` §1).

**Ground truth**

- The owner's words (`../README.md` §1, 2026-10-30).
- AvLoop's delivered behaviour (`docs/SPEC.md` §0, D-001 … D-029) and its readings, which stand unless `../READINGS.md` says otherwise.

**Do**

1. `core/Sequence` (Sequence, Track, Clip) and `core/SequenceEdit` (split, trims, move, remove, ripple remove, insert, overwrite, the clip at a time, the topmost at a point) — pure, tested (RS-1 … RS-4).
2. `core/SequenceJson`: the `"sequence"` section; `ProjectMigrate` 3 → 4 (RS-5); `Project` holds the sequence beside AvLoop's fields while the UI moves over (AS-003 … AS-006): `sequence()`, `changeSequence`.
3. Tests `test-sequence`; the render walk of AS-002 reads the migrated files.

**Do not**

Remove AvLoop's model before the UI stops reading it; rename a key of version 3; let a migrated file render differently (AS-002 checks it).

**Done when**

Build 0 warnings; ctest; format, icons, secrets, LOC PASS; the phase's walks PASSED; README current; pushed with CI green.

**Decisions this phase must record**

D-031 (`docs/SPEC.md` §0); its readings in `../READINGS.md`.

**Validation**

test-sequence: every edit's cases, the overlap rule, a version-3 file of each kind (a crop, shapes, an inverse, layers, keys) migrated and round-tripped. `node control/lab/walk-phase.mjs execution/AS-001-walk …`; check-loc; check-secrets; `validate-phase-system.mjs`.

**Seam if this splits.** AS-001a — the second half of its **Do** after the first.
