# AV-020 — Execution Record

**Phase:** AV-020 — The colour theme (owner item 18, late)
**Status:** DONE
**Date:** 2026-10-27
**Phase start commit:** `3d861cc` (*[AV-019] The clip effects: colour correction and looks before export*)
**ADR:** none owed
**Environment class:** local-disposable (this PC: `build/`, the samples, the walk's folder; the owner's own AvLoop
open from `build\bin` — its locked exe renamed aside, never closed)

## The owner's words

*"need more effect like i want to change the color theme of a clip, say a clip is orange color but i want to change
it to red color theme. this kind of effect needed"*; then *"after completing everything push to main then forcly
shutdown my pc"*.

## What was built

1. `src/core/ClipEffects.{h,cpp}` — Replace colour (From, To, Hue range, Softness, Amount), Colourise, Split toning,
   Duotone: their values, maths (HSV/HSL helpers), file keys (#rrggbb), clamping; `isNeutral` leaves out what an
   effect at 0 % holds (R-71, R-73).
2. `src/ui/widgets/ColourField.{h,cpp}` — hex, swatch (colour dialog), eyedropper; `resources/icons/eyedropper.svg`
   (the 44th icon; `IconFactory`, `resources.qrc`, `test-icons`).
3. `src/ui/canvas/Canvas.{h,cpp}` — `pickColour`: the clip's straight colour under a click, the click not the tool's,
   Esc claimed while picking (R-72).
4. `src/ui/panels/EffectsPanel.cpp`, `SidePanel.h`, `MainWindowCanvas.cpp` — the four groups; a colour chosen at 0 %
   sets 100 %; the eyedropper wired to the canvas; `effects.summary` shows colours.
5. `tests/test-effects.cpp` — `replaceColour` (yellow → red, red and greys kept, orange → deep red, half, softness,
   a hard edge) and `themes` (Colourise's lightness, Split toning, Duotone's ends, a theme at 0 % none, the hex).
6. Walks: `AV-020-walk/walk-theme.json`, `walk-theme-render.mjs`; `AV-001-walk/walk-shell.json` counts 44 icons;
   `tools/walks/readme-shots.json` takes `clip-theme.png`.
7. The programme: item 18 (`README.md` §1–§3), `control/21_SHAPES_THE_COLOUR_THEME.md`, this phase, `OWNER_ITEMS` 18,
   REQ-AV-025, D-018, R-71 … R-73, owner actions 39–40.

## Decisions recorded

- **D-018** — the colour theme; spec §0.
- **R-71** the four effects and their order; **R-72** the eyedropper, colours at 0 %; **R-73** version 2 kept, 44
  icons, no Release, the shutdown after a green CI.

## Specification delta

- §0: `D-018`. The lore: *The colour theme (late)* — four rules.

## Verification

| Command | Result |
| --- | --- |
| `scripts\build.ps1` | built Release, 0 warnings (`/W4 /WX`) |
| `ctest --test-dir build --output-on-failure --timeout 300 -R avloop` | **24 passed**, 0 failed, 110 s (test-effects 13 cases) |
| `scripts\format.ps1 -Check` · `check-icons` · `check-secrets` | PASS 225 files · PASS 44 icons · PASS 626 files |
| `node control/phase-tools/check-loc.mjs` | PASS: 281 code files (the largest `src/core/ClipEffects.cpp`, 425 lines) |
| `node control/phase-tools/validate-phase-system.mjs` | PASS: 20 phases, 25 requirement ids |
| README pictures | `readme-shots.json`: 10 taken (`clip-theme.png` new), 14–292 KB |
| AV-019's CI run | green, 8 m 34 s |

## The walk

```
node control/lab/walk-phase.mjs execution/AV-020-walk walk-theme.json walk-theme-render.mjs → PHASE WALKS PASSED
walk-theme.json (8 s, 22 PASS): the eyedropper on the yellow bar → replace=100 replaceFrom=#fdfd00, one Undo step, the
  Selection and the video untouched; To #ff0000 typed; Esc then a click: nothing picked; Colourise 100 %; Duotone Dark
  typed at 0 % → duotone=100; Split toning 80; Undo; the Preview 640x360.
walk-theme-render.mjs (3 s, 7 PASS): yellow 253,253,0 → 253,0,0; the red and green bars byte for byte; Colourise red:
  the blue bar 223,31,31; Duotone: the black strip 0,0,64; an AV-019 file renders; a whole theme GPU = CPU (max 0).
```

## The pictures

| Picture | What | What it shows — and what was seen in it |
| --- | --- | --- |
| `canvas-replaced.png` | yellow → red | the third bar red like the first; green, blue, magenta, cyan as they were; the stripe's yellow and the timecode's orange turned red too |
| `canvas-colourise.png` | Colourise red 100 % | every bar one red (all at lightness ½), their edges faint; the black strip black |
| `canvas-duotone.png` | Duotone navy → warm | dark bars navy-violet, light ones warm sand; the strip navy |
| `canvas-split.png` | Split toning 80 | the strip navy, the light bars warmer |
| `panel-theme.png` | the section | Replace colour (From #fdfd00 with its swatch and the eyedropper, To #ff0000), Colourise, Split toning, Duotone, each colour a hex and a swatch |
| `preview-replaced.png`, `render-*.png` | the exact output | the yellow bar red in the Preview and in avloop-cli's frame |
| `docs/screenshots/clip-theme.png` | the README | the demo clip's orange made crimson under the ellipse crop, the green corner kept |

## The regressions

| Walk of before | Result | Mended |
| --- | --- | --- |
| AV-001 `walk-shell.json` | FAILED (`icons.count` 44, wanted 43), then PASSED (8 s) | 44 icons |
| AV-003 `walk-selection.json`, AV-011 `walk-keyframes.json`, AV-019 `walk-effects.json`, `walk-effects-render.mjs` | PASSED (11, 8, 8, 5 s) | — |

Logs in `AV-020-walk/regressions/`; AV-019's folder put back as its phase left it.

## External freshness

- Not applicable — no dependency changed.

## Files and line counts

| File | Before | After |
| --- | --- | --- |
| `src/core/ClipEffects.cpp` / `.h` | 241 / 88 | **425 / 109** |
| `src/ui/panels/EffectsPanel.cpp` | 165 | **214** |
| `src/ui/widgets/ColourField.cpp` / `.h` | — | **68 / 42** |
| `src/ui/canvas/Canvas.cpp` / `.h` | 142 / 95 | **192 / 103** |
| `tests/test-effects.cpp` | 275 | **344** |
| `execution/AV-020-walk/walk-theme.json` / `walk-theme-render.mjs` | — | **57 / 71** |

## Recovery log

Attempt 1 RESOLVED — build: LNK1104, the owner's AvLoop open from `build\bin`; its exe renamed aside, relinked.
Attempt 1–2 RESOLVED — test-effects: two expected values of mine wrong (45° turned −60.2° gives blue 0.253; 0.8 green
is 48°, inside the range); the effect right, the expectations mended. Attempt 1 RESOLVED — the shell walk's icon
count. A log line that said AV-019's render walk had passed before it ran was struck, and the walk run.

## Deviations from the phase contract

None.

## Blocked legs (if any)

None.

## Staged paths

The paths above, `docs/SPEC.md`, `docs/lore/avloop.md`, `README.md`, `docs/screenshots/`, the programme's files.

## Next

None — every phase closed again. No Release (R-73). Pushed, CI watched; then the owner's PC shut down at the owner's
word.
