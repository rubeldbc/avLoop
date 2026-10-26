---
phase: AV-007
title: "The renderer"
status: DONE
owner_item: 7
track: A
checkpoint_id: CP-A1
is_checkpoint: false
risk: High
permission_class: VERIFY_ENVIRONMENT_FIRST
code_budget: 550
depends_on: [AV-006]
unlocks: [AV-008, AV-015]
requirements: [REQ-AV-007, REQ-AV-016, REQ-AV-017]
read_scope:
  - phases/26-10-26-034934-AvLoop/control/10_SHAPES_THE_RENDERER.md
  - phases/26-10-26-034934-AvLoop/adr/AVADR-0001-one-renderer-gpu-cpu-parity.md
  - docs/SPEC.md
  - spec: §3.4, §3.6, §3.7
  - after AV-004: src/render/MaskBuilder.h
  - after AV-006: src/core/LoopMap.h
  - after AV-006: src/core/TimeMap.h
---

# The renderer

Canonical current execution state: `../AUTONOMOUS_STATE.md`

**Goal.** One `FrameRenderer` makes output frame k — the frame the preview shows and the export writes — on the GPU
(OpenGL 3.3 core, offscreen) with a CPU renderer giving the same result within 2/255; the readback by double PBO;
the memory budgets enforced (`../README.md` §2 item 7; D-007; AVADR-0001 accepted).

**Ground truth**

- Spec §3.4 (the seven steps), §3.6 (GPU table, settings, the status line, the GPU walk), §3.7 (budgets, lifetimes).
- `LoopMap`, `TimeMap` (AV-006), `MaskBuilder` (AV-004), the cache and decoder (AV-002).

**Do**

1. `render/FrameRenderer` (output frame k: LoopMap → TimeMap → frames from the cache or decoder → draw each with the
   transform into the crop at output size, premultiplied, smooth → blend by weight → multiply the mask → RGBA).
2. `render/GlRenderer.cpp` + `Shaders.cpp` (`QOpenGLContext` + `QOffscreenSurface`, FBO, the transform shader, the
   blend, the mask; the texture pool within 512 MB or 25% of VRAM; device loss → recreate, else CPU), the canvas
   drawing with the same shaders; `render/CpuRenderer.cpp` (the same bilinear sampling and blend on the CPU);
   `render/Blend`.
3. The readback: double-buffered PBO, async; `glReadPixels` as the fallback.
4. Settings "GPU rendering: Auto / Off" (the toast on fallback), "No OpenGL 3.3 → CPU from the start"; the status
   line `GPU: <adapter> · HW decode on`.
5. `avloop-cli frame <project> --at N -o x.png [--cpu]`; `tests/test-renderer.cpp` (GPU vs CPU: mean ≤ 1.0, max ≤ 4;
   2/255 on the unit cases; skipped without GL).

**Do not**

The Preview window (AV-008) or the encoder (AV-008/AV-009).

**Done when**

Build, ctest, gates PASS; `avloop-cli frame` renders every loop mode's frame N of a sample project; the GPU parity
gate on three projects (Rect, Ellipse + Feather, Crossfade); the memory walk (frames rendered for 10 minutes'
worth of loops) flat — Private Bytes < 50 MB growth, VRAM < 32 MB.

**Decisions this phase must record**

D-007 (spec §0); AVADR-0001 accepted; reading R-7 (premultiplied sRGB blending).

**Validation**

`scripts\build.ps1`; `ctest … -R renderer`; `node control/lab/walk-phase.mjs execution/AV-007-walk walk-render.mjs
walk-memory-render.mjs`; format; check-loc; check-secrets.

**Seam if this splits.** AV-007a — the PBO readback and the texture pool's budget — after the GL renderer, the CPU
renderer and their parity.
