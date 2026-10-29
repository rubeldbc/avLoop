---
phase: AS-004
title: "The timeline editor"
status: IN_PROGRESS
owner_item: 4
track: A
checkpoint_id: CP-A1
is_checkpoint: false
risk: High
permission_class: VERIFY_ENVIRONMENT_FIRST
code_budget: 1100
depends_on: [AS-003]
unlocks: [AS-005, AS-009]
requirements: [REQ-AS-004]
read_scope:
  - phases/26-10-30-014542-AvLoopStudio/control/03_SHAPES_THE_EDITOR.md
  - src/ui/timeline/Timeline.cpp
  - src/ui/timeline/TrackLanes.cpp
  - src/ui/timeline/TimelinePaint.cpp
  - spec: §3, §6, §11
---

# The timeline editor

Canonical current execution state: `../AUTONOMOUS_STATE.md`

**Goal.** Premiere's timeline: tracks V1 … V8 with headers, clip bars with thumbnails, selection, moves with snapping, trims, the Razor and Ctrl+K, Delete and ripple delete, the sequence In/Out. (`../README.md` §2 item 4; `control/03` §4).

**Ground truth**

- The owner's words (`../README.md` §1, 2026-10-30).
- AvLoop's delivered behaviour (`docs/SPEC.md` §0, D-001 … D-029) and its readings, which stand unless `../READINGS.md` says otherwise.

**Do**

1. `ui/timeline/SequenceTimeline*` (paint, input, headers) on AvLoop's ruler and playhead; the tools V and C; the keys.
2. Every edit through `sequenceedit` and one Undo step.
3. Walk `walk-timeline-edit.json`: import two videos, cut, move, trim, ripple, undo; the render walk of the result.

**Do not**

Let two clips overlap on a track; let an edit bypass Undo.

**Done when**

Build 0 warnings; ctest; format, icons, secrets, LOC PASS; the phase's walks PASSED; README current; pushed with CI green.

**Decisions this phase must record**

D-034 (`docs/SPEC.md` §0); its readings in `../READINGS.md`.

**Validation**

walk-timeline-edit.json and its render walk. `node control/lab/walk-phase.mjs execution/AS-004-walk …`; check-loc; check-secrets; `validate-phase-system.mjs`.

**Seam if this splits.** AS-004a — the second half of its **Do** after the first.
