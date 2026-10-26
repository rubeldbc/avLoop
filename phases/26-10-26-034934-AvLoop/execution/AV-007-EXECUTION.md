# AV-007 — Execution Record

**Phase:** AV-007 — The renderer (owner item 7)
**Status:** DONE
**Date:** 2026-10-26
**Phase start commit:** `4f2ef40` (*[AV-006] The loop engine*)
**ADR:** AVADR-0001 (one renderer, GPU/CPU parity) **accepted**, with its as-built notes; indexed in `adr/README.md`
**Environment class:** local-disposable (this PC: `build/`, `samples/`, the walk's folder; lab windows behind the
owner's, never activated)

## The owner's words

*"One Preview button opens a separate window … exactly as it will be exported."* — *"Use the GPU."* — *"Be extremely
careful about memory."*

## What was built

1. `src/render/RenderJob` (the snapshot a loop is rendered from: source, range, transform, crop, loop, output rate;
   the output size, plan and working size; `Placement`, the one affine from output pixels to texture coordinates).
2. `render/CpuRenderer` (bilinear, texel centres at +½, transparent outside, up to four weighted layers, the mask,
   the background when opaque, rounded once; rows in parallel) and `render/GlRenderer` (OpenGL 3.3 core on an
   offscreen surface, FBO, the output pass of `Shaders.cpp`, a texture pool of the last two frames' layers under the
   byte budget, double-PBO readback, one recovery from a lost context).
3. `render/FrameRenderer` — output frame k: LoopMap → TimeMap (frame times by the packets for a variable rate,
   `media/FrameIndex`) → two decoder lanes and a small recent-frames cache → the composite → GPU or CPU; Motion's
   pre-pass by `prepare()`; "GPU rendering: Auto / Off" (`performance/gpuRendering`).
4. `avloop-cli frame <project.avloop> --at N -o x.png [--raw] [--cpu]` and `avloop-cli frames <project> [--count]
   [--mem-log]` (`cli/CliRender.cpp`; a GUI application only for the commands that render).
5. Tests: `test-renderer-cpu` (8, hand-worked answers, runs in CI), `test-renderer` (GPU, local: the unit cases ≤ 2/255
   and three projects mean ≤ 1.0, max ≤ 4; skipped without OpenGL 3.3); `test-memory` gained `decodersComeAndGo`
   (13 rows: hardware and software decoders, thumbnail workers, probes, decode workers come and go flat).
6. Mended under earlier phases (see Regressions): the normalizer makes NV12/P010 planar first; one D3D11 device for
   every decoder; filter graphs on one thread; decode and thumbnail workers on threads that live with the window
   (`media/WorkerThread`); the thumbnail decoder single-threaded; `scripts\build.ps1` deploys Qt beside every program.

## Decisions recorded

- **D-007** — the renderer as delivered (and video opens that start no thread); spec §0.
- **AVADR-0001 accepted**; **R-7** (premultiplied sRGB blending) is what both back ends do.
- **One pass over the output, not a quad per layer** — the edges agree only so (ADR's as-built notes).
- **The texture pool keeps the last two frames' layers**, the byte budget only a ceiling.
- **Opening a video starts no thread** — the GPU driver's memory grows with every thread started in this process.

## Specification delta

- §0: `D-007`.
- Lore: *The renderer* — eight rules (one pass; the pool; GL needs a GUI app and a GUI-thread surface; windeployqt per
  program; a walk's timeout and its child; NV12 and `accurate_rnd`; the driver's per-thread memory; floor vs peak).

## Verification

| Command | Result |
| --- | --- |
| `scripts\build.ps1` | built, **0 warnings** (twice from clean objects) |
| `ctest -LE gpu` | **15/15 PASS** |
| `ctest -L gpu` | **1/1 PASS** (test-renderer: unit cases, Rect, Ellipse + Feather, Crossfade) |
| `scripts\format.ps1 -Check` | PASS: 146 files |
| `node tools/icons/check-icons.mjs` | PASS: 43 icons |
| `node tools/check-secrets.mjs` | PASS: 339 files |
| `node control/phase-tools/check-loc.mjs` | PASS: 181 code files, the largest `src/ui/WalkSteps.cpp` 360 lines |
| AV-006's CI run | green, 5 m 56 s |

## The walk

```
node control/lab/walk-phase.mjs execution/AV-007-walk walk-render.mjs walk-memory-render.mjs
walk-render.mjs — output frame N of nine projects, GPU against --cpu (premultiplied bytes):
  simple 40 · crossfade 74 (in the blend) · pingpong 120 (backward) · pingpong-smooth 3 (a fractional turn)
  rect-crop 10 (a fractional crop) · ellipse-feather 10 (scale 1.3) · rounded-flip 20 · frame-blend at 0.5× 7
  motion at 60 fps 7 (the pre-pass ran) — every one on the GPU (RTX 2060 SUPER); mean ≤ 0.075, max 1
walk-memory-render.mjs — a 300 × 300 Ellipse Crossfade round its 135 frames:
  18,000 frames on the GPU, 354.6 fps: Private Bytes peaks 349/350/350 MB (+1.0), VRAM 47/47/47 (0)
  3,000 frames on the CPU, 258.7 fps: 263/263/264 MB (+1.3), VRAM 34 (0)
PHASE WALKS PASSED
```

At 1080p (800 × 800 output) the pool cap took Private Bytes from 1,107 to 632 MB and VRAM from 720 to 248 MB, flat;
70 fps there (two 1080p decoders for the blend).

## The pictures

| Picture | What | What it shows — and what was seen in it |
| --- | --- | --- |
| `AV-007-walk/render-crossfade.png` | Crossfade, the last frame (k 74 of 75) | almost wholly S[F − 1]: the burned-in counter reads 74 (2.467 s), about 1 % of S[D − 1] beneath (the Smooth curve at 15/16) — the frame before O[0] = S[F], so the seam does not show |
| `AV-007-walk/render-pingpong-smooth.png` | the smooth turn's frame 3 | an even blend of two neighbouring frames (the counter's digits doubled) |
| `AV-007-walk/render-ellipse-feather.png` | Ellipse, Feather 16, scale 1.3 | the disc with a soft rim, transparent outside |
| `AV-007-walk/render-motion-60fps.png` | Motion Interpolation at 60 fps | τ halfway between source frames 63 and 64: the counter's digits mixed, the bar and the cross moved half a step (minterpolate's smearing at the counter) |

## The regressions

| Walk of before | Result | Mended |
| --- | --- | --- |
| AV-002 walk-import.mjs | **failed, then PASSED** | "HW and SW give the same frame": mean 1.106, max 180 since AV-005 dropped `accurate_rnd` (swscale took a coarser path for NV12). NV12/P010 made planar first: mean 0, max 0 |
| AV-002 walk-memory-decode.mjs | **failed, then PASSED** | +74.5 MB over three rounds; twelve rounds rose 511 → 768 MB. The NVIDIA GL driver's arena grew with every thread a video open started (~4 MB an open). A shared D3D11 device, single-threaded filter graphs, persistent worker threads, a single-threaded thumbnail decoder: +33.8 MB, twelve rounds 475 → 543 |
| AV-002 walk-seek.mjs, walk-open.json | PASSED | none |
| AV-005 walk-timeline.json, walk-playback.mjs | PASSED | 60 fps still 0 dropped with the planar step and one-thread graphs |
| AV-001, AV-003, AV-004, AV-006 walks | PASSED | none |

AV-005's and AV-006's regression runs replayed AV-002's JSON walk only, not its scripts — so the NV12 difference went
unseen for two phases. Every walk of every earlier phase was run here. Their pictures and logs were restored from git.

## External freshness

Not applicable — no dependency changed.

## Files and line counts

New: `src/render/{RenderJob,CpuRenderer,GlRenderer,FrameRenderer}.{h,cpp}`, `src/media/FrameIndex.{h,cpp}`,
`src/media/WorkerThread.h`, `src/cli/CliRender.cpp`, `tests/{RenderCases.h,test-renderer-cpu.cpp,test-renderer.cpp}`.
Changed: `Shaders.*`, `Decoder.*`, `DecoderHw.cpp`, `SourceNormalizer.*`, `DecodeWorker.*`, `ThumbnailWorker.*`,
`MainWindow.*`, `MainWindowActions.cpp`, `Timeline.*`, `cli/main.cpp`, `scripts/build.ps1`, the CMake files,
`test-memory.cpp`. check-loc: every code file under 600, all new files under 300.

## Recovery log

Attempt 1 RESOLVED — gate: avloop-cli would not start — Qt6Concurrent.dll was not deployed (windeployqt ran on
avloop.exe only); it now runs for both programs.
Attempt 2 RESOLVED — gate: walk-memory-render ran out of its 295 s (18,000 frames at 1080p took 266 s) and left
avloop-cli running; the walk renders the 640p sample now. Private Bytes 1.1 GB there led to the pool cap.
Attempt 3 RESOLVED — gate: AV-002 walk-import (HW ≠ SW) — see Regressions.
Attempt 4 RESOLVED — gate: AV-002 walk-memory-decode — bisected in the app with temporary switches (thumbnails,
canvas frames, decode worker, frame requests, the cache): every piece was flat in a test process, only the app grew;
`cdb -pv` snapshots showed the heaps level and one VirtualAlloc region growing ~0.5 MB an open, full of the NVIDIA
driver's GLCache paths and GL extension strings; the opens that grew it were those that started threads. A thread
churn test with a GL context did not reproduce it (the driver's per-thread cost shows only with a GL-composed window).
The switches were taken out; two clean rebuilds after header changes.

## Deviations from the phase contract

- **Code budget**: 550 planned; ~1,030 new lines in `src` + ~170 changed, and the memory and NV12 mends to AV-002/005
  behaviour. The named seam (AV-007a: PBO readback and the pool budget) would move ~120. Recorded instead of split.
- **"No OpenGL 3.3 → one toast"**: the renderer says why (`gpuNote()`); the toast is shown where the app first renders
  output frames — the Preview (AV-008).
- **A software codec's own threads** (ProRes, VP9 with alpha …) still start with each such video: ~0.8 MB of driver
  memory an open remains; the memory walk passes with it.

## Blocked legs (if any)

None.

## Staged paths

- `src/`, `tests/`, `scripts/build.ps1`, `README.md`, `docs/SPEC.md`, `docs/lore/avloop.md`,
  `phases/26-10-26-034934-AvLoop/`.

## Next

AV-008 — The Preview window, promoted to IN_PROGRESS in the same turn. No avloop process, session cache or walk temp
file left; the owner's focus untouched.
