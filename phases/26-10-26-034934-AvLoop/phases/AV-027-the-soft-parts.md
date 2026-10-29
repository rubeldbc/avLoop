---
phase: AV-027
title: "The soft parts"
status: DONE
owner_item: 24
track: B
checkpoint_id: CP-B1
is_checkpoint: false
late: true
risk: Medium
permission_class: VERIFY_ENVIRONMENT_FIRST
code_budget: 450
depends_on: [AV-026]
unlocks: [AV-028]
requirements: [REQ-AV-031]
read_scope:
  - phases/26-10-26-034934-AvLoop/control/27_SHAPES_THE_SOFT_PARTS.md
  - phases/26-10-26-034934-AvLoop/control/25_SHAPES_THE_SHAPES_AND_THE_PEN.md
  - docs/SPEC.md
  - after AV-024: src/core/MaskShape.h
  - after AV-024: src/ui/options/ShapeOptions.cpp
  - src/render/MaskBuilder.cpp
  - tests/test-mask.cpp
  - spec: §6.1, §11.1
---

# The soft parts

Canonical current execution state: `../AUTONOMOUS_STATE.md`

**Goal.** Every shape with its own Feather and Strength: a region within a selection made partly or wholly
transparent by a Cut shape at the strength wanted, its edge softened as far as wanted — exact in the Preview and the
export (`../README.md` §1 item 24; D-024). A late phase: the owner's wish of 2026-10-29, [5].

**Ground truth**

- The owner's words (`../README.md` §1, 2026-10-29).
- §6.1's feather (inward over f output px, a 1 px edge at 0); AV-004's `distance` and `coverage`; AV-026's soft
  ops; R-22 (the canvas hints, the Preview is exact).

**Do**

1. `MaskShape::feather`, `strength`; the file's keys, version 3; the multi-selection's typed value sets all (R-90).
2. `MaskBuilder`: a shape's signed distance — analytic for the standard kinds, an exact Euclidean distance
   transform of the raster for Polygon and Path (`render/DistanceField.cpp`, pure, tested against brute force) —
   through §6.1's formula × strength; the soft ops of AV-026 on it; the cache key (R-91).
3. The options bars' Feather and Strength; the Crop section's rows and the list's "· 20 px · 60 %"; the overlay's
   dim scaled by a cut's strength.
4. Tests; `walk-soft.json`, `walk-soft-render.mjs`; the README's feature list and a picture.

**Do not**

Change a shape at Feather 0 and Strength 100 by a byte (AV-024 … AV-026's walks again); keep an image a shape
(the running mask, one raster and the EDT's rows only); draw feather on the canvas.

**Done when**

Build 0 warnings; ctest; format, icons, secrets, LOC PASS; `walk-soft.json` and `walk-soft-render.mjs` PASSED; the
walks of the crop (AV-004), the shapes (AV-024), the pen (AV-025) and the combinations (AV-026) PASSED again; README
current; pushed with CI green.

**Decisions this phase must record**

D-024 (spec §0); R-90 … R-91.

**Validation**

`node control/lab/walk-phase.mjs execution/AV-027-walk walk-soft.json walk-soft-render.mjs`; the regressions;
check-loc; check-secrets; `validate-phase-system.mjs`.

**Seam if this splits.** AV-027a — the distance transform for paths — after Feather and Strength on the standard
kinds.
