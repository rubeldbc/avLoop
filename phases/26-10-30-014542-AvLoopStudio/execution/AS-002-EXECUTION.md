# AS-002 — Execution Record

**Phase:** AS-002 — The composite (owner item 2)
**Status:** DONE
**Date:** 2026-10-30
**Phase start commit:** d71708b (`[AS-001]`)
**ADR:** none owed — AVADR-0001 (one renderer, GPU = CPU) holds: the composite is the CPU's for every setting, and one
plain clip keeps AVADR-0001's own two back ends (RS-10)
**Environment class:** local-disposable (this PC: the build in `build/`, the samples in `samples/`, the walk's folder)

## The owner's words

*"Make this application like mini adobe premiere pro … My final goal that I have to flexible edit and cutting like
hole a video and export as transparent."*

## What was built

1. `src/render/SequenceRenderer.{h,cpp}` — the composite: tracks bottom to top, the clip present at a time, its frames
   by the FPS method on its own frame times, its effects, its keyed motion and opacity, its masks; the loop's blend over
   whole composites; the matte; Alpha Off's background; a decoder pool (file, size, lane; 8, least recently used); a
   frame cache (8) and a mask cache (4); `plain()` (one clip AvLoop's path draws), `workingSize()`.
2. `src/render/FrameRenderer.*` — a job beyond one plain clip goes to the composite at the loop's times (terms with
   their segment frames and lanes; Motion's pre-pass handed over for V1's clip); `decodersOpen()`; the composite's
   backend is "CPU". `src/render/LayerPass.*` removed (its work is the composite's).
3. `src/render/RenderJob.*` — the job carries the sequence (made with the probed source) and `composite`;
   `Placement::of(frame, output, transform, size)` for any media.
4. `src/render/MaskBuilder.*` — `shapesCoverage()`: the running coverage of shapes (the crop's and a clip's masks).
   `src/render/CpuRenderer.*` — `sampleLinear()` shared with the composite.
5. `src/core/Sequence.*` — the **matte** (a CropShape in frame px, JSON `"matte"`); `fromAvLoop` puts the crop there
   (RS-10) and takes the frame where the video stands at In (RenderJob's).
6. `src/cli/CliSequence.cpp`, `main.cpp` — `avloop-cli sequence <project> --at T -o x.png [--raw x.rgba]`.
7. `tests/test-composite.cpp` — 9 cases (11 with the data rows): one plain clip as AvLoop's path (plain; ellipse matte
   with keyframes; inverted matte with a crossfade) within 1/255; the Razor's halves; a hole in V2 showing V1; a keyed
   opacity; the matte's hole; eight tracks on two lanes in eight decoders; a layer taking the composite.
   `tests/test-sequence.cpp` — the matte in the file's form and from AvLoop's crop; the frame at In.

## Decisions recorded

- **D-032** — the composite; spec §0. D-031's file paragraph corrected (the crop is the matte; the frame at In).
- **RS-10 … RS-13** — the crop as the matte and the plain path kept (RS-6 refined); a loop's blend over whole
  composites; the FPS method per clip, Motion the main video's; each clip at its working size, offline media not drawn.

## Specification delta

- §0: `D-032` — the composite (AS-002). README: `avloop-cli sequence … --at T`. The lore: a Qt test run by hand without
  Qt's plugin path hangs; a pixel check where both answers look alike proves nothing.

## Verification

| Command | Result |
| --- | --- |
| `scripts\build.ps1` | Built Release, 0 warnings (`/W4 /WX`) |
| `ctest --test-dir build --output-on-failure --timeout 300 -R avloop` | **27 passed**, 0 failed, 75.7 s |
| `test-composite` (by `spawnSync`, as ctest) | 11 passed, 0 skipped, 958 ms |
| `scripts\format.ps1 -Check`; `node tools/icons/check-icons.mjs` | PASS 260 files (one reformatted); PASS 61 icons |
| `node tools/check-secrets.mjs` | PASS (on its own, exit code 0) |
| `check-loc.mjs` | PASS: 339 code files, the largest `src/core/ClipEffects.cpp` 425 |
| `validate-phase-system.mjs` | PASS: 9 phases, 9 requirement ids |
| `git push origin main`; CI | pushed `3b2edd7`; CI **red** (run 40949767710): AvLoop's validator — AV-033's read scope named the removed `LayerPass.cpp`; mended in `[AS-003]` |

## The walk

```
node control/lab/walk-phase.mjs execution/AS-002-walk walk-composite-render.mjs → PASSED (9 s), 8 checks
  counter-640p30.mp4 (its frame number in 16 stripes along its foot):
  one video, an ellipse crop (280×280, feather 16), keyframes ×1.2 → ×1.4: avloop-cli frame --at 30 --cpu (AvLoop's
    path) and sequence --at 1 (the composite): 280×280, max diff 0
  a layer (V2, half size, its media from 3 s): frame 30 by the composite, the GPU setting = --cpu byte for byte, "CPU";
    the counters read: the main video 30, the layer 120
  hand-made version-4 sequences: the Razor's halves (V1 0–2 s, then from the media's 3 s): 45 at 1.5 s, 105 at 2.5 s;
    a Cut ellipse in V2 over a mirrored V1: centre 254,0,253 = V1's (V2 there 0,254,0), corner = V2's, foot 150;
    an inverted feathered ellipse matte: alpha 0 | 255; V2 keyed 0 → 100 % over 2 s: alpha 128 at 1 s
  1,200 frames of a project with three layers: 198.2 fps on the CPU, private bytes 228.9 → 237.9 MB (+1.5 after 1 s)
No avloop process, session cache, lab folder or temp file left (walk-phase.mjs).
```

## The pictures

| Picture | What | What it shows — and what was seen in it |
| --- | --- | --- |
| `AS-002-walk/hole-in-v2.png` | V1 mirrored, V2 (from 5 s) with a Cut ellipse | V2's bars and its frame 150 all round; through the ellipse V1's mirrored magenta, blue and its diagonal — the hole. The first run's picture showed no hole: both tracks were blue there (lore) |
| `AS-002-walk/matte-hole.png` | an inverted ellipse matte, feather 8 | frame 0; the ellipse clear (white in the viewer), its edge softened outward |
| `AS-002-walk/razor-2.5s.png` | the second half at 2.5 s | the media's 3.5 s: "00:00:03.500", 105 |
| `AS-002-walk/regressions/preview-layer.png` | AV-030's walk: the Preview, a layer | the main counter with the half-size layer at 50 % in the top-left quarter — the app's Preview by the composite |

## The regressions

| Walk of before | Result | Mended |
| --- | --- | --- |
| AS-001 `walk-sequence.mjs` | 33 PASSED | reads anew: the crop is the matte; the layers project's frame where the video stands at In (960×540 from −80, −70) |
| AV-007 render, AV-009 loops, AV-019 effects, AV-020 theme | PASSED (17, 5, 4, 3 s) | none (AV-007's picture: max 1/255 on 33 px, GPU noise) |
| AV-022 cut, AV-023 inverse, AV-024 shapes, AV-025 pen, AV-026 combine, AV-027 soft, AV-029 align (render) | PASSED | none |
| AV-030 `walk-layers-render.mjs`, AV-033 `walk-layer-keys-render.mjs` (now the composite) | PASSED (5, 4 s) | none |
| AV-030 `walk-layers.json`, AV-031 `walk-tracks.json`, AV-033 `walk-layer-keys.json`, AV-008 `walk-preview.json` | PASSED (9, 9, 9, 16 s) | none |

Their logs and pictures: `AS-002-walk/regressions/`; AvLoop's walk folders put back (`git checkout`).

## External freshness

- Not applicable — no dependency changed.

## Files and line counts

| File | Before | After |
| --- | --- | --- |
| `src/render/SequenceRenderer.h` / `.cpp` | — | **123** / **279** |
| `src/render/LayerPass.h` / `.cpp` | 51 / 125 | removed |
| `src/render/FrameRenderer.h` / `.cpp` | 89 / 173 | **89** / **187** |
| `src/render/RenderJob.h` / `.cpp` | 52 / 85 | **58** / **96** |
| `src/render/CpuRenderer.h` / `.cpp` | 45 / 78 | **69** / **56** |
| `src/render/MaskBuilder.h` / `.cpp` | 44 / 177 | **49** / **182** |
| `src/core/Sequence.h` / `.cpp` | 82 / 242 | **86** / **260** |
| `src/cli/CliSequence.cpp` | 62 | **101** |
| `tests/test-composite.cpp`; `tests/test-sequence.cpp` | —; 187 | **223**; **196** |

## Recovery log

- Attempt 1 RESOLVED — the hole check passed on a picture with no visible hole (both tracks blue at the centre); V1
  mirrored and the hole moved, the check now also asks that V2 is not there; the unit test likewise.
- Attempt 2 RESOLVED — `test-composite.exe` run by hand hung on Qt's platform-plugin box; stopped, run as ctest does.
- Attempt 3 RESOLVED — AS-001's walk failed two checks after the frame moved to where the video stands at In; read anew.
- Attempt 4 RESOLVED — `close-phase.mjs` failed its validation (AS-002's read scope named the removed
  `LayerPass.cpp`) after writing the record; run again, it wrote the record twice and "Active phase: none". The scope
  names `SequenceRenderer.cpp`, the state mended by hand; sync, validate and continuity PASS.

## Deviations from the phase contract

- "Draw an output frame anywhere but the SequenceRenderer": one plain clip keeps AvLoop's own path (RS-10) — its GPU
  speed, its bytes; the composite agrees with it within 1/255 (test and walk).
- A migrated file with layers and a crossfade differs in the blend's frames (RS-11): the whole composite is blended.

## Staged paths

- `src/render/SequenceRenderer.h`, `src/render/SequenceRenderer.cpp`, `src/render/LayerPass.h` (removed),
  `src/render/LayerPass.cpp` (removed), `src/render/FrameRenderer.h`, `src/render/FrameRenderer.cpp`,
  `src/render/RenderJob.h`, `src/render/RenderJob.cpp`, `src/render/CpuRenderer.h`, `src/render/CpuRenderer.cpp`,
  `src/render/MaskBuilder.h`, `src/render/MaskBuilder.cpp`, `src/core/Sequence.h`, `src/core/Sequence.cpp`,
  `src/cli/CliSequence.cpp`, `src/cli/main.cpp`, `src/ui/MainWindowLayers.cpp`, `src/CMakeLists.txt`,
  `tests/test-composite.cpp`, `tests/test-sequence.cpp`, `tests/CMakeLists.txt`, `docs/SPEC.md`, `docs/lore/avloop.md`,
  `README.md`, the programme's `READINGS.md`, `AUTONOMOUS_STATE.md`, `PROGRESS.json`, `control/00_PHASE_INDEX.md`,
  `phases/AS-002-the-composite.md`, `phases/AS-003-*.md`, `execution/AS-001-EXECUTION.md`, `execution/AS-001-walk/`,
  `execution/AS-002-EXECUTION.md`, `execution/AS-002-walk/`.

## Next

AS-003 — the editor's shell, promoted to IN_PROGRESS in the same turn. No avloop process, session cache or walk temp
file left.
