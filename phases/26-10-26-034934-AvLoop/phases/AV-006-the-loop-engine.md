---
phase: AV-006
title: "The loop engine"
status: DONE
owner_item: 6
track: A
checkpoint_id: CP-A1
is_checkpoint: false
risk: High
permission_class: VERIFY_ENVIRONMENT_FIRST
code_budget: 500
depends_on: [AV-005]
unlocks: [AV-007, AV-015]
requirements: [REQ-AV-006]
read_scope:
  - phases/26-10-26-034934-AvLoop/control/09_SHAPES_THE_LOOP_ENGINE.md
  - docs/SPEC.md
  - spec: §8
  - after AV-005: src/ui/timeline/Timeline.h
  - after AV-005: src/core/Timecode.h
---

# The loop engine

Canonical current execution state: `../AUTONOMOUS_STATE.md`

**Goal.** The loop's maths as pure, tested functions and the Loop panel that sets them: `TimeMap` (Speed, output FPS,
the three FPS methods), `LoopMap` (Simple, Crossfade, Ping-pong, Ping-pong + Smooth turn), the `Interpolator`
pre-pass, the crossfade band on the timeline (`../README.md` §2 item 6; D-006).

**Ground truth**

- Spec §8 entire: D, τ(i), S[i], L for each mode, the crossfade formula with `c(x)` Linear / Smooth, the trapezoid
  velocity `e(x)` of the smooth turn, the FPS choices and methods, Speed 0.10×–4.00×.
- The range, the timecode grid and the timeline of AV-005.

**Do**

1. `core/LoopSettings.h`, `core/OutputSettings.h` (FPS as a rational; Source = the stream's `avg_frame_rate`, VFR →
   the nearest common value; at most 60), `core/TimeMap` (segment frame → source time → one or two source frames
   with weights by method; by PTS for VFR), `core/LoopMap` (output frame k → one or two segment frames + weight;
   F and T clamped; L per mode); SetLoop command.
2. `media/Interpolator` (a `minterpolate` pre-pass, `mi_mode=mci:mc_mode=aobmc:vsbmc=1`, into the disk cache, with
   progress and cancel).
3. `ui/panels/LoopPanel.cpp` (Mode, Crossfade s ↔ frames linked, Curve, Turn, Speed, FPS method; the summary `Loop
   4.97 s · 149 frames @ 30 fps`; clamped values flash yellow; the slow-motion hint) and the Output section's FPS row.
4. The timeline's crossfade band (gradient over the F frames before Out, dashed outline over the F after In; its left
   edge dragged changes F) and the Smooth turn's quiet fills; In/Out/F/T snapped to the output FPS grid; the editor's
   playback at Speed.
5. `tests/test-loopmap.cpp` (L per mode; `O[0] = S[F]`, the dominant part of `O[L−1]` is `S[F−1]`; no duplicate
   frame at Ping-pong turns; smooth turn positions), `test-timemap.cpp` (the three methods, VFR by PTS, Speed).

**Do not**

Render the blended frames (AV-007) or export (AV-009); audio (never).

**Done when**

Build, ctest, gates PASS; the unit tests prove §8.2's formulas for every mode and §8.3's methods; S6's other half —
dragging the crossfade band changes F (walk + screenshot); the panel's summary matches `LoopMap::length`.

**Decisions this phase must record**

D-006 (spec §0); reading R-6's FPS part (Source FPS by default, at most 60).

**Validation**

`scripts\build.ps1`; `ctest … -R "loopmap|timemap"`; `node control/lab/walk-phase.mjs execution/AV-006-walk
walk-loop.json`; format; check-loc; check-secrets.

**Seam if this splits.** AV-006a — the Interpolator and the Motion Interpolation method — after TimeMap, LoopMap,
the panel and the band.
