# AV-012 — Execution Record

**Phase:** AV-012 — The Loop Finder (owner item 12)
**Status:** DONE
**Date:** 2026-10-26
**Phase start commit:** `4dc723c` (*[AV-011] The keyframes: position and scale over time*)
**ADR:** none owed
**Environment class:** local-disposable (this PC: `build/`, `samples/` (+ `loop4s.mp4`), the walks' temp folders; lab
windows behind the owner's, never activated)

## The owner's words

*"Find the best In/Out pairs automatically."*

## What was built

1. `src/export/LoopFinder` — fingerprints (a software decoder of its own, ≤ 640 px; 64×64 grey of the export's frame
   with the transform and keyframes at each time; 4 KB a frame), every pair scored (frames + motions), rows in
   parallel (`QtConcurrent::blockingMapped`) in chunks with progress and cancel, a histogram for the 95th percentile,
   the best five by suppression within a second at both ends, ties to the shorter loop; the ends' thumbnails.
2. `src/ui/export/LoopFinderWindow` (search range, lengths, Find, progress, Cancel, the list with both ends and the
   score; click = SetRange, double-click = and Preview) and `ui/MainWindowFinder.cpp` (Menu → Loop Finder, Ctrl+L,
   enabled with a video; a new video closes it; probes; the walk steps `finderChoose`, `finderClose`); its QSS.
3. `avloop-cli find`; `tools/media/make-samples.mjs` → `loop4s.mp4`.
4. `tests/test-loopfinder.cpp` (4: the period first ±2 frames before the tying 8 s, the separation, the bounds,
   Cancel; the sample when present).
5. Found on the way and mended: the log kept every lab run's temp folder (closed before `main` returns now; walk-phase
   fails a leftover lab folder; walk-recover removes the folders its crashed runs leave); a message of the teardown
   could reach the log's statics.

## Decisions recorded

- **D-012** — the Loop Finder as delivered; spec §0.
- **R-50** the 0–100 score by the 95th percentile, ties to the shorter loop, a pair's meaning.

## Specification delta

- §0: `D-012`. Lore: *The Loop Finder* — three rules.

## Verification

| Command | Result |
| --- | --- |
| `scripts\build.ps1` | built, **0 warnings** |
| `ctest` | **22/22 PASS** (new: avloop-loopfinder) |
| `scripts\format.ps1 -Check` | PASS (209 files) |
| `node tools/icons/check-icons.mjs` | PASS: 43 icons |
| `node tools/check-secrets.mjs` | PASS |
| `node control/phase-tools/check-loc.mjs` | PASS (the largest `src/ui/MainWindowProject.cpp`, 388 lines) |
| AV-011's CI run | green, 8 m 11 s |

## The walk

```
node control/lab/walk-phase.mjs execution/AV-012-walk walk-finder.json walk-finder.mjs
walk-finder.json (7 s): without a video no Finder; loop4s.mp4, Ctrl+L, the whole file, Find: done, 5 loops, the best
  4.00 s, score 100 — picture; row 1 → In 0.033, Out 4.033; Ctrl+Z → In 0; a double-click on row 2 → In 1.033 and the
  Preview open (60+ frames made), closed; loops up to 30 s, Find, Cancel → idle; the window closed
walk-finder.mjs (18 s): avloop-cli find — the whole file: 0.033–4.033 (4.000 s, raw 0, score 100) and the next four
  4 s loops a second apart; a project's crop of the orange box's lane: 1.033–5.033 (4.000 s); loops of 2–3 s: within
  the bounds, the best raw 6.571 (score 48); the window opened, searched and closed four times under --mem-log:
  peaks 262 / 264 / 269 / 265 MB, floors after each close 261 / 263 / 264 / 265 MB (241 MB before the first)
PHASE WALKS PASSED
```

## The pictures

| Picture | What | What it shows — and what was seen in it |
| --- | --- | --- |
| `AV-012-walk/finder.png` (and `docs/screenshots/finder.png`) | the Loop Finder | Search: The whole file, Length 2.00–10.00 s, Find; "The best 5 loop(s)…"; five rows, each the first and the last frame (the same picture — the loop joins), "In 00:00:00.01 · Out 00:00:04.01 · 4.00 s · score 100" to 4.01–8.01 |

## The regressions

| Walk of before | Result | Mended |
| --- | --- | --- |
| AV-010 walk-settings.json | the app crashed at exit once (all steps passed) | the log closed before `main` returns; four runs clean |
| AV-010 walk-recover.mjs | would leave its crashed runs' lab folders | it removes them |
| AV-005 walk-playback.mjs | one frame dropped at 60 fps once; passed on the rerun | none (load) |
| AV-001 … AV-011, every other walk | PASSED (walk-phase now also checks lab folders) | none |

Their pictures and logs were restored from git.

## External freshness

Not applicable — no dependency changed.

## Files and line counts

New: `src/export/LoopFinder.{h,cpp}` (251), `src/ui/export/LoopFinderWindow.{h,cpp}` (199),
`src/ui/MainWindowFinder.cpp` (70), `src/cli/CliFind.cpp` (101), `tests/test-loopfinder.cpp` (96). Changed:
`MainWindow.*`, `MainMenu.cpp`, `MainWindowActions.cpp`, `app/Diagnostics.*`, `app/main.cpp`, `cli/main.cpp`,
`app.qss`, `tools/media/make-samples.mjs`, `control/lab/walk-phase.mjs`, `AV-010-walk/walk-recover.mjs`, CMake.

## Recovery log

Attempt 1 RESOLVED — gate: the build — frame numbers are 64-bit (`nearestFrame`).
Attempt 2 RESOLVED — gate: walk-finder.json (In/Out) — the whole file starts at 0: the best is 0.033–4.033.
Attempt 3 RESOLVED — gate: the picture — Cancel stretched across the idle window, the answers' text too dim: Cancel
shows only while searching; the list themed.
Attempt 4 RESOLVED — gate: AV-010 walk-settings (an exit crash) and 134 lab folders left — see What was built 5.

## Deviations from the phase contract

- **Code budget**: 500 planned; ~720 new lines + ~120 changed. The named seam (AV-012a: the window) would move ~270.
  Recorded instead of split.

## Blocked legs (if any)

None.

## Staged paths

- `src/`, `tests/`, `tools/media/make-samples.mjs`, `resources/themes/app.qss`, `README.md`, `docs/SPEC.md`,
  `docs/lore/avloop.md`, `docs/screenshots/finder.png`, `phases/26-10-26-034934-AvLoop/`.

## Next

AV-013 — Formats and the batch queue, promoted to IN_PROGRESS in the same turn. No avloop process, session cache, lab
folder or walk temp file left; the owner's focus untouched.
