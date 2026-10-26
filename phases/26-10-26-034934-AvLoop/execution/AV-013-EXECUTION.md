# AV-013 — Execution Record

**Phase:** AV-013 — The batch and the other formats (owner item 13)
**Status:** DONE
**Date:** 2026-10-26
**Phase start commit:** `0a07348` (*[AV-012] The Loop Finder: the best In/Out pairs, found*)
**ADR:** none owed
**Environment class:** local-disposable (this PC: `build/`, `samples/`, the walks' temp folders and the lab runs' data
folders; lab windows behind the owner's, never activated)

## The owner's words

*"Batch export; GIF, APNG, MP4 and ProRes 4444 besides WebM."*

## What was built

1. The formats: `media/GifPalette` (palettegen over every frame, paletteuse Bayer with alpha cut at 128),
   `media/EncoderOptions` (encoder, fallback, container, pixel format, colour tags, bitrates, ProRes profile, the
   muxers' options), `media/Encoder` (GIF's palette pass, the fallback from `h264_mf` to `libopenh264`, the encoder
   used), `media/Muxer` (container by format, alpha_mode only for WebM), `export/FormatProfiles` (six profiles,
   `available()` with the reason), `export/OutputVerify` (each codec; alpha by the decoded frame), alpha by format in
   `RenderJob` and `Project`, GIF's 50 fps cap in `OutputSettings::effectiveFps`, `ExportJob::twoPasses`, the Preview's
   temporary file by the format's extension.
2. The Output section's Format row (disabled formats with their reason; Alpha disabled for MP4; the 50 fps note); the
   Quality section's note for the formats it does not apply to.
3. `export/ExportQueue` (entries, one at a time through a job maker, failures move on, Pause, the locked `queue.json`,
   Pending on reopening) and `appsettings::queueFile()`.
4. `ui/export/BatchWindow` (the list, drag to reorder, Add current, Remove, Clear done, Pause, Start) and
   `ui/MainWindowBatch.cpp` (Menu → Batch Queue, Menu → Add to Batch Queue / Ctrl+Shift+E, Export and the queue never at
   once, the quit question, probes, the walk steps `batchAdd` (with `temp`), `batchStart`, `batchPause`, `batchMove`,
   `batchClose`); its QSS. A lab run's default export folder is its own data folder.
5. `avloop-cli export --format`; its PASS line names the encoder (before the renderer, which walks read last).
6. Found on the way and mended: a walk's menu left Qt's application state Active, so a later dialog's close took the
   owner's focus (the walk runner now keeps the state Inactive while another process holds the foreground, and logs
   the step after which the focus came); at exit an estimate's render task could post to the deleted export context
   (`~MainWindow` drains render → export → render first).
7. `tests/test-export.cpp` grows GIF, APNG, MP4 H.264 and ProRes 4444 (a moving disc: no frame piles on the last);
   `tests/test-export-queue.cpp` (3: order and a failure, Pause, the file reopened and a second queue on it).

## Decisions recorded

- **D-013** — the formats and the batch as delivered; spec §0 (with `h264_mf`'s availability on this build).
- **R-51** … **R-55** — Pause, Ctrl+Shift+E's snapshot and name, GIF's rules, MP4's rate and the formats without
  quality settings, where `queue.json` lives.

## Specification delta

- §0: `D-013`. Lore: *The batch and the other formats* — seven rules.

## Verification

| Command | Result |
| --- | --- |
| `scripts\build.ps1` | built, **0 warnings** |
| `ctest` | **23/23 PASS** (new: avloop-export-queue; avloop-export 10 cases) |
| `scripts\format.ps1 -Check` | PASS (217 files) |
| `node tools/icons/check-icons.mjs` | PASS: 43 icons |
| `node tools/check-secrets.mjs` | PASS |
| `node control/phase-tools/check-loc.mjs` | PASS (the largest `src/ui/MainWindow.cpp`, 389 lines) |
| AV-012's CI run | green, 7 m 55 s |

## The walk

```
node control/lab/walk-phase.mjs execution/AV-013-walk walk-formats.mjs walk-batch.json walk-batch-restart.mjs
walk-formats.mjs (11 s): avloop-cli export --format of the counter with an ellipse (400×280, L = 90), each checked by
  the app and read back by ffprobe — GIF: gif, 90 frames, 1-bit alpha (0 partial pixels), NETSCAPE2.0 loop 0; APNG:
  apng rgba, full alpha (8376 partial), acTL plays 0; MP4: h264 yuv420p by h264_mf, opaque with #ff00ff behind
  (254,0,254), moov before mdat; ProRes: prores yuva444p12le profile 4444 (ap4h), full alpha; GIF asked 60 fps → 50/1,
  150 frames, delays 2 cs
walk-batch.json (36 s): the queue empty and saved; three entries (VP9, GIF at 60 → 50 fps with its note — picture —,
  ProRes by Ctrl+Shift+E with the template's name); Menu → Batch Queue; ProRes dragged first — picture; Start: Export
  disabled meanwhile; done,done,done — picture; an entry into a missing folder fails with its reason, the next (APNG)
  is written — picture; Clear done leaves the failed one; the window closed
walk-batch-restart.mjs (24 s): two ProRes entries, Start, a crash while the first runs (queue.json: running,pending);
  reopened: pending,pending — picture; Start: done,done; both files whole, no .part
PHASE WALKS PASSED
```

## The pictures

| Picture | What | What it shows — and what was seen in it |
| --- | --- | --- |
| `AV-013-walk/output-gif.png` | Output with GIF | Format GIF, Crop size (640×360), "GIF plays at most 50 fps", Alpha Auto (on), Background dimmed, FPS 60 |
| `AV-013-walk/quality-gif.png` | Quality with GIF | Preset dimmed, "GIF has no quality settings", CRF and Speed dimmed, Estimate ~13.3 MB |
| `AV-013-walk/batch-pending.png` | the queue | the .mov dragged first, one.webm, two.gif: formats, 640×360, Pending; Start orange, Pause and Remove dimmed |
| `AV-013-walk/batch-done.png` | after Start | three rows "Done · 39.3 MB / 0.99 MB / 12.4 MB", 100 %; Clear done enabled, Start dimmed |
| `AV-013-walk/batch-failed.png` | a failure | three.mov "Failed" (its reason the tooltip) between the done rows and four.apng "Done · 14.0 MB" |
| `AV-013-walk/batch-restart.png` | after the crash | first.mov and second.mov, MOV ProRes 4444, Pending |
| `docs/screenshots/batch.png` | README | the demo's three entries: two done, the third running with its percentage |

## The regressions

| Walk of before | Result | Mended |
| --- | --- | --- |
| AV-015 walk-gpu-off.mjs | "the exports were made on the CPU" failed: the PASS line ended with the encoder | the encoder's name goes before the renderer |
| AV-015 walk-memory.mjs (12 min, on this phase's build before the last two fixes) | PASSED: playback −9.3 MB, exports +39.6 MB (the 2nd peaked lower: 1112 against 1156 MB before; the 3rd 1152), Previews +3.0 MB, at rest −65.5 MB; VRAM flat | none |
| `tools/walks/readme-shots.json` | the owner's focus taken at the end (twice); then an exit crash after WALK PASSED | see What was built 6; then PASSED, exit 0 |
| AV-001 … AV-012, AV-015, every other walk (rerun after the last fixes) | PASSED | none |

Their pictures and logs were restored from git.

## External freshness

Not applicable — no dependency changed (the bundled FFmpeg already had every encoder; recorded in D-013).

## Files and line counts

New: `src/media/GifPalette.{h,cpp}` (191), `src/export/ExportQueue.{h,cpp}` (327), `src/ui/export/BatchWindow.{h,cpp}`
(236), `src/ui/MainWindowBatch.cpp` (148), `tests/test-export-queue.cpp` (182). Changed: `media/Encoder.*`,
`EncoderOptions.*`, `Muxer.*`, `export/ExportJob.*`, `FormatProfiles.*`, `OutputVerify.cpp`, `core/QualitySettings.*`,
`LoopSettings.*`, `AppSettings.*`, `Project.cpp`, `render/RenderJob.cpp`, `ui/panels/OutputPanel.cpp`,
`QualityPanel.cpp`, `ui/preview/PreviewWindow.cpp`, `MainWindow*`, `MainMenu.cpp`, `WalkRunner.*`,
`cli/CliExport.cpp`, `app.qss`, `tests/test-export.cpp`, CMake (`Qt6::GuiPrivate` for the walk runner),
`tools/walks/readme-shots.json`.

## Recovery log

Attempt 1 RESOLVED — gate: the build — a `'\n'` written through a Bash heredoc became a real line break.
Attempt 2 RESOLVED — gate: the build — `MainWindow.cpp` needs the queue's header; adding it through PowerShell mangled
the file's UTF-8 (restored from git, edited with Node).
Attempt 3 RESOLVED — gate: the walk's log read for the owner's name — the failure reason named the temp folder; the
probe replaces it.
Attempt 4 RESOLVED — gate: AV-015's regression — see The regressions.
Attempt 5 RESOLVED — gate: the README run's focus check — bisected with five small walks (the menu, then a Qt-active
dialog closed); the walk runner's application state.
Attempt 6 RESOLVED — gate: the README run's exit code (0xC0000005; the minidump's stack: a queued task on a worker
thread calling `invokeMethod` on a freed object) — the teardown order.

## Deviations from the phase contract

- **Code budget**: 550 planned; ~1080 new lines + ~400 changed. The named seam (AV-013a: the queue and its window)
  would move ~710. Recorded instead of split: both halves are tested and walked here.
- **The walks**: `walk-batch-restart.mjs` added to the two the contract names (a restart needs two app runs).

## Blocked legs (if any)

None.

## Staged paths

- `src/`, `tests/`, `tools/walks/readme-shots.json`, `resources/themes/app.qss`, `README.md`, `docs/SPEC.md`,
  `docs/lore/avloop.md`, `docs/screenshots/`, `phases/26-10-26-034934-AvLoop/`.

## Next

AV-014 — The installer, promoted to IN_PROGRESS in the same turn. No avloop process, session cache, lab folder or walk
temp file left; the owner's focus untouched.
