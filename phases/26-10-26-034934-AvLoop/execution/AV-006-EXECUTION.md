# AV-006 — Execution Record

**Phase:** AV-006 — The loop engine (owner item 6)
**Status:** DONE
**Date:** 2026-10-26
**Phase start commit:** `6c1f4ff` (*[AV-005] The timeline*)
**ADR:** none owed (the Interpolator's frames live in the disk cache under AVADR-0002's budget)
**Environment class:** local-disposable (this PC: `build/`, `samples/`, the walk's folder; lab windows behind the
owner's, never activated)

## The owner's words

*"The point is that a resized WebM is exported that feels like a loop."* — *"Crossfade inside the selected
range."* — *"There will be a frame rate setting."*

## What was built

1. `src/core/LoopSettings` (mode, F, curve, T, Speed, method; ids for the file) and `OutputSettings` (the frame
   rate: Source — the stream's, a variable rate's nearest common one, at most 60 — or a chosen/Custom rate);
   `core/LoopMap` (D, L, F/T clamps, `pick` for every mode, the smooth turn's position, `LoopPlan`); `core/TimeMap`
   (`FrameTimes` constant or by PTS, τ(i), Nearest / Frame Blend / Motion); `Project::changeLoop` (SetLoop) and
   `changeOutput` (SetOutput; a new rate moves In and Out to its grid in the same Undo step); `.avloop` `loop` and
   `output.fps`.
2. `src/media/Interpolator` — the `minterpolate` pre-pass (mi_mode=mci, mc_mode=aobmc, vsbmc=1) at the output rate,
   Speed by stretched timestamps, alpha on its own branch, frames into the disk cache under a key of every input,
   progress and cancel.
3. `src/ui/panels/LoopPanel.cpp` (Mode, Crossfade s ↔ frames, Curve, Turn, Speed, FPS method, the slow-motion hint,
   the summary, the clamp flash; rows a mode does not use hidden) and `OutputPanel.cpp` (FPS: Source (n), the list,
   Custom…); `Section::addRow` keeps combo boxes from widening the panel.
4. The timeline's crossfade band (gradient before Out, dashed outline after In, its edge dragged sets F) and the
   smooth turn's quiet ends; the editor's playback at Speed; the timeline repaints on loop and rate changes.
5. Tests: `test-loopmap` (5), `test-timemap` (7), `test-interpolator` (4, on the samples).

## Decisions recorded

- **D-006** — the loop engine as delivered; spec §0.
- **R-6's FPS part** — Source by default, at most 60 (`OutputSettings::effectiveFps`); a variable rate takes the
  nearest of 15 … 60.
- **R-28** Crossfade is the default loop; **R-29** a new rate moves In/Out to its grid in the same Undo step, F and T
  stay in seconds; **R-30** the Motion pre-pass runs when the Preview or the export needs its frames (AV-008, AV-009).
- **F and T are stored in seconds** (§11.1's `fade`, `turn`) and worked out in frames by `LoopPlan`, so a rate change
  keeps their length in time.

## Specification delta

- §0: `D-006`.
- Lore: *The loop engine* — four rules (minterpolate needs the frame after; minterpolate is YUV, alpha apart; combo
  boxes and the panel's width; Crossfade's ends in tests).

## Verification

| Command | Result |
| --- | --- |
| `scripts\build.ps1` | built, **0 warnings** |
| `ctest -LE gpu` | **14/14 PASS** |
| `scripts\format.ps1 -Check` | PASS: 131 files |
| `node tools/icons/check-icons.mjs` | PASS: 43 icons |
| `node tools/check-secrets.mjs` | PASS |
| `node control/phase-tools/check-loc.mjs` | PASS: every code file under 600 |
| AV-005's CI run | green, 5 m 38 s |

## The walk

```
node control/lab/walk-phase.mjs execution/AV-006-walk walk-loop.json
walk-loop.json (74 steps; counter-640p30, In 2 s – Out 7 s):
  Crossfade by default: D150 F15 L135, "Loop 4.50 s · 135 frames @ 30 fps"
  Simple 150 frames 5.00 s; Ping-pong 298 frames 9.93 s; Ping-pong + Smooth turn 298 (T 9) — picture
  F typed as 30 frames → F30 L120; 10 s typed → clamped to F75, the field flashed; 0.5 s → F15
  the band's edge at 6.5 s dragged to 6.0 s → F30 (edge 6.0); Ctrl+Z → F15, Ctrl+Y → F30 — pictures before/after
  Speed 0.5 → D300 F30 L270, the slow-motion hint; Motion Interpolation → no hint — picture of the panel
  60 fps → "540 frames @ 60 fps"; 25 → "225 frames @ 25 fps", In 2 and Out 7 kept; Ctrl+Z → 60; Source → 30;
  Custom 47.952
  Home, Space for 1 s at 0.5× → the playhead at 2.5 s (± 0.12), 0 dropped
  "the owner's focus untouched"
PHASE WALKS PASSED
```

## The pictures

| Picture | What | What it shows — and what was seen in it |
| --- | --- | --- |
| `AV-006-walk/loop-smooth-turn.png` | Ping-pong + Smooth turn | the Loop rows Mode, Turn 0.30 s, Speed, FPS method; "Loop 9.93 s · 298 frames @ 30 fps"; the quiet fills at both ends of the range on the timeline |
| `AV-006-walk/band-before.png`, `band-after.png` | the crossfade band | the orange gradient rising to Out and the dashed outline after In; after the drag both twice as long |
| `AV-006-walk/loop-panel.png` | Crossfade, 0.5×, Motion | Crossfade 1.00 s / 30 f, Curve Smooth, Speed 0.50×, "Loop 9.00 s · 270 frames @ 30 fps", Output FPS "Source (30)"; every row inside the panel |

## The regressions

| Walk of before | Result | Mended |
| --- | --- | --- |
| AV-005 walk-timeline.json, walk-playback.mjs | PASSED | none |
| AV-004 walk-crop.json | PASSED | none |
| AV-003 walk-selection.json | PASSED | none |
| AV-001 walk-shell.json / .mjs | PASSED | none |
| AV-002 walk-open.json | PASSED | none |

Their pictures and logs were regenerated and restored from git.

## External freshness

Not applicable — no dependency changed (`minterpolate` is part of the fetched LGPL FFmpeg 7.1.5).

## Files and line counts

New: `src/core/{LoopSettings,LoopMap,TimeMap}.{h,cpp}`, `src/media/Interpolator.{h,cpp}`,
`src/ui/panels/{LoopPanel,OutputPanel}.cpp`, `tests/test-{loopmap,timemap,interpolator}.cpp`. Changed:
`Project.*`, `ProjectJson.*`, `Commands.h`, `Timecode.*`, `Timeline.*`, `TimelinePaint.cpp`, `TimelineInput.cpp`,
`MainWindowTimeline.cpp`, `MainWindowCanvas.cpp`, `MainWindowActions.cpp`, `SidePanel.*`, `Section.cpp`,
`CropPanel.cpp`, `app.qss`, the CMake files.

## Recovery log

Attempt 1 RESOLVED — gate: tests (3 suites): two expectations of mine were wrong (O[0] is itself blended when D = 2F;
0.16 s is an exact tie between 0.15 and 0.17), and the pre-pass stopped one frame short (minterpolate needs the input
after a frame) — fed four source frames further.
Attempt 2 RESOLVED — gate: the panel picture: the side panel's rows were clipped (a long combo item set the column's
width); combo boxes now size to a short minimum.

## Deviations from the phase contract

- **Code budget**: 500 planned; ~940 new lines in `src` + ~190 changed. The named seam (AV-006a: the Interpolator)
  would move ~210. Recorded instead of split.
- **The Interpolator's progress and cancel are an API** here; the Preview (AV-008) and the export (AV-009) show them
  when they run it (R-30).

## Blocked legs (if any)

None.

## Staged paths

- `src/`, `tests/`, `resources/themes/app.qss`, `README.md`, `docs/SPEC.md`, `docs/lore/avloop.md`,
  `phases/26-10-26-034934-AvLoop/`.

## Next

AV-007 — The renderer, promoted to IN_PROGRESS in the same turn. No avloop process, session cache or walk temp file
left; the owner's focus untouched.
