# AV-030 — Execution Record

**Phase:** AV-030 — The timeline layers I — the clips and the composite (owner item 27, late; the tracks are AV-031)
**Status:** DONE
**Date:** 2026-10-30
**Phase start commit:** `593ae4c` (*[AV-029] The align tools and the shape layers*)
**ADR:** none owed (AVADR-0001 kept: one CPU pass after either back end; AVADR-0002: at most 8 layers and decoders)
**Environment class:** local-disposable (this PC: `build-lab/`, the samples, the walk's folder; the owner's AvLoop open
from `build\bin`, untouched)

## The owner's words

*"[1] as well as make multiple layer system in the time line like adobe premiere pro"*

## What was built

1. `src/core/VideoLayer.{h,cpp}` (new) — a layer (file, size, length, track, Start, In/Out, transform, Opacity, shown,
   locked), its own time, its file form; the draw order, the hit, the overlap rule.
2. `src/core/Project.{h,cpp}`, `Commands.h` — `layers()`, `changeLayers` (one Undo step), the selected layer,
   `Aspect::Layers`/`LayerSelection`; a new video clears them. `ProjectJson.{h,cpp}` — `"layers"`.
3. `src/render/LayerPass.{h,cpp}` (new) — the layers laid over the main frame inside the mask, the background last;
   one software decoder a layer. `RenderJob` carries the layers; `FrameRenderer` runs the pass after either back end.
4. `src/ui/panels/LayersPanel.cpp` (new), `SidePanel.{h,cpp}` — the Layers section; `src/ui/MainWindowLayers.cpp` (new)
   — Add as layer…, the walk's `addLayer`, the canvas's layer frames and outlines (`Canvas::setUnderlay`);
   `MainMenu.cpp`, `MainWindow.{h,cpp}`, `CanvasOverlay.cpp`.
5. Tests: `test-shapes` (`videoLayers`), `test-project-json` (`layersRoundTrip`).
6. Walks: `AV-030-walk/walk-layers.json`, `walk-layers-render.mjs`.

## Decisions recorded

- **D-027** — the timeline layers (with AV-031); spec §0.
- **R-97** tracks above the main video; **R-98** layers carry transform and opacity only; **R-99** no transitions or
  audio; **R-100** the one-video API kept; **R-101** the file; **R-102** the CPU pass; **R-103** the canvas's look.

## Specification delta

- §0: `D-027` (part I). The lore: *The timeline layers (late)* — three rules.

## Verification

| Command | Result |
| --- | --- |
| `scripts\build.ps1 -BuildDir build-lab` | built Release, 0 warnings |
| `ctest --test-dir build-lab --output-on-failure --timeout 300 -R avloop` | **25 passed**, 0 failed, 76 s |
| `scripts\format.ps1 -Check` · `check-icons` · `check-secrets` | PASS: 251 files formatted (after one test line reflowed) · PASS: 61 icons, 61 listed, avloop.ico with 7 sizes · PASS: 882 files, nothing secret or personal |
| `node control/phase-tools/check-loc.mjs` | PASS: 316 code files, none over 600 lines (the largest: src/core/ClipEffects.cpp, 425 lines) |
| `node control/phase-tools/validate-phase-system.mjs` | PASS: 31 phases, 34 requirement ids |
| README pictures | not retaken: the default window's look unchanged but for the closed-by-default Layers header |
| AV-029's CI run | CI_RESULT |

## The walk

```
AVLOOP_BIN_DIR=build-lab/bin node control/lab/walk-phase.mjs execution/AV-030-walk walk-layers.json
  walk-layers-render.mjs → PHASE WALKS PASSED
walk-layers.json (9 s, 21 PASS): the counter; addLayer the counter → 1 layer "counter-640p30:V2:0.00:320,180,100:100",
  selected, one Undo step; the section's Scale 50, X 160, Y 90, Opacity 60 → "…:160,90,50:60", 5 steps; F5; a second
  addLayer → V3; Delete → 1; Reset Layers → 0; Ctrl+Z → 1, Ctrl+Z → 2; the canvas's decoders ≤ 8.
walk-layers-render.mjs (6 s, 11 PASS): the main video at (100, 125) red 253,0,0; the layer there green 0,254,0; beside
  it 254,0,253 as it was; Opacity 50 % 127,127,0; hidden, starting at 5 s and a file without "layers" byte for byte the
  main video's; GPU vs CPU max diff 0; inside a crop's ellipse its corner clear, the layer kept; exported 60 frames,
  the file's (100, 125) 0,254,0.
```

## The pictures

| Picture | What | What it shows — and what was seen in it |
| --- | --- | --- |
| `canvas-layer.png` | the canvas | the counter, and in its top-left quarter the counter again at half size and 60 % — its bars over the main ones, faded — outlined solid with "counter-640p30" above it |
| `panel-layers.png` | the Layers section | "counter-640p30 · V2" checked; X 160, Y 90, Scale 50 %, Opacity 60 %, Start 0.00 s, Track 2, Locked, Delete |
| `preview-layer.png` | the Preview | the same, exact: the small counter at 60 % over the top-left quarter |
| `render-layer.png` | avloop-cli frame | the layer at full opacity in the top-left quarter |

## The regressions

| Walk of before | Result | Mended |
| --- | --- | --- |
| AV-001 `walk-shell.json`, AV-008 `walk-preview.json`, AV-011 `walk-keyframes.json` | PASSED (8, 17, 9 s) | — |
| AV-007 `walk-render.mjs`, `walk-memory-render.mjs` | PASSED (17, 25 s) | — |
| AV-009 `walk-loops.mjs`, AV-010 `walk-project.mjs` | PASSED (6, 17 s) | — |
| AV-019 `walk-effects-render.mjs`, AV-024 `walk-shapes-render.mjs` | PASSED (4, 5 s) | — |

Logs in `AV-030-walk/regressions/`; the earlier folders put back (AV-008's `preview.avloop`, rewritten by its walk).

## External freshness

- Not applicable — no dependency changed.

## Files and line counts

| File | Before | After |
| --- | --- | --- |
| `src/core/VideoLayer.cpp` / `.h` | — | **85 / 64** |
| `src/render/LayerPass.cpp` / `.h` | — | **122 / 55** |
| `src/ui/panels/LayersPanel.cpp` · `MainWindowLayers.cpp` | — | **164 · 162** |
| `src/render/FrameRenderer.cpp` · `core/Project.cpp` | 161 · 335 | **173 · 362** |

## Recovery log

Attempt 1 RESOLVED — test-shapes: a layer placed by its x as a left edge (it is the centre). Attempt 1 RESOLVED —
walk-layers-render: the bar under x 100 is red, not green (the layer's green there) — expectations by the base frame.

## Deviations from the phase contract

- R-98: the project's video stays the main clip with its keyframes, effects and shapes; a layer has its transform
  and opacity — no `core/Sequence` of equal clips, no `ProjectClips.cpp`; the selected clip does not re-bind the panels.
  The Layers section edits a layer's place, scale, opacity, start and track. The canvas shows layer frames at rest.

## Blocked legs (if any)

None.

## Staged paths

The paths above, `docs/SPEC.md`, `docs/lore/avloop.md`, `README.md`, the programme's files.

## Next

AV-031 — the timeline layers II: the tracks.
