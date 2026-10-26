---
phase: AV-004
title: "The crop tools"
status: DONE
owner_item: 4
track: A
checkpoint_id: CP-A1
is_checkpoint: false
risk: Medium
permission_class: VERIFY_ENVIRONMENT_FIRST
code_budget: 550
depends_on: [AV-003]
unlocks: [AV-005, AV-015]
requirements: [REQ-AV-004, REQ-AV-016]
read_scope:
  - phases/26-10-26-034934-AvLoop/control/07_SHAPES_THE_CROP.md
  - docs/SPEC.md
  - spec: §3.3, §6.1
  - after AV-003: src/ui/canvas/Canvas.h
  - after AV-003: src/ui/tools/Tool.h
  - after AV-003: src/core/Project.h
---

# The crop tools

Canonical current execution state: `../AUTONOMOUS_STATE.md`

**Goal.** The crop frame decides what is exported: Rect (M), Rounded (U) and Ellipse (E) drawn, moved and resized on
the canvas with ratios, modifiers, snapping, Feather and Radius; the dim outside; the mask built once and cached; the
Crop section of the side panel (`../README.md` §2 item 4; D-004).

**Ground truth**

- Spec §6.1 entire; §3.3 (`CropShape`, the output size rule: crop w×h, long side ≤ 1920, even, ≥ 16).
- The canvas, tools and Undo of AV-003.

**Do**

1. `core/CropShape` (kind, rect, radius, feather, ratio), `core/AspectRatio` (Free, Original, 1:1 … 2:3, 4:5,
   Custom; a change keeps the centre and the area), the output size rule in `core/Geometry`; SetCrop command.
2. `ui/tools/CropTool.cpp` + `CropHandles.cpp`: draw (marquee), move, 8 handles, replace by dragging outside (one Undo
   step), Shift (ratio lock; 1:1 Free, a circle for Ellipse), Alt (from the centre), arrows 1/10 px, Esc, Delete,
   double-click = the whole video, snapping to the video's edges and centre within 8 screen px (Ctrl off), switching
   kind keeps the rectangle; drawing below 16 px cancelled.
3. The overlay: 60% black outside, 1.5 px orange border, 8×8 white handles with orange borders, the dashed bounding
   box for Ellipse and Rounded, the rule-of-thirds guides and the size label while dragging.
4. `render/MaskBuilder` (Rect / Rounded / Ellipse, Feather 0–200 output px inward, a 1 px anti-aliased edge at 0;
   built at the output size, cached by its inputs) — the CPU mask both renderers use (AVADR-0001 at AV-007).
5. `ui/options/CropOptions.cpp` (Ratio, X, Y, W, H, Swap, Clear, Radius, Feather), `ui/panels/SidePanel` +
   `CropPanel.cpp`; the "Transparent area in output" status; Alpha Auto's "needs alpha" rule in `core`.
6. `tests/test-aspect.cpp`, `test-mask.cpp` (Ellipse corners 0, centre 255, Feather steps down gradually).

**Do not**

Keyframe the crop (never, §1.3); render or export (AV-007, AV-009); more than one crop (never).

**Done when**

Build, ctest, gates PASS; S5's canvas half — a walk draws each shape, sets every ratio, uses Shift/Alt, snaps,
nudges with arrows; the mask PNGs from `test-mask` show alpha 0 at the Ellipse corners, 255 at the centre and a
gradual Feather edge; the screenshots of each shape.

**Decisions this phase must record**

D-004 (spec §0); reading R-8 (one crop; none = the whole video).

**Validation**

`scripts\build.ps1`; `ctest … -R "aspect|mask|commands"`; `node control/lab/walk-phase.mjs execution/AV-004-walk
walk-crop.json`; format; check-loc; check-secrets.

**Seam if this splits.** AV-004a — snapping, the guides and the Crop panel — after the three shapes, handles,
ratios and the mask.
