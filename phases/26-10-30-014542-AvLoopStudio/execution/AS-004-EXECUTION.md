# AS-004 — Execution Record

**Phase:** AS-004 — The timeline editor (owner item 4)
**Status:** DONE
**Date:** 2026-10-30
**Phase start commit:** 74c61d1 (`[AS-003]`)
**ADR:** none owed — every edit is `sequenceedit` through one value command (D-033)
**Environment class:** local-disposable (this PC: the build in `build/`, the samples in `samples/`, the walk's folder)

## The owner's words

*"Make this application like mini adobe premiere pro … My final goal that I have to flexible edit and cutting like
hole a video and export as transparent."*

## What was built

1. `src/ui/timeline/SequenceLanes.{h,cpp}`, `SequenceLanesInput.cpp` (new; AvLoop's `TrackLanes` removed): the
   sequence's tracks V1 … V8 on the timeline's scale; selection (Shift adds; a layer's is the Layers section's and
   follows it); move with snapping and the frame grid, across tracks; trims; Alt+drag copies; the Razor mode; Ctrl+K;
   Delete and Shift+Delete; a track header's menu (hide, show, lock, unlock); double-click to a start or a key; V1 keeps
   a clip. The walk mapper `lanes` (seconds, track with V1 = 1) and probes `lanes.tracks`, `lanes.selected`,
   `lanes.razor` (AV-031's names kept).
2. `src/ui/MainWindowLayers.cpp` — the lanes in the timeline column; C (the Razor) and Ctrl+K as window keys;
   `MainWindow::selectTool` leaves the Razor. `src/ui/MainWindowProject.cpp` — the two rows in the shortcut list.
3. `src/core/Project.*` — `length()`, the time axis; `Timeline::duration`, `MainWindow::fileEnd`, the In/Out rules and
   the In/Out tool read it; `restore` validates In/Out by it.
4. `src/ui/MainWindowMonitor.cpp` — the probe `sequence.ends`.

## Decisions recorded

- **D-034** — the timeline editor; spec §0. **RS-17 … RS-19** — V1 keeps a clip (the main video), a V1 selection is not
  the main clip; edits on the frame grid; the time axis the project's length, the strip and the bars' thumbnails left.

## Specification delta

- §0: `D-034`. README: Edit on the timeline; the shortcuts row. The lore: a drag in px is never on a frame.

## Verification

| Command | Result |
| --- | --- |
| `scripts\build.ps1` | Built Release, 0 warnings (`/W4 /WX`) |
| `ctest --test-dir build --output-on-failure --timeout 300 -R avloop` | **27 passed**, 0 failed, 75.2 s |
| `scripts\format.ps1 -Check`; `node tools/icons/check-icons.mjs` | PASS 268 files; PASS 61 icons |
| `node tools/check-secrets.mjs` | PASS (on its own, exit code 0) |
| `check-loc.mjs` | PASS: the largest code file `src/core/ClipEffects.cpp`, 425 lines |
| `validate-phase-system.mjs` (both programmes) | PASS: 33 phases; PASS: 9 phases |
| `git push origin main`; CI | see the commit `[AS-004]` and its run |

## The walk

```
node control/lab/walk-phase.mjs execution/AS-004-walk walk-timeline-edit.json walk-timeline-render.mjs → PASSED
  walk-timeline-edit.json (1440 × 900): the counter; import counter-1080p30.mp4; Insert at 2 s on V1 →
    V1:0.00-2.00,2.00-12.00,12.00-20.00 (the axis 20 s)
  C: the Razor on; a click at (16 s, V1) → the last clip cut: …@12.00+2.00 …@16.00+6.00; V: the Razor off
  a click at (5 s, V1) selects counter-1080p30@2.00; Delete → V1:0.00-2.00,12.00-16.00,16.00-20.00 (a gap); Ctrl+Z
  Shift+Delete on it → V1:0.00-2.00,2.00-6.00,6.00-10.00 (rippled)
  a drag (8 s, V1) → (8 s, V2): V2:6.00-10.00, one layer; Ctrl+drag of the end (5.99 → 5) → 2.00-5.00 (on a frame)
  a click on empty V1 deselects; the playhead at 1 s; Ctrl+K → V1:0.00-1.00,1.00-2.00,2.00-5.00; undo.count 6;
    the monitor the composite; Save As edited.avloop
  walk-timeline-render.mjs: edited.avloop version 5, V1 three clips; avloop-cli sequence --at: 0.5 s frame 15, 1.5 s
    frame 45, 3 s frame 90 (the rippled clip from its media's 2 s), 7 s frame 210 (V2 from its media's 6 s), 5.5 s
    alpha 0 (the gap); the project removed after
No avloop process, session cache, lab folder or temp file left (walk-phase.mjs).
```

## The pictures

| Picture | What | What it shows — and what was seen in it |
| --- | --- | --- |
| `AS-004-walk/timeline-inserted.png` | after Insert | V1's three bars, the axis 20 s |
| `AS-004-walk/timeline-edited.png` | after every edit | V1: 0–1 (the main clip, brighter), 1–2, 2–5; V2: 6–10; the playhead at 1 s; the total 10 s. Below, AvLoop's strip still shows the main media over the axis (RS-19) |
| `AS-004-walk/render-3s.png` | the composite at 3 s | the counter's frame 90, 00:00:03.000 |

## The regressions

| Walk of before | Result | Mended |
| --- | --- | --- |
| AV-031 `walk-tracks.json`, AV-030 `walk-layers.json`, AV-033 `walk-layer-keys.json` (the lanes now the sequence's) | PASSED (10, 9, 9 s) — again after the frame grid | none |
| AV-005 `walk-timeline.json`, AV-011 `walk-keyframes.json`, AV-001 `walk-shell.json` / `.mjs`, AV-010 `walk-settings.json` | PASSED | none |
| AS-003 `walk-shell.json` | PASSED | none |

Their logs: `AS-004-walk/regressions/`; AvLoop's walk folders put back.

## External freshness

- Not applicable — no dependency changed.

## Files and line counts

| File | Before | After |
| --- | --- | --- |
| `src/ui/timeline/TrackLanes.h` / `.cpp` | 56 / 244 | removed |
| `src/ui/timeline/SequenceLanes.h` / `.cpp` / `SequenceLanesInput.cpp` | — | **79** / **247** / **139** |
| `src/ui/MainWindowLayers.cpp`; `MainWindowMonitor.cpp` | 199; 106 | **211**; **117** |
| `src/ui/MainWindow.cpp`; `src/core/Project.cpp` | 409; 284 | **411**; **288** |

## Recovery log

- Attempt 1 RESOLVED — the trim to 5 s ended at 5.01 (a drag in px): lane edits and the Razor land on the frame grid.
- Attempt 2 RESOLVED — a script edit failed on a line the formatter had reflowed after two of its edits applied; the
  rest made against the current text (scripts check each edit; the partial state was read before going on).

## Deviations from the phase contract

- Thumbnails in the bars are not drawn (names and keys); dragging from the Project panel to a track is not built —
  Insert and Overwrite stay the way in (RS-15, RS-19).
- The track header is the lane's name chip with a right-click menu, not a column of its own (the bars keep the
  timeline's x).

## Staged paths

- `src/ui/timeline/SequenceLanes.h`, `SequenceLanes.cpp`, `SequenceLanesInput.cpp`, `TrackLanes.h` (removed),
  `TrackLanes.cpp` (removed); `src/ui/MainWindow.h`, `MainWindow.cpp`, `MainWindowLayers.cpp`, `MainWindowMonitor.cpp`,
  `MainWindowProject.cpp`, `MainWindowTimeline.cpp`, `timeline/Timeline.cpp`, `timeline/TimelineInput.cpp`,
  `tools/InOutTool.cpp`; `src/core/Project.h`, `Project.cpp`, `ProjectEdit.cpp`; `src/CMakeLists.txt`; `docs/SPEC.md`,
  `docs/lore/avloop.md`, `README.md`; AvLoop's `phases/AV-033-*.md`; the Studio's `READINGS.md`,
  `AUTONOMOUS_STATE.md`, `PROGRESS.json`, `control/00_PHASE_INDEX.md`, `phases/AS-004-*.md`, `phases/AS-005-*.md`,
  `execution/AS-003-EXECUTION.md`, `execution/AS-004-EXECUTION.md`, `execution/AS-004-walk/`.

## Next

AS-005 — the Effect Controls, promoted to IN_PROGRESS in the same turn. No avloop process, session cache or walk temp
file left.
