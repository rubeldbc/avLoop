# AV-019 — Execution Record

**Phase:** AV-019 — The clip effects (owner item 17, late)
**Status:** DONE
**Date:** 2026-10-27
**Phase start commit:** `22bd046` (*[AV-018] The wheel and the keyframe copy: wheel zoom about the pointer, Alt+drag
copies*)
**ADR:** none owed — AVADR-0001 kept (the effects are made before either back end; one FrameRenderer still)
**Environment class:** local-disposable (this PC: `build/`, the samples, the walk's folder, the session's temp)

## The owner's words

*"add a righ side group, Clip effects. where a lots of clip effect could be applied to the clip to change or color
correction the clip before export"*

## What was built

1. `src/core/ClipEffects.{h,cpp}` — the eighteen values and On, `clamped`, `isNeutral`, `key`; `effects::Colour` (the
   per-pixel maths of `control/20` §1, its constants once), posterize, the vignette; the sRGB curves by 4097-point
   tables; the file's keys (R-66).
2. `src/render/EffectsPass.{h,cpp}` — blur (three box passes each way, edges held) and sharpen (unsharp, σ 1 source
   px) premultiplied, then each pixel straight; rows in parallel, the scratch kept; neutral returns the frame itself.
3. `src/render/FrameRenderer.*`, `RenderJob.*`, `CpuRenderer.h`, `GlRenderer.cpp` — the pass on every source frame
   (Motion's too) at the working size; `Composite::look` keys the GPU's texture pool; Opacity < 100 % keeps alpha.
4. `src/core/Project.*`, `Commands.h`, `ProjectJson.*`, `ProjectMigrate.cpp` — `effects`, `changeEffects` (one Undo
   step, merging a field), `Aspect::Effects`, a new video without; the file's version 2 and its migration (R-68).
5. `src/render/Shaders.cpp`, `src/ui/canvas/{Canvas.h,CanvasGl.cpp,CanvasGlState.h}` — the canvas's shader: a 64³
   colour table of `effects::Colour`, posterize, vignette, opacity, 7 × 7-tap Gaussians; the QPainter fallback by
   the pass (R-67).
6. `src/ui/panels/EffectsPanel.cpp`, `SidePanel.*`, `MainWindowCanvas.cpp`, `resources/themes/app.qss` — the Clip
   effects section, its group names (`subhead`), the probes (R-70).
7. `src/ui/MainWindow.cpp` — an open Preview deleted first in the destructor (the crash at quit, below).
8. Tests: `tests/test-effects.cpp` (new: 9 cases), `test-renderer.cpp` (the effects project; two looks through one
   context), `test-project-json.cpp` (version 2, a version-1 file without effects).
9. Walks: `AV-019-walk/walk-effects.json`, `walk-effects-render.mjs`; `AV-010-walk/walk-recover.mjs` mended (the
   title of AV-017). `tools/walks/readme-shots.json`: two pictures of the effects.
10. The programme: item 17 (`README.md` §1–§4), `control/20_SHAPES_THE_CLIP_EFFECTS.md`, this phase, `OWNER_ITEMS`
    17, REQ-AV-024, D-017, R-66 … R-70, owner actions 37–38, risk 18.

## Decisions recorded

- **D-017** — the clip effects; spec §0.
- **R-66** which effects; **R-67** made on the CPU for the output, a close GPU look on the canvas; **R-68** the file's
  version 2; **R-69** Opacity and Alpha, the thumbnails and the Finder as the source is; **R-70** the section's place,
  no Release.

## Specification delta

- §0: `D-017`. The lore: *The clip effects (late)* — five rules.

## Verification

| Command | Result |
| --- | --- |
| `scripts\build.ps1` | built Release, 0 warnings (`/W4 /WX`) |
| `ctest --test-dir build --output-on-failure --timeout 300 -R avloop` | **24 passed** (23 + effects), 0 failed, 114 s |
| `scripts\format.ps1 -Check` | PASS: 223 files formatted (after formatting the new files) |
| `node tools/icons/check-icons.mjs` · `node tools/check-secrets.mjs` | PASS · PASS |
| `node control/phase-tools/check-loc.mjs` | PASS (the largest `src/ui/MainWindowProject.cpp`, 399 lines) |
| `node control/phase-tools/validate-phase-system.mjs` | PASS: 19 phases, 24 requirement ids |
| README pictures | `avloop.exe --walk tools/walks/readme-shots.json`: 9 taken (2 new), 14–293 KB |
| AV-018's CI run | green, 8 m 14 s |

## The walk

```
node control/lab/walk-phase.mjs execution/AV-019-walk walk-effects.json walk-effects-render.mjs → PHASE WALKS PASSED
walk-effects.json (8 s, 21 PASS): none set; Exposure 0.6, Contrast 20, Temperature 40, Vibrance 50, Vignette 60,
  Sharpen 30, Hue -150 typed — seven Undo steps; Invert, Posterize 4, Blur 8, Opacity 60; Ctrl+Z takes Opacity back;
  On off (the values kept) and on; F5: the Preview 640x360 with the look; Reset → none; Ctrl+Z → the look again.
walk-effects-render.mjs (5 s, 9 PASS): a version-1 file: 921600 bytes, red 253,0,0,255; Invert 2,255,255,255;
  Saturation 0 → 54,54,54; Opacity 50 % → 127,0,0,128; Off with values: max diff 0 from none; Blur 20 px at x 100:
  253,0,0 → 194,59,0; a whole look: GPU and CPU max diff 0, 247 from none.
```

## The pictures

| Picture | What | What it shows — and what was seen in it |
| --- | --- | --- |
| `canvas-none.png` / `canvas-off.png` | the counter sample, none / On off | the six bars as decoded; Off identical to none |
| `canvas-look.png` | the typed look | brighter, warmer bars (red-orange), stronger colour, corners darkened by the vignette, the stripe crisper |
| `canvas-creative.png` | + Invert, Posterize 4, Blur 8, Opacity 60 % | cyan, magenta, blue, yellow, green, red bars in four flat levels, soft edges, the checkerboard through them |
| `panel-effects.png`, `panel-effects-creative.png` | the section | On and Reset; Light, Colour, Creative, Detail; every value fits its field ("-150°", "8 px", "60 %"), set ones highlighted |
| `preview-effects.png` | the Preview, Live | the same inverted, posterized, blurred look over the checker, 640×360 · 285 frames |
| `render-invert.png`, `render-look.png` | avloop-cli frames | the bars inverted; the hue-turned, warmer look with its vignette |
| `docs/screenshots/clip-effects*.png` | the README | the demo clip warmer under an ellipse crop with a vignette; the section beside it |

## The regressions

| Walk of before | Result | Mended |
| --- | --- | --- |
| AV-001 `walk-shell.json`, AV-003 `walk-selection.json`, AV-008 `walk-preview.json`, AV-010 `walk-settings.json`, AV-013 `walk-batch.json` | PASSED (8, 11, 17, 10, 37 s) | — |
| AV-007 `walk-render.mjs`, AV-010 `walk-project.mjs`, AV-011 `walk-keyframes.mjs`, AV-013 `walk-formats.mjs` | PASSED (23, 16, 5, 12 s) | — |
| AV-010 `walk-recover.mjs` | FAILED (the title "AvLoop — …", AV-017's rename unread), then PASSED (12 s) | the expected title |

The JSON walks ran from copies in the session's temp; the Node walks in place, their folders' logs then put back;
every log in `AV-019-walk/regressions/`.

## External freshness

- Not applicable — no dependency changed.

## Files and line counts

| File | Before | After |
| --- | --- | --- |
| `src/core/ClipEffects.cpp` / `.h` | — | **241 / 88** |
| `src/render/EffectsPass.cpp` / `.h` | — | **164 / 36** |
| `src/ui/panels/EffectsPanel.cpp` | — | **165** |
| `src/ui/canvas/CanvasGl.cpp` / `Canvas.h` | 180 / 89 | **265 / 95** |
| `src/render/Shaders.cpp` | 85 | **139** |
| `src/core/Project.cpp` / `.h` | 300 / 150 | **319 / 155** |
| `src/render/FrameRenderer.cpp` · `GlRenderer.cpp` · `RenderJob.cpp` | 157 · 314 · 81 | **160 · 315 · 84** |
| `tests/test-effects.cpp` · `test-renderer.cpp` | — · 113 | **275 · 145** |
| `execution/AV-019-walk/walk-effects.json` / `walk-effects-render.mjs` | — | **59 / 81** |

## Recovery log

Attempt 1 RESOLVED — walk-effects: Invert's click missed (the checkbox stretched across its column); repair: it is as
wide as its text; result: PASS. Attempt 1 RESOLVED — walk-effects: a lab folder left with a minidump; cause:
`~QWidget` deleted the open Preview after `renderService_` was gone (`kn`: `deleteChildren → ~PreviewWindow →
RenderService::trim → invokeMethod`); repair: `~MainWindow` deletes it first; result: PASS, no folder left. Attempt 1
RESOLVED — the section's picture: "0.6 EV", "0.0 px" cut; repair: Exposure's unit in its tooltip, Blur in whole px,
Hue's "°" without a space. Attempt 1 RESOLVED — format: the new files formatted.

## Deviations from the phase contract

Two mends outside the effects: the crash at quit with the Preview open (found by this walk), AV-010's recovery walk.

## Blocked legs (if any)

None.

## Staged paths

The paths above, `docs/SPEC.md`, `docs/lore/avloop.md`, `README.md`, `docs/screenshots/`, the programme's files.

## Next

None — every phase closed again. No Release (R-70); CI watched after the push.
