---
phase: AV-023
title: "The inverse selection"
status: NOT_STARTED
owner_item: 21
track: B
checkpoint_id: CP-B1
is_checkpoint: false
late: true
risk: Medium
permission_class: VERIFY_ENVIRONMENT_FIRST
code_budget: 400
depends_on: [AV-022]
unlocks: [AV-024]
requirements: [REQ-AV-028]
read_scope:
  - phases/26-10-26-034934-AvLoop/control/24_SHAPES_THE_INVERSE_SELECTION.md
  - phases/26-10-26-034934-AvLoop/control/07_SHAPES_THE_CROP.md
  - docs/SPEC.md
  - src/core/CropShape.h
  - src/core/Geometry.cpp
  - src/render/MaskBuilder.cpp
  - src/render/RenderJob.cpp
  - src/core/ProjectMigrate.cpp
  - src/ui/canvas/CanvasOverlay.cpp
  - src/ui/panels/CropPanel.cpp
  - spec: §3.3, §6.1, §10.2, §11.1
---

# The inverse selection

Canonical current execution state: `../AUTONOMOUS_STATE.md`

**Goal.** The crop's chosen area inverted: the whole video kept and the area a transparent hole in it, its feather
softening outward — on the canvas, in the Preview and in every export (`../README.md` §1 item 21; D-021). A late
phase: the owner's wish of 2026-10-29, [2].

**Ground truth**

- The owner's words (`../README.md` §1, 2026-10-29).
- D-004 and `control/07` (the crop, the mask, R-8); R-22 (the feather is the Preview's); R-49 (no crop: the frame
  where the video stands at In); R-68 and R-73 (the file's version and the Release it is tied to).

**Do**

1. `CropShape::invert`; `geometry::exportFrame`, `needsAlpha`, `coverage` with it; `RenderJob` and the Project's
   frame and alpha (R-79).
2. The file's version 3 with `ProjectMigrate` 2 → 3; `"invert"` under `crop`; 1.1.0's refusal kept (R-80).
3. `MaskBuilder`: the inverted coverage, the shape placed in the video's frame, the feather outward; the cache key.
4. The overlay's dim inside; the Crop section's Inverse box; the crop bars' Inverse toggle and its icon; Delete
   clears it, a new crop keeps it (R-81).
5. Tests; `walk-inverse.json`, `walk-inverse-render.mjs`; the README's feature list and the crop picture.

**Do not**

Change a non-inverted crop's mask by a byte; add a second crop (R-8); keyframe the crop; rename a key.

**Done when**

Build 0 warnings; ctest; format, icons, secrets, LOC PASS; `walk-inverse.json` and `walk-inverse-render.mjs`
PASSED; the walks of the crop (AV-004), the renderer (AV-007), the project file (AV-010) and the keyframes (AV-011)
PASSED again; README current; pushed with CI green.

**Decisions this phase must record**

D-021 (spec §0); R-79 … R-81.

**Validation**

`node control/lab/walk-phase.mjs execution/AV-023-walk walk-inverse.json walk-inverse-render.mjs`; the
regressions; check-loc; check-secrets; `validate-phase-system.mjs`.

**Seam if this splits.** AV-023a — the crop bars' toggle and icon — after the model, the mask and the section.
