---
phase: AS-002
title: "The composite"
status: IN_PROGRESS
owner_item: 2
track: A
checkpoint_id: CP-A1
is_checkpoint: false
risk: High
permission_class: VERIFY_ENVIRONMENT_FIRST
code_budget: 800
depends_on: [AS-001]
unlocks: [AS-003, AS-009]
requirements: [REQ-AS-002]
read_scope:
  - phases/26-10-30-014542-AvLoopStudio/control/03_SHAPES_THE_EDITOR.md
  - src/render/FrameRenderer.cpp
  - src/render/LayerPass.cpp
  - src/render/MaskBuilder.cpp
  - src/core/LoopMap.h
  - spec: §3, §6, §11
---

# The composite

Canonical current execution state: `../AUTONOMOUS_STATE.md`

**Goal.** One renderer for a sequence time: every clip present, bottom to top, decoded, its effects, its masks, its keyed motion and opacity, over a transparent frame; the loop engine over In/Out; the Preview, the export and avloop-cli made by it. (`../README.md` §2 item 2; `control/03` §2).

**Ground truth**

- The owner's words (`../README.md` §1, 2026-10-30).
- AvLoop's delivered behaviour (`docs/SPEC.md` §0, D-001 … D-029) and its readings, which stand unless `../READINGS.md` says otherwise.

**Do**

1. `render/SequenceRenderer`: the decoder pool (at most 8), a clip's look, its mask (MaskBuilder at its own size), the over (RS-6).
2. `RenderJob` and `FrameRenderer` take the sequence: output frame k → sequence time(s) → the composite; AvLoop's single path kept for the monitor's live look (RS-7).
3. Tests; `walk-composite-render.mjs`: AvLoop's render walks' projects (migrated) byte for byte or within 2/255, two clips cut on V1, a hole in V2 showing V1, GPU = CPU.

**Do not**

Draw an output frame anywhere but the SequenceRenderer; hold more than 8 decoders; let the memory walk grow.

**Done when**

Build 0 warnings; ctest; format, icons, secrets, LOC PASS; the phase's walks PASSED; README current; pushed with CI green.

**Decisions this phase must record**

D-032 (`docs/SPEC.md` §0); its readings in `../READINGS.md`.

**Validation**

the render walk; AvLoop's walk-render, walk-loops, walk-layers-render again on migrated projects. `node control/lab/walk-phase.mjs execution/AS-002-walk …`; check-loc; check-secrets; `validate-phase-system.mjs`.

**Seam if this splits.** AS-002a — the second half of its **Do** after the first.
