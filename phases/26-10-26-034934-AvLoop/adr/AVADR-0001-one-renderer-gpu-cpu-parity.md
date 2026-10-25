# AVADR-0001 — One renderer, GPU/CPU parity

Status: **Drafted 2026-10-26** by the plan; accepted by AV-007 before the renderer's code.

## Context

The owner: *"One Preview button opens a separate window with a slider that shows only the exported part, exactly as
it will be exported"* and *"Use the GPU."* Spec §3: "Preview and export use the same `FrameRenderer`, so what the
preview shows is exactly what gets exported. This is the central architecture decision." Spec §3.6: every GPU path has
a guaranteed CPU fallback; results differ by ≤ 2/255. CI and Remote Desktop have no OpenGL 3.3.

## Decision

1. **One `FrameRenderer`** makes output frame k from a `RenderJob` snapshot. The Preview's Live mode, the Encoded
   mode's encoder, the export job, `avloop-cli frame` and `avloop-cli export` all call it. Nothing else composes
   output frames.
2. **Two back ends behind it**: `GlRenderer` (OpenGL 3.3 core on an offscreen surface; FBO; textures; the transform,
   blend and mask shaders; double-PBO readback) and `CpuRenderer` (the same maths on the CPU). The canvas draws the
   source with the same transform shader.
3. **Parity by construction**: (a) the mask is built once on the CPU by `MaskBuilder` at the output size and the GPU
   samples it as a texture at pixel centres (`GL_NEAREST`) — no shape maths differ; (b) source frames reach both back
   ends pre-scaled to the working size by swscale, so bilinear sampling works near 1:1 where GL's and the CPU's
   filters agree; (c) the CPU sampler uses GL's texel-centre convention and clamp-to-transparent border; (d) blending
   is premultiplied sRGB in float with one rounding (R-7).
4. **Parity by test**: `test-renderer` renders unit cases on both (≤ 2/255 per channel) and three projects (mean ≤ 1.0,
   max ≤ 4); the GPU half is skipped, and says so, without a context.
5. **Fallback**: "GPU rendering: Auto / Off"; no GL 3.3 → CPU from the start with one toast; device loss → recreate
   once, else CPU, the running export continuing from that frame.

## Consequences

- The export's speed is the GPU's when present; correctness never depends on it.
- Any new visual feature (keyframes at AV-011) is added to both back ends in the same phase, with a test case.
- The Encoded preview is the only view that shows compression; Live is the renderer's exact frame.
