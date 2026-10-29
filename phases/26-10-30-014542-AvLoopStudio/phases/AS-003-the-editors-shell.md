---
phase: AS-003
title: "The editor's shell"
status: DONE
owner_item: 3
track: A
checkpoint_id: CP-A1
is_checkpoint: false
risk: High
permission_class: VERIFY_ENVIRONMENT_FIRST
code_budget: 900
depends_on: [AS-002]
unlocks: [AS-004, AS-009]
requirements: [REQ-AS-003]
read_scope:
  - phases/26-10-30-014542-AvLoopStudio/control/03_SHAPES_THE_EDITOR.md
  - src/ui/MainWindow.cpp
  - src/ui/panels/SidePanel.cpp
  - src/ui/canvas/CanvasOverlay.cpp
  - spec: §3, §6, §11
---

# The editor's shell

Canonical current execution state: `../AUTONOMOUS_STATE.md`

**Goal.** Premiere's layout: the Project panel (the media bin), the Program monitor (the composite at the playhead), the Effect Controls, the Timeline; the Frame tool for the sequence frame. (`../README.md` §2 item 3; `control/03` §3).

**Ground truth**

- The owner's words (`../README.md` §1, 2026-10-30).
- AvLoop's delivered behaviour (`docs/SPEC.md` §0, D-001 … D-029) and its readings, which stand unless `../READINGS.md` says otherwise.

**Do**

1. `ui/panels/ProjectPanel` (import, list, thumbnails, drag to a track, Insert and Overwrite); the main window's splitters, remembered.
2. The Program monitor: the canvas drawing the composite at the playhead (the SequenceRenderer's frame when paused, the live look while playing).
3. The Frame tool (F): the frame's presets and custom size; walks `walk-shell.json`.

**Do not**

Lose a feature of AvLoop's without its new home (the panels move, they do not go).

**Done when**

Build 0 warnings; ctest; format, icons, secrets, LOC PASS; the phase's walks PASSED; README current; pushed with CI green.

**Decisions this phase must record**

D-033 (`docs/SPEC.md` §0); its readings in `../READINGS.md`.

**Validation**

walk-shell.json; AvLoop's shell walk again with its counts. `node control/lab/walk-phase.mjs execution/AS-003-walk …`; check-loc; check-secrets; `validate-phase-system.mjs`.

**Seam if this splits.** AS-003a — the second half of its **Do** after the first.
