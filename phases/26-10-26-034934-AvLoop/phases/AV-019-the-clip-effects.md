---
phase: AV-019
title: "The clip effects"
status: DONE
owner_item: 17
track: B
checkpoint_id: CP-B1
is_checkpoint: false
late: true
risk: Medium
permission_class: VERIFY_ENVIRONMENT_FIRST
code_budget: 900
depends_on: [AV-018]
unlocks: []
requirements: [REQ-AV-024]
read_scope:
  - phases/26-10-26-034934-AvLoop/control/20_SHAPES_THE_CLIP_EFFECTS.md
  - docs/SPEC.md
  - src/render/FrameRenderer.cpp
  - src/render/GlRenderer.cpp
  - src/ui/canvas/CanvasGl.cpp
  - src/core/ProjectJson.cpp
  - spec: §3.4, §3.6, §5.1, §11.1, §11.2
---

# The clip effects

Canonical current execution state: `../AUTONOMOUS_STATE.md`

**Goal.** A Clip effects section in the side panel: colour correction and looks applied to the clip before it is
placed — on the canvas, in the Preview, in every export and in avloop-cli alike (`../README.md` §1 item 17; D-017).
A late phase: the owner's wish after AV-018 closed.

**Ground truth**

- The owner's words (`../README.md` §1, 2026-10-27).
- Spec §3.4 (the output frame's steps), §3.6 and AVADR-0001 (one renderer, GPU and CPU agreeing), §5.1 (the side
  panel), §11.1 (the project file; a format change bumps the version with a migration), §11.2 (Undo).

**Do**

1. `core/ClipEffects`: the eighteen values and On, their maths, the file's keys (R-66).
2. `render/EffectsPass`: the effects on each source frame on the CPU, before both back ends; the GPU's texture pool
   keyed by the look too (R-67).
3. The project: `effects`, one Undo step a field, a new video without; the file's version 2 with its migration; Alpha
   Auto on when Opacity is under 100 % (R-68, R-69).
4. The canvas: the same effects by its shader (a 3D colour table, Gaussian taps); the QPainter fallback by the pass.
5. The panel's Clip effects section (R-70); tests; the walks; the README and its pictures.

**Do not**

Draw the export or the Preview by anything but the FrameRenderer; let a neutral setting change a byte; rename a key.

**Done when**

Build 0 warnings; ctest; format, icons, secrets, LOC PASS; `walk-effects.json` and `walk-effects-render.mjs` PASSED,
the earlier walks of the canvas, the renderer, the Preview, the project file, the keyframes, the batch and the formats
PASSED again; README current with its pictures; pushed with CI green.

**Decisions this phase must record**

D-017 (spec §0); R-66 … R-70.

**Validation**

`node control/lab/walk-phase.mjs execution/AV-019-walk walk-effects.json walk-effects-render.mjs`; the regressions;
check-loc; check-secrets; `validate-phase-system.mjs`.

**Seam if this splits.** AV-019a — the canvas's shader — after the pass, the panel and the export.
