# AV-029 — Execution Record

**Phase:** AV-029 — The align tools and the shape layers (owner item 26, late)
**Status:** DONE
**Date:** 2026-10-30
**Phase start commit:** `641e113` (*[AV-028] The resets: a label's click, a group's right-click Reset*)
**ADR:** none owed
**Environment class:** local-disposable (this PC: `build-lab/`, the samples, the walk's folder; the owner's AvLoop open
from `build\bin`, untouched)

## The owner's words

*"[2] There will be align tool set for multiple object and make a layer system for multiple objects"*

## What was built

1. `src/core/ShapeAlign.{h,cpp}` (new, pure) — the selection's box, the six aligns (to it or a frame), the two
   distributes, a layer moved, a shape duplicated.
2. `src/ui/options/AlignBar.cpp` (new) — Align to, the six align and two distribute buttons on the S bar, enabled by
   the selection's size; `OptionsBar.h`, `ShapeOptions.cpp` place them.
3. `src/ui/panels/ShapesPanel.cpp` — the layers: each row's check box (shown/hidden), the lock icon, drag to reorder;
   Name, Visible, Locked, Up, Down, Duplicate; probe `shapes.flags`.
4. Icons `align-left`, `align-centre`, `align-right`, `align-top`, `align-middle`, `align-bottom`, `distribute-h`,
   `distribute-v` (54 … 61); `test-icons` 61; AV-001's shell walk 61.
5. `tests/test-shapes.cpp` — `alignAndLayers` (the six by their edges, never resized, one to the frame, a locked one
   stays, distribute's equal gaps, two do nothing, a move, a copy).
6. Walks: `AV-029-walk/walk-align.json`, `walk-align-render.mjs`.

## Decisions recorded

- **D-026** — the align tools and the shape layers; spec §0.
- **R-94** the objects are the shapes; **R-95** a flat layers panel; **R-96** buttons, icons 54 … 61.

## Specification delta

- §0: `D-026`. The lore: *The align tools and the shape layers (late)* — two rules.

## Verification

| Command | Result |
| --- | --- |
| `scripts\build.ps1 -BuildDir build-lab` | built Release, 0 warnings, first time |
| `ctest --test-dir build-lab --output-on-failure --timeout 300 -R avloop` | first run 24 of 25 (`avloop-memory` failed with no output); alone PASSED (47 s); the whole again **25 passed**, 0 failed, 75 s |
| `scripts\format.ps1 -Check` · `check-icons` · `check-secrets` | PASS: 245 files formatted · PASS: 61 icons, 61 listed, avloop.ico with 7 sizes · PASS: 853 files, nothing secret or personal |
| `node control/phase-tools/check-loc.mjs` | PASS: 309 code files, none over 600 lines (the largest: src/core/ClipEffects.cpp, 425 lines) |
| `node control/phase-tools/validate-phase-system.mjs` | PASS: 31 phases, 34 requirement ids |
| README pictures | not retaken: the default window unchanged (the align tools show with the S tool) |
| AV-028's CI run | green, 9 m 42 s (run 40928874326) |

## The walk

```
AVLOOP_BIN_DIR=build-lab/bin node control/lab/walk-phase.mjs execution/AV-029-walk walk-align.json walk-align-render.mjs
  → PHASE WALKS PASSED
walk-align.json (9 s, 25 PASS): three rectangles; Ctrl+A, Align left → rect:add:21,20,60,60;21,100,60,80;21,40,80,80
  (sizes kept); Ctrl+Z; Distribute horizontally → the middle at 221 (gaps 140 and 140); Ctrl+Z; Esc, a click on the
  middle one alone → "1"; To Frame, centre and middle → 290,140 (the video's centre); Name "Window"; Duplicate →
  4, "Window copy" selected "2"; Up → "3"; Visible off → "vu,vu,vu,hu"; a click on the first → "0"; Locked →
  "vl,…"; Esc, a click on it → nothing selected; Ctrl+Z ×5 back to three, the old name.
walk-align-render.mjs (3 s, 5 PASS): a Cut on top clears the overlap (0, A alone 255); swapped below a Select: 255; the
  Cut hidden byte for byte the Select alone; a locked one renders as any; GPU vs CPU max diff 0.
```

## The pictures

| Picture | What | What it shows — and what was seen in it |
| --- | --- | --- |
| `canvas-before.png` / `canvas-distributed.png` | three rectangles; distributed | three boxes at different heights; the middle one moved so the two gaps match |
| `canvas-layers.png` | after rename, copy, hide, lock | the hidden copy not drawn (no outline, dim over it); the others bright |
| `panel-layers.png` | the Crop section | rows "Window copy" (box unchecked), "Rectangle 3", "Window", "Rectangle 1" with a lock icon; Name, Visible, Locked, Up, Down, Duplicate; Role, Feather, Strength, Delete, Combine |

## The regressions

| Walk of before | Result | Mended |
| --- | --- | --- |
| AV-001 `walk-shell.json` | PASSED (8 s) | 61 icons written first |
| AV-024 `walk-shapes.json`, AV-026 `walk-combine.json`, AV-027 `walk-soft.json` | PASSED (12, 11, 8 s) | — |

Logs in `AV-029-walk/regressions/`.

## External freshness

- Not applicable — no dependency changed.

## Files and line counts

| File | Before | After |
| --- | --- | --- |
| `src/core/ShapeAlign.cpp` / `.h` | — | **110 / 38** |
| `src/ui/options/AlignBar.cpp` | — | **75** |
| `src/ui/panels/ShapesPanel.cpp` | 198 | **314** |
| `tests/test-shapes.cpp` | 170 | **209** |

## Recovery log

Attempt 1 RESOLVED — walk-align: a click on one of three selected shapes kept all three (by design: several are dragged
together); the walk presses Esc first. Attempt 1 RESOLVED — ctest: `avloop-memory` failed once after five GUI walks
with no output; passed alone and in the next full run (nothing of this phase touches media or memory; the lore's rule).

## Deviations from the phase contract

- The rows' names are edited in the Name field, not in the row (the row shows "name · role · soft", which the earlier
  walks read); a drag reorders, and Up/Down do the same for walks.

## Blocked legs (if any)

None.

## Staged paths

The paths above, `docs/SPEC.md`, `docs/lore/avloop.md`, `README.md`, the programme's files.

## Next

AV-030 — the timeline layers I.
