# AV-018 — Execution Record

**Phase:** AV-018 — The wheel and the keyframe copy (owner item 16, late)
**Status:** DONE
**Date:** 2026-10-27
**Phase start commit:** `b4e9ec4` (*[AV-017] The name and the About box: AvLoop by Rubel, the developer's contact, 1.0.1*)
**ADR:** none owed
**Environment class:** local-disposable (this PC: `build/` (git-ignored), the samples, the walk's folder; the owner's
own AvLoop open from `build\bin` throughout — its locked exe renamed aside, never closed)

## The owner's words

*"mouse wheen move will zoom in out targeted to the mous pointer area in canvas"*; then, while it was being built,
*"alt drag will copy a keyframe in time line"*.

## What was built

1. `src/ui/canvas/Canvas.{h,cpp}` — the wheel, Ctrl or not, zooms a step a notch about the pointer; the deltas of a
   finer wheel added up to a notch (`wheelZoom_`); Shift+wheel and a sideways wheel pan across (R-63).
2. `src/core/Keyframes.{h,cpp}` — `keyframes::copied`: a keyframe copied to a time, refused where one already is.
3. `src/core/Project.{h,cpp}` — `copyKeyframe`: "Copy a keyframe" with MoveKeyframe's id, so the copy's moves in the
   same drag merge into one Undo step.
4. `src/ui/timeline/Timeline.h`, `TimelineInput.cpp` — Alt on a diamond: the first step off its frame copies, the copy
   then moves; the copy cursor with Alt over a diamond; a drag follows its keyframe only when the move or copy was
   taken (the mend of AV-011's drag, R-64).
5. `tests/test-keyframes.cpp` — `copied` (in order, values and Ease kept, refused onto itself and onto another); the
   project's copy and its move one step, Undo.
6. `control/lab/av-lab.mjs`, `walk-phase.mjs` — processes listed with their PID; "no AvLoop left" counts only those
   not running before the walks.
7. Walks: `AV-018-walk/walk-wheel.json`, `walk-key-copy.json`.
8. The programme: item 16 (`README.md` §1–§3), `control/19_SHAPES_THE_WHEEL_AND_KEY_COPY.md`, this phase,
   `OWNER_ITEMS` 16, REQ-AV-023, D-016, R-63 … R-65, owner actions 35–36.

## Decisions recorded

- **D-016** — the wheel and the keyframe copy; spec §0.
- **R-63** the canvas's wheel zooms, Ctrl or not; Shift/sideways pans; the timeline's wheel unchanged. **R-64** Alt+drag
  copies, one Undo step; a refused drag keeps its own. **R-65** no new version, no Release.

## Specification delta

- §0: `D-016`. The lore: *The wheel and the keyframe copy (late)* — four rules.

## Verification

| Command | Result |
| --- | --- |
| `scripts\build.ps1` | built Release, 0 warnings (`/W4 /WX`) |
| `ctest --test-dir build --output-on-failure --timeout 300 -R avloop` | **23 passed**, 0 failed, 120 s |
| `scripts\format.ps1 -Check` | PASS: 217 files formatted (after one line of `test-keyframes.cpp` formatted) |
| `node tools/icons/check-icons.mjs` | PASS: 43 icons, avloop.ico with 7 sizes |
| `node tools/check-secrets.mjs` | PASS: 588 files |
| `node control/phase-tools/check-loc.mjs` | PASS: 271 code files (the largest `src/ui/MainWindowProject.cpp`, 399 lines) |
| `node control/phase-tools/validate-phase-system.mjs` | PASS: 18 phases, 23 requirement ids |
| README pictures | not retaken — no screen changed its look |

## The walk

```
node control/lab/walk-phase.mjs execution/AV-018-walk walk-wheel.json walk-key-copy.json → PHASE WALKS PASSED
walk-wheel.json (6 s, 24 PASS): Fit, 100 % at pan (320, 180); the wheel at scene (100, 50) → 150 % pan (246.8, 136.7),
  → 200 % pan (210, 115) — the point stays under the pointer; −60 nothing, −60 again → 150 %; Ctrl+wheel → 100 % pan
  (320.3, 180); Shift+wheel → pan X 380.3, a sideways wheel → 320.3; undo.count 0; the owner's focus untouched.
walk-key-copy.json (6 s, 20 PASS): K at 2 s, X 520 typed at 4 s → "2.000:320.0,180.0,1.000|4.000:520.0,180.0,1.000";
  Alt+drag 2 s → 3 s: 3 keyframes, 3.000 = 320.0,180.0,1.000, the playhead at 3, undo.index 2 → 3; Ctrl+Z 2, Ctrl+Y 3;
  Alt+click: still 3, undo.index 3; a plain drag 3 s → 4.5 s across 4 s: "|4.000:520.0,…|4.500:320.0,…", no 3.000.
```

## The pictures

| Picture | What | What it shows — and what was seen in it |
| --- | --- | --- |
| `AV-018-walk/wheel-100.png` | the canvas at 100 % | the 640×360 counter video centred, the Selection's border and handles, Navy around |
| `AV-018-walk/wheel-200.png` | after two notches at scene (100, 50) | 200 %: the video's top-left corner moved up-left, the timecode box (scene ≈ 100, 50) at the same place in the picture as at 100 % (≈ 434, 195 of 1200) |
| `AV-018-walk/key-copied.png` | the timeline after the Alt+drag | diamonds at 2 s (hollow), 3 s (solid, under the playhead) and 4 s (hollow) |
| `AV-018-walk/key-moved-across.png` | after the plain drag across 4 s | diamonds at 2 s and 4 s hollow, 4.5 s solid under the playhead; nothing at 3 s |

## The regressions

| Walk of before | Result | Mended |
| --- | --- | --- |
| `AV-003-walk/walk-selection.json` (Ctrl+wheel 1 → 1.5, the Hand, the middle-drag) | PASSED (11 s) | — |
| `AV-011-walk/walk-keyframes.json` (the diamond dragged 4 → 4.5 s, Delete, Undo) | PASSED (8 s) | — |

Run from a copy in the session's temp so the old folders keep their pictures; logs in `AV-018-walk/regressions/`.

## External freshness

- Not applicable — no dependency changed.

## Files and line counts

| File | Before | After |
| --- | --- | --- |
| `src/ui/canvas/Canvas.cpp` / `.h` | 138 / 88 | **142 / 89** |
| `src/ui/timeline/TimelineInput.cpp` / `Timeline.h` | 196 / 126 | **207 / 127** |
| `src/core/Keyframes.cpp` / `.h` | 89 / 46 | **96 / 48** |
| `src/core/Project.cpp` / `.h` | 294 / 149 | **300 / 150** |
| `tests/test-keyframes.cpp` | 186 | **207** |
| `control/lab/av-lab.mjs` / `walk-phase.mjs` | 141 / 59 | **142 / 60** |
| `execution/AV-018-walk/walk-wheel.json` / `walk-key-copy.json` | — | **47 / 44** |

## Recovery log

Attempt 1 RESOLVED — gate: build; cause: LNK1104, `build\bin\avloop.exe` held by the owner's open AvLoop (an unsaved
project); repair: the exe renamed aside (`avloop.running-23088.exe`, git-ignored, removed once the owner closes it),
relinked; result: built. Attempt 1 RESOLVED — walk-key-copy: the canvas drag left X at 519.9; the setup types X 520
instead. Attempt 1 RESOLVED — format: one line of the test.

## Deviations from the phase contract

The walk tool's leftover-process check was narrowed to the walk's own processes (the owner's app open on this PC).

## Blocked legs (if any)

None.

## Staged paths

The paths above, `docs/SPEC.md`, `docs/lore/avloop.md`, `README.md`, the programme's files.

## Next

None — every phase closed again. No Release (R-65); CI watched after the push.
