# 10 Shapes — the renderer (item 7, AV-007)

Canonical current execution state: `../AUTONOMOUS_STATE.md`

Spec §3.4, §3.6, §3.7. D-007; AVADR-0001 (one renderer, GPU/CPU parity) accepted here. Reading R-7.

## 1. `FrameRenderer`

- Input: a `RenderJob` snapshot (crop, transform and keyframes, loop, output, the source's info) — never the live
  `Project` (export runs on a snapshot). Output: an RGBA8 premultiplied image of the output size for frame k.
- Steps: LoopMap(k) → picks; TimeMap(each pick) → source PTS and weights; frames from `FrameCache` (else the
  `Decoder`); draw each into the output canvas with the transform at that time — the crop rect maps to the output
  rect, the video rect placed by the transform, smooth (bilinear) — premultiplied; blend the weighted frames; multiply
  the mask (`MaskBuilder`, cached); alpha Off → composite over the Background colour.
- Two back ends behind one interface: `GlRenderer` and `CpuRenderer`; `FrameRenderer` picks GL unless "GPU rendering:
  Off" or GL failed, and says which in the status.

## 2. `GlRenderer` (`GlRenderer.cpp`, `Shaders.cpp`)

- Its own `QOpenGLContext` (3.3 core) on a `QOffscreenSurface`, shared with the canvas's context where possible; an
  FBO of the output size (RGBA8); textures from a pool (LRU, ≤ 512 MB or 25% of VRAM by DXGI); the frame textures
  uploaded premultiplied.
- Shaders: `transform` (a quad for the video rect in output space; samples with `GL_LINEAR`, clamp-to-border
  transparent), `blend` (mix by weight), `mask` (multiply by the mask texture with `GL_NEAREST`). Output gamma: none
  (sRGB values blended as stored, R-7).
- Readback: two PBOs alternate (`glReadPixels` into PBO n while mapping n−1); `glReadPixels` direct when PBOs fail.
- Device loss / context loss: recreate once; else CPU and a toast; a running export continues on the CPU from that
  frame.

## 3. `CpuRenderer`

The same maths: for each output pixel, the inverse transform to source texel space, bilinear with texel centres at
+0.5 and transparent outside, premultiplied; the blend `(1−w)a + wb` in float, rounded once at the end; the mask
multiply `(c × m + 127) / 255`. Rows split across threads (`QtConcurrent::blockingMap`), no allocation a frame.

## 4. The working size and the cache

The renderer tells the cache the working size it needs: the video's display size × min(1, effective scale) where the
effective scale = transform scale × (output size / crop size). The cache holds frames at that size (crop region later
at AV-009's export for big Ping-pong segments, written to the disk cache GOP by GOP for the reversed half).

## 5. Settings and status

"GPU rendering: Auto / Off" (Settings, AV-010 shows the tab; the key exists now); "No OpenGL 3.3 → CPU from the
start, one toast"; the status line `GPU: <adapter name> · HW decode on` (adapter from `GL_RENDERER` or DXGI).

## 6. Tests and walks

`test-renderer`: unit cases (identity, scale 0.5/2, flips, a fractional offset, Ellipse + Feather mask, a 0.3 blend)
GPU vs CPU ≤ 2/255; three projects mean ≤ 1.0, max ≤ 4; the GPU half skipped (and said) without a GL 3.3 context.
`avloop-cli frame <project> --at N -o x.png [--cpu]`. Walks: `walk-render.mjs` (every mode's frame N; GPU vs CPU
difference measured), `walk-memory-render.mjs` (10 minutes' worth of loop frames rendered by the CLI with `--mem-log`;
growth < 50 MB, VRAM < 32 MB).
