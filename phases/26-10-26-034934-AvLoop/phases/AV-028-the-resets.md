---
phase: AV-028
title: "The resets"
status: DONE
owner_item: 25
track: B
checkpoint_id: CP-B1
is_checkpoint: false
late: true
risk: Low
permission_class: VERIFY_ENVIRONMENT_FIRST
code_budget: 350
depends_on: [AV-027]
unlocks: [AV-029]
requirements: [REQ-AV-032]
read_scope:
  - phases/26-10-26-034934-AvLoop/control/28_SHAPES_THE_RESETS.md
  - docs/SPEC.md
  - src/ui/panels/Section.cpp
  - src/ui/options/OptionsBar.cpp
  - src/ui/widgets/StepSpinBox.h
  - src/ui/panels/EffectsPanel.cpp
  - src/ui/panels/LoopPanel.cpp
  - src/ui/panels/OutputPanel.cpp
  - src/ui/panels/QualityPanel.cpp
  - spec: §5.1, §6.2, §11.2
---

# The resets

Canonical current execution state: `../AUTONOMOUS_STATE.md`

**Goal.** A click on the label beside any control resets that control to its default; a right click on any group of
the right panel offers "Reset <group>", which resets the group — each one Undo step (`../README.md` §1 item 25;
D-025). A late phase: the owner's wishes of 2026-10-29, [6] and [7].

**Ground truth**

- The owner's words (`../README.md` §1, 2026-10-29).
- §5.1 (the side panel's rows), §6.2 (the Selection's Reset), §11.2 (Undo); R-6 (the output's defaults), R-28 (the
  loop's), R-66 (the effects' Reset); the fresh values of each struct in `src/core`.

**Do**

1. `ui/widgets/ResetLabel`; `Section::addRow` and `OptionsRow` making their labels so; each control's default
   (`StepSpinBox::setDefaultValue`, the combos', the colour fields', the check boxes') set where it is made; the
   reset through the control's own change path (R-92, R-93).
2. The groups' context menus ("Reset Crop" … "Reset Clip effects", "Reset shapes"; the options bars' "Reset
   <tool>"); one Undo step a group.
3. Every labelled control of `control/28` §1 covered — the five sections, the options bars, the Preview, Settings,
   the Loop Finder; a read-only row's label inert.
4. Tests (`test-commands`); `walk-resets.json`; the README's feature list.

**Do not**

Reset through anything but Undo commands; make a label's click change the focus of a field being typed into; take
a position (Crop X/Y/W/H, a shape's place) as having a default.

**Done when**

Build 0 warnings; ctest; format, icons, secrets, LOC PASS; `walk-resets.json` PASSED; the walks of the shell
(AV-001), the Selection (AV-003), the crop (AV-004), the loop (AV-006) and the effects (AV-019) PASSED again; README
current; pushed with CI green.

**Decisions this phase must record**

D-025 (spec §0); R-92 … R-93.

**Validation**

`node control/lab/walk-phase.mjs execution/AV-028-walk walk-resets.json`; the regressions; check-loc; check-secrets;
`validate-phase-system.mjs`.

**Seam if this splits.** AV-028a — the context menus — after the label clicks.
