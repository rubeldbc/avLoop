---
phase: AV-011
title: "The keyframes"
status: NOT_STARTED
owner_item: 11
track: B
checkpoint_id: CP-B1
is_checkpoint: false
risk: Medium
permission_class: VERIFY_ENVIRONMENT_FIRST
code_budget: 450
depends_on: [AV-015]
unlocks: [AV-012, AV-016]
requirements: [REQ-AV-011]
read_scope:
  - phases/26-10-26-034934-AvLoop/control/14_SHAPES_THE_KEYFRAMES.md
  - docs/SPEC.md
  - spec: §6.2, §6.4, §7.1, §12.2
  - after AV-003: src/core/VideoTransform.h
  - after AV-007: src/render/FrameRenderer.h
---

# The keyframes

Canonical current execution state: `../AUTONOMOUS_STATE.md`

**Goal.** The video's position and scale can change over time: the Keyframe tool (K), keyframes added, moved and
deleted on the timeline's keyframe row, Linear or Ease interpolation, the Selection writing into the keyframe at the
playhead, the renderer interpolating (`../README.md` §2 item 11; D-011).

**Ground truth**

- Spec §6.4 (K, diamonds, `[`/`]`, Delete, Linear / Ease = smoothstep), §6.2 (Selection with keyframes, like After
  Effects), §7.1 row 5, §12.2 (keyframes outside In–Out still interpolate); the crop is never keyframed (§1.3).

**Do**

1. `core/Keyframes` (sorted by time; value at t: before the first = the first, after the last = the last, between =
   Linear or Ease by the earlier keyframe's interp); AddKeyframe, MoveKeyframe, DeleteKeyframe commands; the
   project file's `transform.keyframes` (written as §11.1 shows).
2. `ui/tools/KeyframeTool.cpp` (K adds/updates at the playhead) + `ui/options` rows (Add, Delete, Prev, Next,
   Interpolation); the Selection writing into the keyframe at the playhead, creating one if none.
3. The timeline's keyframe row (diamonds; selected solid orange, others hollow; Ease with rounded corners; drag to
   move in time, Delete removes, `[` / `]`; the playhead snaps to them).
4. `FrameRenderer` and the canvas take the transform at each output frame's source time.
5. `tests/test-keyframes.cpp`.

**Do not**

Keyframe the crop or anything other than position and scale (flips are not interpolated: they hold).

**Done when**

Build, ctest, gates PASS; S14's keyframes part — positions between keyframes as expected (a project with two
keyframes, `avloop-cli frame` at the middle shows the video halfway, Linear and Ease measured); the row pictured.

**Decisions this phase must record**

D-011 (spec §0); flips hold rather than interpolate.

**Validation**

`scripts\build.ps1`; `ctest … -R keyframes`; `node control/lab/walk-phase.mjs execution/AV-011-walk
walk-keyframes.json walk-keyframes.mjs`; format; check-loc; check-secrets.

**Seam if this splits.** AV-011a — the keyframe row's drag and snapping — after the model, the tool and the
renderer's interpolation.
