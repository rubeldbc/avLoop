# AV-025 — Execution Record

**Phase:** AV-025 — The pen tool (owner item 22, late; with AV-024)
**Status:** DONE
**Date:** 2026-10-29
**Phase start commit:** `b825663` (*[AV-024] The shapes: standard and custom shapes that select or cut*)
**ADR:** none owed
**Environment class:** local-disposable (this PC: `build-lab/`, the samples, the walk's folder; the owner's AvLoop open
from `build\bin`, untouched)

## The owner's words

*"[1] there will be cutoma shape creation like illustrator will be available with full pen tool"*

## What was built

1. `src/ui/tools/PathEdit.{h,cpp}` (new, pure) — a segment's point, the segment and handle hit tests, an anchor added
   by de Casteljau (the outline unchanged), removed, converted smooth ↔ corner, a handle moved (the pair turned, Alt
   breaking it), the anchors' kinds, a polygon as a path (R-86).
2. `src/ui/tools/PenTool.cpp` (new) — P: corner clicks, smooth drags, closing on the first anchor or Enter, Esc; on the
   selected path: handles, anchors, Alt+click convert, a segment click adds, Delete; Ctrl+click selects; the anchors,
   handles and the next segment drawn.
3. `src/ui/options/ShapeOptions.cpp`, `OptionsBar.h` — the Pen's row (Role, how it is used). `Tool.h`,
   `MainWindow.cpp`, `MainWindowCanvas.cpp` — P on the rail after S. `resources/icons/tool-pen.svg` (the 50th icon);
   `test-icons` 50; AV-001's shell walk 50 icons, 11 tools.
4. `tests/test-shapes.cpp` — `penGeometry` (the split keeps the curve, a straight side's corner, convert both ways,
   the handle pair and Alt, remove, hits, a polygon as a path).
5. Walks: `AV-025-walk/walk-pen.json`, `walk-pen-render.mjs`.

## Decisions recorded

- **D-022** — the shapes and the Pen; spec §0 (the Pen's paragraph).
- **R-86** the Pen as Illustrator's for closed paths; a polygon edited becomes a path.

## Specification delta

- §0: `D-022`, the Pen. The lore: one rule (a curve's hit test finds the anchor's own end).

## Verification

| Command | Result |
| --- | --- |
| `scripts\build.ps1 -BuildDir build-lab` | built Release, 0 warnings, first time |
| `ctest --test-dir build-lab --output-on-failure --timeout 300 -R avloop` | **25 passed**, 0 failed, 75 s (test-shapes `penGeometry` first time) |
| `scripts\format.ps1 -Check` · `check-icons` · `check-secrets` | PASS: 236 files formatted · PASS: 50 icons, 50 listed, avloop.ico with 7 sizes · PASS: 753 files, nothing secret or personal |
| `node control/phase-tools/check-loc.mjs` | PASS: 297 code files, none over 600 lines (the largest: src/core/ClipEffects.cpp, 425 lines) |
| `node control/phase-tools/validate-phase-system.mjs` | PASS: 31 phases, 34 requirement ids |
| README pictures | `readme-shots.json` by `build-lab`: 11 taken, `main-window.png` 202 KB (P on the rail) |
| AV-024's CI run | CI_RESULT |

## The walk

```
AVLOOP_BIN_DIR=build-lab/bin node control/lab/walk-phase.mjs execution/AV-025-walk walk-pen.json walk-pen-render.mjs
  → PHASE WALKS PASSED
walk-pen.json (11 s, 35 PASS): P; clicks (100, 100), (300, 100), a drag at (300, 300) → (340, 300), a click
  (100, 300) — 4 anchors, no shape yet; a click on the first closes → path:add:n4 "c,c,s,c", one Undo step; Alt+click
  the smooth anchor → "c,c,c,c", again → "c,c,s,c"; a click on the top segment → n5 "c,c,c,s,c", the new anchor
  selected; Delete → n4; Alt+drag of its out handle → "c,c,b,c"; an anchor dragged; Ctrl+click beside → nothing
  selected; a path begun and Esc → none; Role Cut, two clicks and a drag, Return → ;path:cut:n3 "c,c,s"; F5;
  Ctrl+Z → one shape.
walk-pen-render.mjs (1 s, 7 PASS): the curved square 640x360; (200, 70) between chord and curve 255, (200, 30) 0;
  below 255, beside 0, 0; the edge at x 200 crosses ½ at y ≈ 40 (the cubic's 40); the same anchors without handles
  leave (200, 70) clear; GPU vs CPU max diff 1.
```

## The pictures

| Picture | What | What it shows — and what was seen in it |
| --- | --- | --- |
| `canvas-pen-drawn.png` | the path just closed | four anchors over the green and yellow bars, the right-bottom one smooth with its two mirrored handle dots; the bottom side curved |
| `canvas-pen-edited.png` | after convert, add, delete, the Alt drag, a move | the top-left anchor moved up; the bottom-right anchor's two handles now in different directions (the pair broken) and the bottom side swung down; the inside bright, the rest dimmed |
| `preview-pen.png` | the Preview | the green and yellow bars cut out along the curved outline over the checker, "VP9 alpha" |
| `render-pen.png` | avloop-cli frame | the square with its top bulging up to y ≈ 40 |

## The regressions

| Walk of before | Result | Mended |
| --- | --- | --- |
| AV-001 `walk-shell.json` | PASSED (8 s) | 50 icons, 11 tools written first |
| AV-024 `walk-shapes.json`, `walk-shapes-render.mjs` | PASSED (12, 4 s) | — |
| AV-007 `walk-render.mjs` | PASSED (16 s) | — |

Logs in `AV-025-walk/regressions/`; AV-007's and AV-024's folders put back as their phases left them.

## External freshness

- Not applicable — no dependency changed.

## Files and line counts

| File | Before | After |
| --- | --- | --- |
| `src/ui/tools/PenTool.cpp` · `PathEdit.cpp` / `.h` | — | **278 · 143 / 48** |
| `src/ui/options/ShapeOptions.cpp` | 129 | **155** |
| `tests/test-shapes.cpp` | 81 | **125** |

## Recovery log

None — built, tested and walked the first time.

## Deviations from the phase contract

None (the editing of an existing path, the seam AV-025a, was not needed).

## Blocked legs (if any)

None.

## Staged paths

The paths above, `docs/SPEC.md`, `docs/lore/avloop.md`, `README.md`, `docs/screenshots/`, the programme's files.

## Next

AV-026 — the shape combinations.
