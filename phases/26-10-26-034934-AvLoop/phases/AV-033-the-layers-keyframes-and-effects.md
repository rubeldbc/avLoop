---
phase: AV-033
title: "The layers' keyframes and effects"
status: DONE
owner_item: 29
track: B
checkpoint_id: CP-B1
is_checkpoint: false
late: true
risk: High
permission_class: VERIFY_ENVIRONMENT_FIRST
code_budget: 700
depends_on: [AV-031]
unlocks: [AV-032]
requirements: [REQ-AV-036]
read_scope:
  - phases/26-10-26-034934-AvLoop/control/30_SHAPES_THE_TIMELINE_LAYERS.md
  - docs/SPEC.md
  - src/core/VideoLayer.h
  - src/render/LayerPass.cpp
  - src/ui/panels/LayersPanel.cpp
  - src/ui/panels/EffectsPanel.cpp
  - src/ui/timeline/TrackLanes.cpp
  - spec: §6.4, §11.1, §11.2
---

# The layers' keyframes and effects

Canonical current execution state: `../AUTONOMOUS_STATE.md`

**Goal.** A video layer as a Premiere Pro clip: its position, scale and opacity keyframed over its own time (Linear or
Ease), its own clip effects through the Clip effects section, its keyframes on its bar in the lanes — the same in the
canvas, the Preview and every export (`../README.md` §1 item 29; D-029). A late phase: R-98's veto by the owner.

**Ground truth**

- The owner's words (`../README.md` §1, 2026-10-30): "video layer will have all the facility like adobe premiere pro
  with keyframes functionality also. do it first. then do the test".
- D-011 (the main video's keyframes: one a frame, Linear/Ease, a field edit writing into the key at the playhead),
  D-017 (the clip effects, `EffectsPass`), D-027 (the layers, the LayerPass).

**Do**

1. `VideoLayer`: keys {t, x, y, scale, opacity, ease} over its own time and `ClipEffects`; its look at a time; the
   file's `"keys"`, `"effects"` (R-106).
2. `LayerPass` and the canvas's look: the layer at its keyed place, scale and opacity, its effects by `EffectsPass`.
3. The Layers section: Key, Delete key, Ease; a field edit (and the Selection's drag) on a keyed layer writes into the
   key at the playhead; the Clip effects section edits the selected layer's effects (R-107); the lanes' diamonds, a
   double-click on one takes the playhead there.
4. Tests; `walk-layer-keys.json`, `walk-layer-keys-render.mjs`; README.

**Do not**

Change the main video's keyframes or effects when a layer is selected (but the Clip effects section's target); let a
layer without keys or effects render differently by a byte from AV-031's.

**Done when**

Build 0 warnings; ctest; gates PASS; the phase's walks PASSED; AV-030's and AV-031's walks PASSED again; pushed with
CI green (the full sweep is AV-032's, after this phase).

**Decisions this phase must record**

D-029 (spec §0); R-106, R-107.

**Validation**

`node control/lab/walk-phase.mjs execution/AV-033-walk walk-layer-keys.json walk-layer-keys-render.mjs`; check-loc;
check-secrets; `validate-phase-system.mjs`.

**Seam if this splits.** AV-033a — the layers' effects — after the keyframes.
