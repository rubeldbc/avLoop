# AV-028 — Execution Record

**Phase:** AV-028 — The resets (owner item 25, late)
**Status:** DONE
**Date:** 2026-10-30
**Phase start commit:** `80bd697` (*[AV-027] The soft parts: Feather and Strength a shape*)
**ADR:** none owed
**Environment class:** local-disposable (this PC: `build-lab/`, the samples, the walk's folder; the owner's AvLoop open
from `build\bin`, untouched)

## The owner's words

*"[6] Clicking on the text label next to any control should reset its value. [7] Each group on the right-hand side
should include a right-click context menu with a 'Reset' option, which will reset the settings for that group."*

## What was built

1. `src/ui/widgets/ResetLabel.{h,cpp}` (new) — `reset::setDefault`/`hasDefault`/`apply` (through each control's own
   signal) and `ResetLabel` (a click resets; the hand cursor and tip where there is a default; named
   `<control>.label`). `ColourField::choose`.
2. `src/ui/panels/Section.{h,cpp}` — rows' labels are ResetLabels; `addResetAction` and the right-click menu.
   `src/ui/options/OptionsBar.{h,cpp}` — the bars' labels likewise; `setReset` and its menu.
3. The defaults and group resets: `CropPanel.cpp`, `ShapesPanel.cpp`, `LoopPanel.cpp`, `OutputPanel.cpp`,
   `QualityPanel.cpp`, `EffectsPanel.cpp`; the bars `CropOptions.cpp`, `ShapeOptions.cpp`, `SelectionOptions.cpp`.
4. `src/ui/WalkSteps.cpp`, `WalkRunner.{h,cpp}` — the walk's `"context"` step.
5. `tests/test-commands.cpp` — `groupResetIsOneStep`.
6. Walk: `AV-028-walk/walk-resets.json`.

## Decisions recorded

- **D-025** — the resets; spec §0.
- **R-92** a label resets its control by its own edit, no default for positions and read-only rows; **R-93** the
  defaults a fresh project's, the groups' resets one step, the bars their tool's fields.

## Specification delta

- §0: `D-025`. The lore: *The resets (late)* — two rules.

## Verification

| Command | Result |
| --- | --- |
| `scripts\build.ps1 -BuildDir build-lab` | built Release, 0 warnings |
| `ctest --test-dir build-lab --output-on-failure --timeout 300 -R avloop` | **25 passed**, 0 failed, 75 s (test-commands `groupResetIsOneStep`) |
| `scripts\format.ps1 -Check` · `check-icons` · `check-secrets` | PASS: 242 files formatted · PASS: 53 icons, 53 listed, avloop.ico with 7 sizes · PASS: 822 files, nothing secret or personal |
| `node control/phase-tools/check-loc.mjs` | PASS: 305 code files, none over 600 lines (the largest: src/core/ClipEffects.cpp, 425 lines) |
| `node control/phase-tools/validate-phase-system.mjs` | PASS: 31 phases, 34 requirement ids |
| README pictures | not retaken: the window looks as before (the labels' cursor shows on hover) |
| AV-027's CI run | green, 7 m 1 s (run 40926746034) |

## The walk

```
AVLOOP_BIN_DIR=build-lab/bin node control/lab/walk-phase.mjs execution/AV-028-walk walk-resets.json → PHASE WALKS PASSED
walk-resets.json (13 s, 30 PASS): an ellipse crop; Feather 20, Crossfade 1.2 s, Speed 2, Long side 720, CRF 20,
  Exposure 2, Sepia 40 typed (8 Undo steps); the labels clicked — crop.feather 0, loop.speed 1, the Crossfade pair's
  F15 again, effects "sepia=40" (12 steps); Ctrl+Z ×3 gives exposure 2 and speed 2 back, Ctrl+Y ×3; Mode Ping-pong;
  right clicks: Reset Clip effects → none; Reset Quality → CRF 31; Reset Output → crop; Reset Loop → speed 1,
  crossfade; Reset Crop → no crop; Ctrl+Z → the crop, Ctrl+Z → pingpong; the E bar's Feather 33, its label → 0;
  Feather 12, the bar's "Reset Crop: Ellipse" → 0; V, Scale 150, its label → 1.
```

## The pictures

| Picture | What | What it shows — and what was seen in it |
| --- | --- | --- |
| `panel-before-reset.png` | the side panel, changed | the ellipse crop with Feather; Loop Ping-pong at 2×; Output Long side 720; Quality Custom CRF 20 |
| `panel-after-reset.png` | after the five group resets | "No crop: the whole video is exported."; Loop Crossfade 0.50 s / 15 f, Smooth, 1.00×, Nearest; Output WebM VP9, Crop size (640×360), Auto (off), #000000, Source (30); Quality Medium, CRF 31, Speed 2 |

## The regressions

| Walk of before | Result | Mended |
| --- | --- | --- |
| AV-001 `walk-shell.json` | PASSED (7 s) | — |
| AV-003 `walk-selection.json`, AV-004 `walk-crop.json` | PASSED (11, 24 s) | — |
| AV-006 `walk-loop.json`, AV-019 `walk-effects.json` | PASSED (11, 8 s) | — |

Logs in `AV-028-walk/regressions/`.

## External freshness

- Not applicable — no dependency changed.

## Files and line counts

| File | Before | After |
| --- | --- | --- |
| `src/ui/widgets/ResetLabel.cpp` / `.h` | — | **89 / 50** |
| `src/ui/panels/Section.cpp` · `options/OptionsBar.cpp` | 106 · 91 | **125 · 113** |
| `src/ui/WalkSteps.cpp` | 390 | **411** |
| `src/ui/options/ShapeOptions.cpp` · `panels/EffectsPanel.cpp` · `panels/OutputPanel.cpp` | 232 · 214 · 267 | **251 · 222 · 279** |
| `tests/test-commands.cpp` | 186 | **221** |

## Recovery log

Attempt 1 RESOLVED — walk-resets: two expectations of mine (Reset Loop after the labels had reset the loop made no
step; the Scale field is `options.scale`); the walk changed the loop first and named the field right.

## Deviations from the phase contract

- The Settings window, the Preview's lists and the Loop Finder keep plain labels (R-92: not the right-hand side; the
  Settings are saved at once, outside Undo).

## Blocked legs (if any)

None.

## Staged paths

The paths above, `docs/SPEC.md`, `docs/lore/avloop.md`, `README.md`, the programme's files.

## Next

AV-029 — the align tools and the shape layers.
