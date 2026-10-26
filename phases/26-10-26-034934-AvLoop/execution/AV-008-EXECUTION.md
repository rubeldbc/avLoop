# AV-008 — Execution Record

**Phase:** AV-008 — The Preview window (owner item 8)
**Status:** DONE
**Date:** 2026-10-26
**Phase start commit:** `39ab391` (*[AV-007] The renderer*)
**ADR:** none owed (AVADR-0001: the Preview uses the FrameRenderer; AVADR-0002: its cache is bounded — R-31)
**Environment class:** local-disposable (this PC: `build/`, `samples/`, the walk's folder; lab windows — the Preview
too — behind the owner's, never activated)

## The owner's words

*"One Preview button opens a separate window with a slider that shows only the exported part, exactly as it will be
exported."*

## What was built

1. `src/core/QualitySettings` (Low / Medium / High / Custom; CRF, Speed, max bitrate, two-pass, target — the last two
   AV-009's) and `OutputFormat` (WebM VP9 / VP8); `Project::changeQuality` (SetQuality); `.avloop` `quality` and
   `output.format`; `RenderJob` carries the quality.
2. `src/media/Encoder` + `EncoderOptions` + `Muxer` — premultiplied frames made straight, `yuva420p`/`yuv420p`,
   libvpx-vp9 / libvpx with §10.3's parameters, BT.709 (VP9) / BT.601 (VP8) tagged, pts = k, keyframe first, one GOP,
   WebM with `alpha_mode`, a cancel deletes the file.
3. `src/render/RenderService` — one render thread and one GPU context for the window's life; `FrameRenderer` can use
   its context; `GlRenderer` makes its context and program on the rendering thread.
4. `src/ui/preview/PreviewWindow` (the window, its rows, keys, geometry, the temporary file), `PreviewWidgets`
   (`PreviewView`: backgrounds and zoom; `PreviewSlider`: seam marks, turns, the cached bar), `PreviewPlayer` (Live
   and Encoded tracks, the bounded cache, the waiting clock, Around seam), `PreviewEncode.cpp` (encode in the
   background, then the file through libvpx); `ui/MainWindowPreview.cpp` (Preview button, F5, 250 ms updates, probes,
   the walk steps `previewSeek`, `previewWrite`, `previewClose`, `projectSave`).
5. The lab: the walk's key step activates (in Qt only) its target's window; `avloop.lab` on the application keeps the
   Preview quiet as well.
6. The editor's playback keeps the lap's first six frames (each wrap starts at once, the decoder seeking behind them).
7. Tests: `test-export` (6: VP9 and VP8 with alpha read back through libvpx — size, rate, codec, alpha, colour, one
   GOP and a first keyframe —, no alpha without it, a cancel leaves no file).

## Decisions recorded

- **D-008** — the Preview window as delivered; spec §0.
- **R-14** confirmed: the encoder lives in `media/` for the Encoded preview and the export.
- **R-31** the Preview's cache (a third of the frame cache, ≤ 512 MB); **R-32** VP9 BT.709 / VP8 BT.601, straight
  colour; **R-33** one render thread and GPU context for the app's life.
- **The Preview never drops a frame**: its clock waits for a frame not yet made ("Caching…"), as §9.3 asks, unlike the
  editor's clock (§7.3).

## Specification delta

- §0: `D-008`.
- Lore: *The Preview and the encoder* — five rules (a context's thread; VP8 and BT.709; WebM's alpha marking; keys in a
  second window; a wrap between keyframes).

## Verification

| Command | Result |
| --- | --- |
| `scripts\build.ps1` | built, **0 warnings** (from clean objects after the header changes) |
| `ctest -LE gpu` | **16/16 PASS** |
| `ctest -L gpu` | **1/1 PASS** |
| `scripts\format.ps1 -Check` | PASS: 165 files |
| `node tools/icons/check-icons.mjs` | PASS: 43 icons |
| `node tools/check-secrets.mjs` | PASS: 366 files |
| `node control/phase-tools/check-loc.mjs` | PASS: 201 code files, the largest `src/ui/WalkSteps.cpp` 362 lines |
| AV-007's CI run | green, 6 m 26 s |

## The walk

```
node control/lab/walk-phase.mjs execution/AV-008-walk walk-preview.json walk-preview-check.mjs
walk-preview.json (87 steps; counter-640p30, In 2 s – Out 5 s, Crossfade, an Ellipse with Feather 12):
  F5 opens the Preview (F5 again: the same window); 75 frames made and cached; "640×360 · 30 fps · 75 frames · 2.50 s ·
  VP9 alpha"; the frame 640×360 — picture
  Green, Navy, White, Black, Checker; 100% and Fit — pictures
  → 1; ← ← → 74 (it loops); Play 1.2 s → 35 (round the seam); Around seam on and off
  Encoded: "done" in a few seconds, 0.28 MB, "Encoded · 0.28 MB", the frames from the file — picture
  frame 10 written in Live and in Encoded, the encoded file kept for the check
  the Feather changed in the main window → Encoded "Out of date"; Live made again (75 cached)
  closed → no Preview; F5 → a fresh one (no encode); closed
walk-preview-check.mjs:
  Live frame 10 = avloop-cli frame 10 of the same project: mean 0.0000, max 0
  Encoded frame 10 against Live: mean 1.07 (compression only)
  the Encoded file: alphaOf corner 0, centre 255; vp9 640×360 30/1
  the Preview opened and closed six times: Private Bytes peaks 612/616/617/618/619/619 MB (+3.2), VRAM +0
PHASE WALKS PASSED
```

## The pictures

| Picture | What | What it shows — and what was seen in it |
| --- | --- | --- |
| `AV-008-walk/preview-checker.png` | Live over Checker | only the export: the feathered ellipse, the checker through its soft rim and outside; the slider with the seam ticks and the orange Crossfade strip at its end; the cached bar full; the info row |
| `AV-008-walk/preview-green.png`, `preview-navy.png` | the backgrounds | the loop over #00FF00 and over Navy — the Feather's soft edge visible against both |
| `AV-008-walk/preview-100.png` | 100% | the frame at its own size, pixel-exact |
| `AV-008-walk/preview-encoded.png` (and `docs/screenshots/preview.png`) | Encoded | "Encoded · 0.28 MB", the frame from the WebM with its alpha, "· 0.28 MB" in the info row; Encoded lit orange |
| `AV-008-walk/live-10.rgba.png`, `encoded-10.rgba.png` | frame 10 | the same frame from Live and from the encoded file |

## The regressions

| Walk of before | Result | Mended |
| --- | --- | --- |
| AV-005 walk-timeline.json | **failed once, then PASSED twice** | one frame dropped where playback wrapped to an In between keyframes; the lap's first frames are kept now |
| AV-001 … AV-007, every walk | PASSED | none (the key step's change: keys go by their window's shortcuts) |

Their pictures and logs were restored from git.

## External freshness

Not applicable — no dependency changed (libvpx 1.16 is part of the fetched LGPL FFmpeg).

## Files and line counts

New: `src/core/QualitySettings.{h,cpp}`, `src/media/{Encoder,EncoderOptions,Muxer}.{h,cpp}`,
`src/render/RenderService.{h,cpp}`, `src/ui/preview/{PreviewWindow,PreviewWidgets,PreviewPlayer}.{h,cpp}`,
`src/ui/preview/PreviewEncode.cpp`, `src/ui/MainWindowPreview.cpp`, `tests/test-export.cpp`. Changed: `LoopSettings.h`,
`Project.*`, `ProjectJson.*`, `Commands.h`, `RenderJob.*`, `FrameRenderer.*`, `GlRenderer.cpp`, `DecodeWorker.*`,
`MainWindow.*`, `WalkRunner.h`, `WalkSteps.cpp`, `app/main.cpp`, `app.qss`, the CMake files.

## Recovery log

Attempt 1 RESOLVED — gate: test-export (VP8): colour 14–16 off — VP8 is read as BT.601; VP8 is written in BT.601.
Attempt 2 RESOLVED — gate: the build — `Track` defined in one source file, used in two; moved to the header.
Attempt 3 RESOLVED — gate: walk-preview crashed (0xC0000409) on F5 — the GL context belonged to the GUI thread; made on
the render thread now.
Attempt 4 RESOLVED — gate: walk-preview (the window's ← and →) — the key step activated the main window; it activates
its target's window now.
Attempt 5 RESOLVED — gate: AV-005 walk-timeline (one frame dropped at a wrap) — see Regressions.

## Deviations from the phase contract

- **Code budget**: 550 planned; ~1,720 new lines in `src` + ~150 changed. The named seam (AV-008a: Encoded and the
  encoder) would move ~600. Recorded instead of split.
- **The size estimate** (`~1.8 MB` before encoding) is AV-009's `SizeEstimator`; the info row shows the actual size
  once Encoded has run.

## Blocked legs (if any)

None.

## Staged paths

- `src/`, `tests/`, `resources/themes/app.qss`, `README.md`, `docs/SPEC.md`, `docs/lore/avloop.md`,
  `docs/screenshots/preview.png`, `phases/26-10-26-034934-AvLoop/`.

## Next

AV-009 — The export, promoted to IN_PROGRESS in the same turn. No avloop process, session cache or walk temp file left;
the owner's focus untouched.
