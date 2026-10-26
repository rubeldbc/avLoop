# AV-009 — Execution Record

**Phase:** AV-009 — The export (owner item 9)
**Status:** DONE
**Date:** 2026-10-26
**Phase start commit:** `b94a965` (*[AV-008] The Preview window: Live and Encoded, the encoder*)
**ADR:** none owed (AVADR-0001: the export uses the FrameRenderer; R-33: on the one render thread)
**Environment class:** local-disposable (this PC: `build/`, `samples/`, the walks' temp folders; lab windows behind the
owner's, never activated)

## The owner's words

*"Export writes a VP9 (or VP8) alpha WebM that OBS's Media Source plays as a seamless transparent loop."*

## What was built

1. `src/export/ExportJob` — the job from a `RenderJob` snapshot: frames made on the render service's thread (two a
   turn, at most six ahead), the encoder on the export thread, `<name>.part`, `MoveFileEx` (REPLACE_EXISTING; five
   more tries a second apart, then `name (1).webm`), the check; Cancel / error / full disk delete the `.part` (the full
   disk says what is needed and free); `bad_alloc` stops cleanly; two passes for Two-pass and Target size, one more
   when the size lands > 5 % off.
2. `src/export/OutputVerify` (frame count, rate, W×H, codec, `alpha_mode` and the alpha plane by libvpx, the first
   keyframe, no audio), `SizeEstimator` (3 × 10 frames, a frame at a time, extrapolated), `FormatProfiles` (VP9, VP8;
   the default name; `name (n).webm`).
3. The encoder: pass 1 / pass 2 with its statistics, an average bitrate for Target size, packets counted without a
   file (the estimate, pass 1), the look-ahead settable, a full disk recognised (`Muxer::outOfSpace`).
4. The model: `OutputSettings` Size (Crop / Long side / Custom W×H + lock), Alpha (Auto / On / Off), Background;
   `Project::outputSize()`, `outputAlpha()`, `exportFrame()`, `exportPath()`; `RenderJob` takes them; the `.avloop`
   keys (R-38); presets drop Custom's fields.
5. UI: `OutputPanel.cpp` (every row), `QualityPanel.cpp` (presets, typing makes Custom, Max bitrate, Two-pass,
   Target size, the estimate), `ui/export/ExportBar` (status bar text, bar, Cancel), `MainWindowExport.cpp` (Ctrl+E,
   the Save dialog, the toasts and the OBS tip, the quit question, the transparent-output question, Export and
   Preview disabled with the reason, the lazy export thread and estimator, probes and the walk steps `exportTo`,
   `answer`, `closeWindow`); `Toast` with a second button, its text above its buttons; `Section::row`.
6. `avloop-cli export <project> -o out.webm [--cpu] [--repeat N]`.
7. Tests: `test-export-job` (9: names, the estimate's arithmetic, export + check, Cancel leaves nothing, a held
   destination goes beside, Two-pass, Target size VP9 / VP8 within 5 %), `test-geometry` (the Output sizes),
   `test-project-json` (the output keys). The lab: the expect/waitFor `lacks` test.

## Decisions recorded

- **D-009** — the export as delivered; spec §0.
- **R-6** confirmed (VP9, Medium, Alpha Auto, Crop size, Source FPS).
- **R-34** the Save dialog each time; **R-35** the estimate after changes, not at a video's opening; **R-36**
  `name (1).webm` without a question; **R-37** Target size in MiB, one correction either way; **R-38** the output keys.

## Specification delta

- §0: `D-009`.
- Lore: *The export* — four rules (GL off its thread; libvpx's VBR and an unneeded target; an estimate is an export in
  small; a wrapped label beside buttons).

## Verification

| Command | Result |
| --- | --- |
| `scripts\build.ps1` | built, **0 warnings** (from clean objects after the header changes) |
| `ctest -LE gpu` | **17/17 PASS** (new: avloop-export-job) |
| `ctest -L gpu` | **1/1 PASS** |
| `scripts\format.ps1 -Check` | PASS: 179 files |
| `node tools/icons/check-icons.mjs` | PASS: 43 icons |
| `node tools/check-secrets.mjs` | PASS |
| `node control/phase-tools/check-loc.mjs` | PASS: 219 code files, the largest `src/ui/MainWindow.cpp` 364 lines |
| AV-008's CI run | green, 6 m 43 s |

## The walk

```
node control/lab/walk-phase.mjs execution/AV-009-walk walk-export.mjs walk-loops.mjs walk-fps.mjs walk-export-ui.json
walk-export.mjs (S10, 51 s): avloop-cli export of the counter, read back by ffprobe and libvpx —
  VP9 Alpha Auto (ellipse): vp9 400×280, alpha_mode 1, video only, one keyframe at 0, 90 packets = L, corner 0 /
    centre 255; Off: no alpha_mode, alpha 255; On over an opaque crop: alpha_mode 1, alpha 255; Auto over it: no
    alpha; VP8 with alpha: the same; Long side 256: 256×180
  Target size 0.1 MB VP9: 103,421 bytes (−1.4 %, 3 passes); 0.12 MB VP8: 125,251 bytes (−0.5 %, 2 passes)
  a destination held open without sharing: written as "held (1).webm" after the tries (6.8 s), the held file untouched;
    no .part anywhere
  three exports in a row in the app: Private Bytes peaks 712 / 722 / 723 MB, VRAM 117 / 109 / 109 MB
walk-loops.mjs (S7, S11, 7 s): a source repeating every 2 s, In 1 – Out 3 —
  simple 60 = L, seam 1.05× the median; crossfade 45, 1.09×; pingpong 118, 0.84×; pingpong-smooth 118, 0.05×;
  crossfade with alpha 45, 1.30×; no frame repeated; each read twice in a row: 2L frames, no error
walk-fps.mjs (S8, 41 s): 15 / 24 / 30 / 60 / 12 fps × Nearest / Blend / Motion: D frames, r_frame_rate right, CFR
  (steps 1/fps ± 0.67 ms — WebM's milliseconds); Speed 0.5× 120 frames, 2× 30
walk-export-ui.json (S10's UI, 19 s): Export disabled without a video ("Open a video to export it"); the default name
  counter-640p30_loop_640x360.webm; Size: Long side 256 → 256x144, Custom 500 locked → 500x282, unlocked 500x300,
  4000 → 1920x144 with "Limited to 1920 px …" — picture; Alpha Auto (on) / Off / Background #ff00ff; VP8; Quality
  High 24 → typed 33 = Custom → Medium 31; the estimate "~0.29 MB" — picture; Export: done, checked, the file there,
  the toast with the OBS tip — picture; again: no tip; Cancel at frame 3+: "Exporting 31/75 · 41% · 57 fps · ETA
  0:01" — picture —, cancelled, nothing left, "Export cancelled"; closing during an export: No keeps it, Yes cancels
  it, nothing left; a crop outside the video: the question, No → no export
PHASE WALKS PASSED
```

## The pictures

| Picture | What | What it shows — and what was seen in it |
| --- | --- | --- |
| `AV-009-walk/export-toast.png` (and `docs/screenshots/export.png`) | an export done | the editor with the ellipse; the side panel's Output (WebM VP9, Crop size (640×360), Auto (on), the background greyed as alpha is on) and Quality (Medium, CRF 31, Speed 2, Estimate ~0.29 MB); the toast "Exported 640×360 · 2.50 s · 0.28 MB" and the OBS line on two lines, Show in folder and Don't show again beneath |
| `AV-009-walk/output-panel.png` | Output, Custom | Custom W×H 4000 × 300 unlocked, the note "Limited to 1920 px on the long side: 1920×144" |
| `AV-009-walk/quality-panel.png` | Quality | Medium, CRF 31, Speed 2, Estimate ~0.29 MB |
| `AV-009-walk/export-bar.png` | the status bar | "Exporting 31/75 · 41% · 57 fps · ETA 0:01", the thin bar, Cancel |

## The regressions

| Walk of before | Result | Mended |
| --- | --- | --- |
| AV-002 walk-memory-decode.mjs | **failed twice, then PASSED twice** (growth 34–39 MB) | the size estimate measured at every opening (peaks ≈ 1 GB); now after changes only, a frame at a time (R-35), its thread and the render service made at first use |
| AV-001 … AV-008, every other walk | PASSED | none |

Their pictures and logs were restored from git.

## External freshness

Not applicable — no dependency changed.

## Files and line counts

New: `src/export/{ExportJob,OutputVerify,SizeEstimator,FormatProfiles}.{h,cpp}` (358 / 106 / 186 / 44 + headers),
`src/ui/MainWindowExport.cpp` (292), `src/ui/export/ExportBar.{h,cpp}`, `src/ui/panels/QualityPanel.cpp` (124),
`src/cli/CliExport.cpp` (110), `tests/test-export-job.cpp` (190). Changed: `LoopSettings.*`, `Project.*`,
`ProjectJson.*`, `QualitySettings.cpp`, `Encoder.*`, `EncoderOptions.cpp`, `Muxer.*`, `RenderJob.*`, `OutputPanel.cpp`
(rewritten, 251), `CropPanel.cpp`, `LoopPanel.cpp`, `Section.*`, `SidePanel.*`, `Toast.*`, `MainWindow.*`,
`MainWindowPreview.cpp`, `WalkSteps.cpp`, `cli/main.cpp`, the CMake files, `.gitignore` (a walk's exported loops).

## Recovery log

Attempt 1 RESOLVED — gate: avloop-cli export crashed after encoding — `backendName()` called `glGetString` on the
export thread; the GPU's name is read on the render thread.
Attempt 2 RESOLVED — gate: Target size ±5 % (−8 % VP9, −19 % VP8 at 3× the CRF size) — libvpx cannot spend bits an
easy loop does not need; one corrected pass either way, and a reachable target in the tests (within 2 %).
Attempt 3 RESOLVED — gate: test-export-job (frame count) — the default Crossfade makes L = D − F; the test reads L.
Attempt 4 RESOLVED — gate: walk-export.mjs — its project files lacked the transform (the video at the scene's origin);
the alpha check expects each case's corners.
Attempt 5 RESOLVED — gate: walk-loops.mjs (Crossfade with alpha: repeats) — the bar left a narrow crop; the crop is
as wide as the frame.
Attempt 6 RESOLVED — gate: walk-fps.mjs (Motion) — avloop-cli export had no disk cache; it has one now, made before the
render service so it outlives the service's last task.
Attempt 7 RESOLVED — gate: AV-002 walk-memory-decode (+363 MB) — see Regressions.
Attempt 8 RESOLVED — gate: the pictures (the toast clipped its text and buttons; Cancel cut by the status bar) — the
toast's text above its buttons; a flat Cancel.

## Deviations from the phase contract

- **Code budget**: 600 planned; ~2,000 new lines in `src` and `tests` + ~600 changed. The named seam (AV-009a: Target
  size, the estimator and the OBS tip) would move ~350. Recorded instead of split.
- **Disk full** is handled (the `.part` deleted, the space needed said) but not exercised: no full volume can be made
  without administrator rights. `Muxer::outOfSpace` is set by FFmpeg's `ENOSPC`.
- **OBS itself** (S11's second half) is not driven: S11's first half (2L frames through libvpx, no error) passes.

## Blocked legs (if any)

None.

## Staged paths

- `src/`, `tests/`, `.gitignore`, `README.md`, `docs/SPEC.md`, `docs/lore/avloop.md`, `docs/screenshots/export.png`,
  `phases/26-10-26-034934-AvLoop/`.

## Next

AV-010 — The project file, promoted to IN_PROGRESS in the same turn. No avloop process, session cache or walk temp file
left; the owner's focus untouched.
