# AV-005 — Execution Record

**Phase:** AV-005 — The timeline (owner item 5)
**Status:** DONE
**Date:** 2026-10-26
**Phase start commit:** `9ab247f` (*[AV-004] The crop tools*)
**ADR:** none owed (AVADR-0002's limits hold: thumbnails ≤ 400 × 160 × 90, the playback ring 6 frames)
**Environment class:** local-disposable (this PC: `build/`, `samples/`, the walk's folder; lab windows behind the
owner's, never activated)

## The owner's words

*"Take any video, cut it at a specific In/Out."* — *"Use the GPU."* — the spec's §7: *"The timeline is
frame-accurate: the frame under the playhead is exactly the frame the canvas shows."*

## What was built

1. `src/core/Timecode` (the output grid, `HH:MM:SS.FF` both ways, `372f`, the default output rate) and
   `core/TimeRange` (§6.4's rules in whole grid frames: withIn/withOut, clear, the handles' clamps, the bar's move,
   a range read from a file); `Project::range`, `changeRange` (SetRange), `outputFps`; `.avloop` `range`.
2. `src/media/Decoder::keyframeAt` (the scrub's fast frame); `DecodeWorker` request modes (Exact, Fast) and playback
   — decoding ahead into a ring of six in display order, laps for the loop, the end mark, `takeDue` showing frames in
   order; `media/ThumbnailWorker` (its own software decoder and thread, keyframe seeks, ≤ 400 samples of 160 × 90,
   a new order replaces what waits, released with the video).
3. `src/ui/timeline/Timeline` (+ `TimelinePaint.cpp`, `TimelineInput.cpp`, `TimelineScale` — the ruler's steps,
   pure and tested), `PlaybackClock` (QElapsedTimer, a 4 ms precise timer, loop laps, drops counted),
   `Transport` with `ui/widgets/TimeEdit`; `ui/tools/InOutTool.cpp`; `ui/MainWindowTimeline.cpp` (the playhead and
   the canvas, the keys, the status bar's "Dropped: N", probes and the `timeline` mapper).
4. `Tool::wantsKey` and `Canvas::event`: the crop's arrows come before the window's frame steps.
5. The lab: the walk's `key` step focuses its target; `waitFor` takes every test `expect` takes (`near`, `min` …);
   lab windows go behind the owner's windows with `WS_EX_NOACTIVATE`.
6. `media/SourceNormalizer`: `accurate_rnd` dropped (see Decisions).
7. Tests: `test-timecode` (6: formats, parsing — every frame's timecode reads back —, the grid, the range rules, the
   ruler's steps at every zoom of four rates and both label kinds, ticks and labels); `test-commands` + SetRange.

## Decisions recorded

- **D-005** — the timeline as delivered; spec §0.
- **R-24** the range is In up to Out (I the first frame, O the last); **R-25** "Dropped" = never shown, frames shown in
  order and dropped only beyond two frames late; **R-26** the timeline repaints at most 30 a second while playing;
  **R-27** the scrub's two speeds — the keyframe while moving, the exact frame after 150 ms at rest or on release.
- **No `accurate_rnd` in the normalizer's scale**: it disables swscale's SIMD (10 ms a 1080p frame against 0.7 ms)
  for rounding within 1/255; `full_chroma_int` stays. AV-002's tests pass unchanged.
- **I and O set In and Out, not the tool** (§6.4): the rail's In and Out buttons select the tools; AV-001's shell
  walk was mended to click them.

## Specification delta

- §0: `D-005`.
- Lore: *The timeline and playback* — eight rules (Ninja's stale objects; `accurate_rnd`; raster repaints beside a
  QOpenGLWidget; `qInfo` in a GUI build; clicks on a frame boundary; keys and focus; `--mem-log` in ms; lab windows
  behind the owner's).

## Verification

| Command | Result |
| --- | --- |
| `scripts\build.ps1` | built, **0 warnings** (and once from clean objects — see Recovery) |
| `ctest -LE gpu` | **11/11 PASS** |
| `scripts\format.ps1 -Check` | PASS: 118 files |
| `node tools/icons/check-icons.mjs` | PASS: 43 icons |
| `node tools/check-secrets.mjs` | PASS: 298 files |
| `node control/phase-tools/check-loc.mjs` | PASS: 151 code files, the largest `src/ui/WalkSteps.cpp` 360 lines |
| AV-004's CI run | green, 5 m 3 s |

## The walk

```
node control/lab/walk-phase.mjs execution/AV-005-walk walk-timeline.json walk-playback.mjs
walk-timeline.json (153 steps, 78 expectations; counter-1080p30, 10 s, 300 frames):
  the whole file at load, playhead 0, "(300 f)", no label overlap; a click at 2.5 s → the exact frame 2.5 shown
  I → In 2.5; →×3 → 2.6 (that frame shown); Shift+→ → 2.9333; O → Out 2.9667 "(14 f)"; Home/End → 2.5 / 2.9333;
  Ctrl+End → 9.9667 (shown), Ctrl+Home → 0; Alt+I, Alt+O → the whole file; four Undo steps
  In after Out → Out to the end; Out before In → In to the start
  the In tool's click → In 4 (Out to the end), the Out tool's click → Out 6.0333 "61 frames", Out typed 7
  In's handle dragged to 3 (frame 3 shown); the bar dragged +1 s → 4–8; Ctrl+Z, Ctrl+Z, Ctrl+Y
  zoom: fit, ×1.25, ×1.95, 360 px/s (12 px a frame), frame numbers — 0 overlaps each (five pictures); \ fits the
  file, Shift+\ the range
  typed 150f → 5.0 shown; Next, Prev, Go to In 3, Go to Out 6.9667
  Space 2.5 s from 6.9667 round 3–7 → 75 shown, 0 dropped; L off/on
  the crop's → moves the crop (playhead kept); with the Selection → a frame step (crop kept)
  thumbnails made; "the owner's focus untouched"
walk-playback.mjs:
  1080p30, 10 s: 304 shown, 0 dropped, "Dropped: 0"; 1080p60, 10 s: 607 shown, 0 dropped
  a minute round In 2 s – Out 5 s: 1804 shown, 0 dropped; Private Bytes peaks 578/577/577 MB (−1.1 MB),
  VRAM 153/153/153 MB (0 MB) — memory-playback.csv
PHASE WALKS PASSED
```

Before the lab windows went behind the owner's: the same 60 fps leg passed visible on top (608 shown, 0 dropped).

## The pictures

| Picture | What | What it shows — and what was seen in it |
| --- | --- | --- |
| `AV-005-walk/timeline-start.png` | the whole window at load | the ruler 00:00:00.00 … 00:00:09.00, thumbnails filling from the visible middle outwards (`panel2` where not yet made), the whole file in range, "(300 f)" |
| `AV-005-walk/zoom-1-fit.png` … `zoom-3.png` | the timeline area, fit to ×1.95 | labels a second apart, then half and fifth seconds; In 3 – Out 7 bracketed, outside darker; never touching labels |
| `AV-005-walk/zoom-4-max.png` | 12 px a frame | labels every 10 frames (00:00:03.10 …), the thumbnails' cells per keyframe |
| `AV-005-walk/zoom-5-max-frames.png` | frame numbers | 85, 90, 95 … every 5 frames, the playhead at 150 |
| `AV-005-walk/timeline-end.png` | after the crop's arrows | the crop dimming the frame, the Selection's border, "Dropped: 0" in the status bar, the time field whole (00:00:04.01) |
| `docs/screenshots/main-window.png` | README | the Rounded crop, In 2 s – Out 7.5 s on a full thumbnail strip, the playhead at 4.2 s (1360 px, 284 KB) |

## The regressions

| Walk of before | Result | Mended |
| --- | --- | --- |
| AV-004 walk-crop.json | PASSED | none — the arrows reach the crop through `wantsKey` |
| AV-003 walk-selection.json | PASSED | none |
| AV-001 walk-shell.json / .mjs | PASSED | I and O no longer select tools (§6.4): the walk clicks `tool.in` / `tool.out` |
| AV-002 walk-open.json | PASSED | none |

Their pictures and logs were regenerated and restored from git; only AV-001's mended walk file changed.

## External freshness

Not applicable — no dependency changed.

## Files and line counts

New: `src/core/{Timecode,TimeRange}.{h,cpp}`, `src/media/ThumbnailWorker.{h,cpp}`,
`src/ui/timeline/{Timeline.h,Timeline.cpp,TimelinePaint.cpp,TimelineInput.cpp,TimelineScale.h,TimelineScale.cpp,
PlaybackClock.h,PlaybackClock.cpp}`, `src/ui/widgets/TimeEdit.{h,cpp}`, `src/ui/tools/InOutTool.cpp`,
`src/ui/MainWindowTimeline.cpp`, `tests/test-timecode.cpp`. Changed: `Project.*`, `ProjectJson.*`, `Commands.h`,
`Decoder*`, `DecodeWorker.*`, `SourceNormalizer.cpp`, `Canvas.*`, `CropTool.cpp`, `Tool.h`, `Transport.*`,
`MainWindow.*`, `MainWindowActions.cpp`, `MainWindowCanvas.cpp`, `WalkRunner.*`, `WalkSteps.cpp`, `app.qss`, the CMake
files, `test-commands.cpp`.

## Recovery log

Attempt 1 RESOLVED — gate: the first walk crashed (0xC0000005) opening a video. cdb's stack: a QWidget made with a
garbage parent inside the canvas's paint. Three tool objects predated the new `Tool::wantsKey` (Ninja's deps log was
out of date), so `previewCrop()` landed on the old vtable's `createOptions()`. All objects deleted and rebuilt.
Attempt 2 RESOLVED — gate: walk-timeline (2 failures): clicks exactly on a frame boundary landed a pixel early; the
walk's clicks aim at mid-frame.
Attempt 3 RESOLVED — gate: 60 fps playback dropped 26 frames. Measured: decoding kept the ring full; the conversion's
`accurate_rnd` cost 10 ms a frame (dropped → 15 drops left); the UI thread's ticks came 14–26 ms apart — a raster
repaint beside the GL canvas costs a whole-window composition (window updates 11–28 ms, paints 2–9 ms of it). The
clock now shows frames in order (drops only beyond two frames late), the timeline repaints at most 30 a second and
paints opaque → 0 dropped at 60 fps. Swap interval 0 and skipping mipmaps were tried and made no difference; the
temporary tracing was taken out.
Attempt 4 RESOLVED — gate: the focus check failed in two of the minute-long runs: the full-screen lab window caught
the owner's clicks. Lab windows now sit behind the owner's windows with `WS_EX_NOACTIVATE`.

## Deviations from the phase contract

- **Code budget**: 600 planned; ~1,610 new lines in `src` + ~420 changed (comments included). The named seam (AV-005a:
  the thumbnail strip and its worker) would move ~250. Recorded instead of split.
- **The crossfade band and the keyframe row** are drawn empty (AV-006, AV-011), as the contract says.

## Blocked legs (if any)

None.

## Staged paths

- `src/`, `tests/`, `resources/themes/app.qss`, `tools/walks/readme-shots.json`, `README.md`, `docs/SPEC.md`,
  `docs/lore/avloop.md`, `docs/screenshots/main-window.png`, `phases/26-10-26-034934-AvLoop/`.

## Next

AV-006 — The loop engine, promoted to IN_PROGRESS in the same turn. No avloop process, session cache or walk temp file
left; the owner's focus untouched.
