---
phase: AV-022
title: "The clip as cut"
status: DONE
owner_item: 20
track: B
checkpoint_id: CP-B1
is_checkpoint: false
late: true
risk: Low
permission_class: VERIFY_ENVIRONMENT_FIRST
code_budget: 250
depends_on: [AV-021]
unlocks: [AV-023]
requirements: [REQ-AV-027]
read_scope:
  - phases/26-10-26-034934-AvLoop/control/23_SHAPES_THE_CLIP_AS_CUT.md
  - docs/SPEC.md
  - src/core/LoopMap.cpp
  - src/core/LoopSettings.cpp
  - src/ui/panels/LoopPanel.cpp
  - src/ui/preview/PreviewWindow.cpp
  - src/ui/MainWindowExport.cpp
  - spec: §8.1, §8.2, §8.5, §9.2, §10.1
---

# The clip as cut

Canonical current execution state: `../AUTONOMOUS_STATE.md`

**Goal.** An export without a loop: the clip from In to Out as cut — no crossfade, no return, no seam — as a fifth
Mode, None, said so in the Loop section, the timeline, the Preview and the export's name (`../README.md` §1 item
20; D-020). A late phase: the owner's wish of 2026-10-29, [1].

**Ground truth**

- The owner's words (`../README.md` §1, 2026-10-29).
- Spec §8.1 and §8.2 (D, the four modes), §8.5 (the Loop panel), §9.2 (the Preview's seam marks), §10.1 (the
  export's name); D-006, D-008, D-009.

**Do**

1. `LoopSettings::Mode::None` (`"none"`), `loopmap::length` and `pick` for it, `LoopPlan` (R-77).
2. The Loop section's Mode list (None first), the rows hidden, the summary "Clip …"; the timeline without a band.
3. The Preview: no seam mark, Around seam disabled with its tip; the export's and the batch's suggested name
   `_clip_` (R-78).
4. Tests; `walk-cut.json`, `walk-cut-render.mjs`; the README's feature list.

**Do not**

Change Simple, Crossfade or the Ping-pongs by a frame; bump the file's version (AV-023 does, R-78); touch the Loop
Finder.

**Done when**

Build 0 warnings; ctest; format, icons, secrets, LOC PASS; `walk-cut.json` and `walk-cut-render.mjs` PASSED; the
walks of the loop engine (AV-006), the Preview (AV-008) and the export (AV-009) PASSED again; README current; pushed
with CI green.

**Decisions this phase must record**

D-020 (spec §0); R-77 … R-78.

**Validation**

`node control/lab/walk-phase.mjs execution/AV-022-walk walk-cut.json walk-cut-render.mjs`; the regressions;
check-loc; check-secrets; `validate-phase-system.mjs`.

**Seam if this splits.** AV-022a — the Preview's and the export's words — after the mode and the panel.
