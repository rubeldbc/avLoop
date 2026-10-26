---
phase: AV-020
title: "The colour theme"
status: DONE
owner_item: 18
track: B
checkpoint_id: CP-B1
is_checkpoint: false
late: true
risk: Low
permission_class: VERIFY_ENVIRONMENT_FIRST
code_budget: 500
depends_on: [AV-019]
unlocks: []
requirements: [REQ-AV-025]
read_scope:
  - phases/26-10-26-034934-AvLoop/control/21_SHAPES_THE_COLOUR_THEME.md
  - docs/SPEC.md
  - src/core/ClipEffects.cpp
  - src/ui/panels/EffectsPanel.cpp
  - src/ui/canvas/Canvas.cpp
  - spec: §5.1, §11.1
---

# The colour theme

Canonical current execution state: `../AUTONOMOUS_STATE.md`

**Goal.** Effects that change a clip's colour theme — an orange clip made red — in the Clip effects section: Replace
colour with an eyedropper, Colourise, Split toning, Duotone (`../README.md` §1 item 18; D-018). A late phase.

**Ground truth**

- The owner's words (`../README.md` §1, 2026-10-27).
- D-017 and `control/20`: the clip effects these join (one colour function, the canvas's table, the CPU pass).

**Do**

1. `ClipEffects`: the four effects' colours, ranges and amounts; their maths in `effects::Colour` (R-71).
2. `ColourField` (hex, swatch, eyedropper); the canvas's pick mode (R-72); the eyedropper icon.
3. The section's rows; tests; the walks; the README and its pictures.

**Do not**

Add a second colour path; let an effect at 0 % change a byte; bump the file's version (R-73).

**Done when**

Build 0 warnings; ctest; format, icons, secrets, LOC PASS; `walk-theme.json` and `walk-theme-render.mjs` PASSED, the
walks of the shell, the canvas, the keyframes and AV-019's PASSED again; README current; pushed with CI green.

**Decisions this phase must record**

D-018 (spec §0); R-71 … R-73.

**Validation**

`node control/lab/walk-phase.mjs execution/AV-020-walk walk-theme.json walk-theme-render.mjs`; the regressions;
check-loc; check-secrets; `validate-phase-system.mjs`.

**Seam if this splits.** AV-020a — the eyedropper — after the four effects.
