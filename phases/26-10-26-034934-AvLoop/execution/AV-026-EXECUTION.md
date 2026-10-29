# AV-026 — Execution Record

**Phase:** AV-026 — The shape combinations (owner item 23, late)
**Status:** DONE
**Date:** 2026-10-29
**Phase start commit:** `46af75f` (*[AV-025] The pen tool: Illustrator's Pen for custom shapes*)
**ADR:** none owed
**Environment class:** local-disposable (this PC: `build-lab/`, the samples, the walk's folder; the owner's AvLoop open
from `build\bin`, untouched)

## The owner's words

*"[4] Similar to Adobe Illustrator, it should be possible to create new shapes by combining or permuting multiple
existing shapes; these new shapes can then be used for selection."*

## What was built

1. `src/core/ShapeCombine.{h,cpp}` (new, pure) — the four path booleans in order, the selected shapes replaced by one
   Path at the lowest one's place; all-corner outlines simplified to ½ px (R-89).
2. `src/core/MaskShape.{h,cpp}` — several outlines a shape (`PathPoint::start`, the file's eighth value), `outlines`,
   `nextAnchor`/`prevAnchor`, `anchorsOf(QPainterPath)`; odd-even with several outlines. `PathEdit.cpp` — the Pen's
   neighbours within an outline.
3. `src/ui/options/ShapeOptions.cpp`, `OptionsBar.h` — the role lists' four roles with icons; `combineSelected`, the
   Combine ▸ menu on the S bar. `src/ui/panels/ShapesPanel.cpp` — Combine op and button. `src/ui/tools/ShapeTool.cpp` —
   Ctrl+A, the Ctrl+drag marquee.
4. Icons `role-add`, `role-keep`, `role-exclude` (51 … 53); `test-icons` 53; AV-001's shell walk 53.
5. Tests: `test-shapes` (`combine`: the four ops' regions, the role kept, the file, two outlines, a ring's hole, fewer
   than two, a shape between), `test-mask` (`keepAndExclude`, the order).
6. Walks: `AV-026-walk/walk-combine.json`, `walk-combine-render.mjs`.

## Decisions recorded

- **D-023** — the shape combinations; spec §0.
- **R-88** live roles and Combine; **R-89** one Path of several outlines, ½ px lines, icons 51 … 53.

## Specification delta

- §0: `D-023`. The lore: *The shape combinations (late)* — three rules.

## Verification

| Command | Result |
| --- | --- |
| `scripts\build.ps1 -BuildDir build-lab` | built Release, 0 warnings |
| `ctest --test-dir build-lab --output-on-failure --timeout 300 -R avloop` | **25 passed**, 0 failed, 76 s |
| `scripts\format.ps1 -Check` · `check-icons` · `check-secrets` | PASS: 238 files formatted · PASS: 53 icons, 53 listed, avloop.ico with 7 sizes · PASS: 777 files, nothing secret or personal |
| `node control/phase-tools/check-loc.mjs` | PASS: 300 code files, none over 600 lines (the largest: src/core/ClipEffects.cpp, 425 lines) |
| `node control/phase-tools/validate-phase-system.mjs` | PASS: 31 phases, 34 requirement ids |
| README pictures | not retaken: the default window unchanged (the new rows show only with shapes) |
| AV-025's CI run | green, 9 m 32 s (run 40923769770) |

## The walk

```
AVLOOP_BIN_DIR=build-lab/bin node control/lab/walk-phase.mjs execution/AV-026-walk walk-combine.json
  walk-combine-render.mjs → PHASE WALKS PASSED
walk-combine.json (11 s, 29 PASS): two overlapping ellipses with S; Ctrl+A → "0,1"; Combine Unite → one shape
  "Path 1 · Select", path:add:n…; Ctrl+Z → two; Minus front, Intersect, Exclude → one each, each undone; a click on
  the top ellipse → "1"; Role Keep only → ellipse:keep, Exclude → ellipse:exclude; Esc; a Ctrl+drag marquee over the
  first → "0", wider → "0,1"; F5.
walk-combine-render.mjs (3 s, 7 PASS): squares A and B: B Keep only → A 0, both 255, B 0, neither 0; B Exclude →
  255, 0, 255, 0; an Add above a Cut keeps its inside 255; the ring of two outlines: band 255, 255, hole 0, outside 0;
  GPU vs CPU max diff 0.
```

## The pictures

| Picture | What | What it shows — and what was seen in it |
| --- | --- | --- |
| `canvas-unite.png` / `canvas-minus.png` / `canvas-intersect.png` / `canvas-exclude.png` | Combine's four | one path each: the two circles' union; the left circle less the right; the lens; the two lunes with the lens dimmed — its anchors along the outlines (½ px apart from the curve) |
| `canvas-keep.png` | B Keep only, live | only the yellow lens bright, both ellipses outlined, B's eight handles |
| `canvas-exclude-live.png` | B Exclude, live | the two lunes bright, the lens dimmed |
| `panel-combine.png` | the Crop section | "Ellipse 2 · Exclude", "Ellipse 1 · Select"; Role Exclude with its icon; Delete; Combine Exclude and its button |
| `preview-exclude.png` | the Preview | the lunes over the checker, the lens clear |
| `render-ring.png`, `render-exclude.png` | avloop-cli frames | the ring with its square hole; the two side parts of A and B |

## The regressions

| Walk of before | Result | Mended |
| --- | --- | --- |
| AV-001 `walk-shell.json` | PASSED (7 s) | 53 icons written first |
| AV-024 `walk-shapes.json`, `walk-shapes-render.mjs` | PASSED (13, 4 s) | — |
| AV-025 `walk-pen.json`, `walk-pen-render.mjs` | PASSED (11, 1 s) | — |

Logs in `AV-026-walk/regressions/`; AV-024's and AV-025's folders put back as their phases left them.

## External freshness

- Not applicable — no dependency changed.

## Files and line counts

| File | Before | After |
| --- | --- | --- |
| `src/core/ShapeCombine.cpp` / `.h` | — | **91 / 28** |
| `src/core/MaskShape.cpp` | 226 | **290** |
| `src/ui/tools/ShapeTool.cpp` · `options/ShapeOptions.cpp` · `panels/ShapesPanel.cpp` | 340 · 155 · 134 | **367 · 192 · 151** |
| `tests/test-shapes.cpp` · `test-mask.cpp` | 125 · 192 | **170 · 218** |

## Recovery log

Attempt 1 RESOLVED — test-shapes: a ring's file round trip differed (the combined shape kept a `rect` its file form
does not write); `rect` left empty. Attempt 1–2 RESOLVED — the combined shapes showed ~200 anchors (Qt's booleans
flatten curves): simplified to ⅓ px, still crowded, then ½ px (R-89).

## Deviations from the phase contract

- A drag on empty canvas with S still draws; the marquee is a Ctrl+drag (a plain drag's meaning kept).

## Blocked legs (if any)

None.

## Staged paths

The paths above, `docs/SPEC.md`, `docs/lore/avloop.md`, `README.md`, the programme's files.

## Next

AV-027 — the soft parts.
