# AS-003 — Execution Record

**Phase:** AS-003 — The editor's shell (owner item 3)
**Status:** DONE
**Date:** 2026-10-30
**Phase start commit:** 3b2edd7 (`[AS-002]`)
**ADR:** none owed — the model's truth moves (D-033) but the commands stay value commands (spec §11.2) and the
renderer's split is AS-002's (RS-10)
**Environment class:** local-disposable (this PC: the build in `build/`, the samples in `samples/`, the walk's folder)

## The owner's words

*"I think the applicaiton stracture is not good enough to edit. Make this application like mini adobe premiere pro …
flexible edit and cutting like hole a video and export as transparent."*

## What was built

1. **The project over the sequence** (D-033, RS-14). `src/core/Project.{h,cpp}` — the truth is `Sequence seq_` and
   the media bin; AvLoop's source, range, transform, keyframes, crop, effects and layers are views kept by `sync()`,
   which says what changed (and `Aspect::Sequence`, `Aspect::Media`); `changeSequence`, `avloopShaped`, `media`,
   `addMedia`. `src/core/ProjectEdit.cpp` (new) — every edit one `SetSequenceCommand` over the sequence, with the
   aspect's own id. `src/core/SequenceView.{h,cpp}` (new) — the views and their way back: the main clip, keys over
   sequence time, the layers by id (masks kept, a copy a new clip), the frame; `settle`.
2. `src/core/Sequence.*` — the frame a rectangle of the scene (x, y, w, h), `main`, a clip's `locked`; `fromAvLoop`
   without translation, always with a main clip. `src/core/VideoLayer.h` — a layer's clip `id`.
3. **The file's version 5**: `ProjectData::sequence`, `media`; `"sequence"` written from the truth, `"media"` the bin;
   `ProjectMigrate` 4 → 5 drops version 4's derived sequence. `RenderJob` takes the project's own sequence.
4. **The Project panel** — `src/ui/panels/ProjectPanel.{h,cpp}` (new): Import…, the list, Track V1 … V8, Insert,
   Overwrite; left of the canvas in `split.left` (`ui/splitLeft`); its style in `app.qss`.
5. **The Program monitor** — `src/ui/MonitorComposite.h`, `src/ui/MainWindowMonitor.cpp` (new): the composite at the
   playhead beyond AvLoop's one video; `Canvas::setVideoHidden`; the walk step `import` and the probes
   `project.media`, `sequence.clips`, `sequence.v1`, `sequence.tracks`, `monitor.composite`.
6. **The Frame tool** — `src/ui/options/CropOptions.cpp`: Frame size on the Rectangle crop's bar.
7. `src/render/SequenceRenderer.cpp` — two threads for decoders above V1 (RS-16: AS-002's memory regression).
8. Tests: `test-sequence` (+3 cases: the project over the sequence, views and back, keys over sequence time),
   `test-commands` (changed says Transform and Sequence), `test-project-json` (version 5).

## Decisions recorded

- **D-033** — the editor's shell; spec §0. **RS-14 … RS-16** — the truth and its views; the panel, the monitor and
  the Frame tool; the composite's decoder threads.

## Specification delta

- §0: `D-033`. README: Frame size, the Project panel, the composite. Screenshots retaken (`readme-shots.json`, 11
  PNGs, the largest 205 KB). The lore: a const_cast into a QList copy; the ½-px landing; regressions miss what they do
  not run; CI validates every programme's phases.

## Verification

| Command | Result |
| --- | --- |
| `scripts\build.ps1` | Built Release, 0 warnings (`/W4 /WX`) |
| `ctest --test-dir build --output-on-failure --timeout 300 -R avloop` | **27 passed**, 0 failed, 75.2 s |
| `scripts\format.ps1 -Check`; `node tools/icons/check-icons.mjs` | PASS 267 files; PASS 61 icons |
| `node tools/check-secrets.mjs` | PASS (on its own, exit code 0) |
| `check-loc.mjs` | PASS: the largest code file `src/core/ClipEffects.cpp`, 425 lines |
| `validate-phase-system.mjs` (both programmes; CI now runs both) | PASS: 33 phases; PASS: 9 phases |
| `git push origin main`; CI | pushed `74c61d1`; CI green (run 40956823795, 10 m 28 s) |

## The walk

```
node control/lab/walk-phase.mjs execution/AS-003-walk walk-shell.json → PASSED, 24 checks
  1440 × 860; the counter opened: the bin 1, one clip, AvLoop's canvas (monitor.composite false)
  import counter-1080p30.mp4: the bin 2, the new one selected
  playhead 2 s, Insert on V1: 3 clips — "counter-640p30@0.00+0.00 counter-1080p30@2.00+0.00 counter-640p30@12.00+2.00";
    the source still counter-640p30.mp4; one Undo step; the monitor the composite
  playhead 3 s and 1 s: pictures; Ctrl+Z: one clip, AvLoop's canvas back
  Track V2, Overwrite at 1 s: 2 tracks, 1 layer, AvLoop's canvas (a layer is AvLoop's shape)
  M, Frame size 1080 × 1920: crop 1080 × 1920 centred at x 320, output 1080x1920
The Studio's walks again on the new model: AS-001 walk-sequence.mjs PASSED (33), AS-002 walk-composite-render.mjs
PASSED (8; 194.4 fps, +1.3 MB). No avloop process, session cache, lab folder or temp file left.
```

## The pictures

| Picture | What | What it shows — and what was seen in it |
| --- | --- | --- |
| `AS-003-walk/shell-open.png` | the main window | the Project panel left (counter-640p30.mp4, 640 × 360 · 10.0 s; Import…, Track V1, Insert, Overwrite), the rail, the canvas, the side panel — AvLoop's layout otherwise |
| `AS-003-walk/monitor-composite-3s.png` | the monitor at 3 s | the 1080p clip at 100 % centred on the 640 × 360 frame (its middle bars, larger than the frame) — the composite, not the main video |
| `AS-003-walk/frame-1080x1920.png` | after Frame size 1080 × 1920 | the crop 1080 × 1920 at −220, −780, "1080 × 1920 · alpha"; the V2 clip in its lane |
| `docs/screenshots/main-window.png` | the README's | the Project panel beside the rounded crop of readme-demo.mp4 |

## The regressions

| Walk of before | Result | Mended |
| --- | --- | --- |
| AvLoop's 45 walks on the new model (before the panel): AV-001 … AV-033 but install, name, release (R-75), the memory walks of AV-007 and AV-015, AV-021 | 44 PASSED; AV-031 `walk-tracks-memory.mjs` FAILED +55.5 MB | AS-002's composite (8 decoders, a thread a core): two threads above V1 → PASSED +34.2 MB (RS-16) |
| after the panel: 29 GUI walks again | 27 PASSED; AV-024 `walk-shapes.json`, AV-029 `walk-align.json` 1 px off | read anew: the ½-px landing moved with the canvas's width (x 101 → 100, 141 → 140, 221 → 220, 401 → 400); both PASSED |
| AS-001 `walk-sequence.mjs`, AS-002 `walk-composite-render.mjs` | 9 and 4 FAILED | read anew: version 5, the scene's px (D-033); PASSED |

Their logs: `AS-003-walk/regressions/`; AvLoop's walk folders put back but the two mended walks.

## External freshness

- Not applicable — no dependency changed.

## Files and line counts

| File | Before | After |
| --- | --- | --- |
| `src/core/Project.cpp`; `ProjectEdit.cpp` | 380; — | **284**; **182** |
| `src/core/Project.h`; `Commands.h` | 194; 78 | **209**; **77** |
| `src/core/SequenceView.h` / `.cpp` | — | **37** / **133** |
| `src/core/Sequence.cpp`; `ProjectJson.cpp` | 260; 218 | **246**; **245** |
| `src/ui/panels/ProjectPanel.h` / `.cpp` | — | **41** / **157** |
| `src/ui/MainWindowMonitor.cpp`; `MonitorComposite.h` | — | **106**; **44** |
| `src/ui/MainWindow.cpp`; `options/CropOptions.cpp` | 395; 213 | **409**; **238** |
| `tests/test-sequence.cpp` | 196 | **276** |

## Recovery log

- Attempt 1 RESOLVED — gate: AS-002's CI red (AvLoop's validator: AV-033 read the removed `LayerPass.cpp`); repair:
  its scope names `SequenceRenderer.cpp`, CI validates the Studio's phases too.
- Attempt 2 RESOLVED — 4 suites failed: a project without a path had no main clip (edits dropped) → V1 always holds
  one; then edits changed the live sequence without a step — a const_cast into a QList copy (lore) → detaching access.
- Attempt 3 RESOLVED — AV-031's memory walk (+55.5 MB) → two decoder threads above V1.
- Attempt 4 RESOLVED — the walk's `source.file` (a full path) and `layers.track` (the selected layer's) read as such.

## Deviations from the phase contract

- Dragging from the Project panel to a track comes with the timeline editor (AS-004, RS-15); Insert and Overwrite
  place at the playhead now.
- The Frame tool is the Rectangle crop with frame sizes, not a tool of its own on F (RS-15).
- The project's move to the sequence (RS-9's end) was done here, before the panels it serves.

## Staged paths

- `src/core/`: `Project.h`, `Project.cpp`, `ProjectEdit.cpp`, `Commands.h`, `Sequence.h`, `Sequence.cpp`,
  `SequenceView.h`, `SequenceView.cpp`, `VideoLayer.h`, `ProjectJson.h`, `ProjectJson.cpp`, `ProjectMigrate.cpp`;
  `src/render/RenderJob.cpp`, `src/render/FrameRenderer.cpp`, `src/render/SequenceRenderer.cpp`;
  `src/cli/CliSequence.cpp`; `src/ui/`: `MainWindow.h`, `MainWindow.cpp`, `MainWindowLayers.cpp`,
  `MainWindowMonitor.cpp`, `MonitorComposite.h`, `canvas/Canvas.h`, `canvas/CanvasGl.cpp`, `panels/ProjectPanel.h`,
  `panels/ProjectPanel.cpp`, `options/CropOptions.cpp`; `src/CMakeLists.txt`; `resources/themes/app.qss`;
  `tests/test-sequence.cpp`, `tests/test-commands.cpp`, `tests/test-project-json.cpp`, `tests/test-composite.cpp`;
  `.github/workflows/ci.yml`; `docs/SPEC.md`, `docs/lore/avloop.md`, `README.md`, `docs/screenshots/*.png`;
  AvLoop's `phases/AV-033-*.md`, `execution/AV-024-walk/walk-shapes.json`, `execution/AV-029-walk/walk-align.json`;
  the Studio's `READINGS.md`, `AUTONOMOUS_STATE.md`, `PROGRESS.json`, `control/00_PHASE_INDEX.md`,
  `phases/AS-003-*.md`, `phases/AS-004-*.md`, `execution/AS-001-walk/`, `execution/AS-002-walk/`,
  `execution/AS-002-EXECUTION.md`, `execution/AS-003-EXECUTION.md`, `execution/AS-003-walk/`.

## Next

AS-004 — the timeline editor, promoted to IN_PROGRESS in the same turn. No avloop process, session cache or walk temp
file left.
