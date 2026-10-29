# AV-022 — Execution Record

**Phase:** AV-022 — The clip as cut (owner item 20, late)
**Status:** DONE
**Date:** 2026-10-29
**Phase start commit:** `d3bffe2` (*[AV] Plan items 20 … 27: AV-022 … AV-031, late*)
**ADR:** none owed
**Environment class:** local-disposable (this PC: `build-lab/`, the samples, the walk's folder; the owner's own AvLoop
open from `build\bin` with unsaved work — never touched: the phase built into `build-lab` and walked it by
`AVLOOP_BIN_DIR`)

## The owner's words

*"[1] Users should be able to export without applying a loop at the end, or without adding a crossfade in the
middle."* — of the seven of 2026-10-29; then *"write code first then e2e and walk test"* (the order kept: code, the
tests, the walks).

## What was built

1. `src/core/LoopSettings.{h,cpp}` — `Mode::None` (`"none"`), added last so every int the panels and walks choose by
   keeps its meaning.
2. `src/core/LoopMap.cpp` — None's length D and picks {k, k, 0} (Simple's); `LoopPlan` unchanged (no clamp flags).
3. `src/ui/panels/LoopPanel.cpp` — "None (the clip as cut)" first in Mode, its tip; the summary "Clip …".
4. `src/ui/preview/PreviewWindow.cpp`, `PreviewWidgets.{h,cpp}` — no seam mark for None; Around seam off and
   disabled, tip "The clip is not a loop"; `MainWindowPreview.cpp` the probe `preview.aroundSeamEnabled`.
5. `src/export/FormatProfiles.{h,cpp}` — `namePattern`: `_loop_` (or a closing `_loop`) said `_clip_` for None;
   `MainWindowExport.cpp` uses it for Export, the batch and `export.defaultName`.
6. `tests/test-loopmap.cpp` (`none`), `test-project-json.cpp` (`noneRoundTrip`), `test-export-job.cpp` (`names`).
7. Walks: `AV-022-walk/walk-cut.json`, `walk-cut-render.mjs`.

## Decisions recorded

- **D-020** — the clip as cut; spec §0.
- **R-77** Mode None, said "Clip", Simple kept; **R-78** `"none"` under version 2 until AV-023, no Release between.

## Specification delta

- §0: `D-020`. The lore: *The clip as cut (late)* — three rules (the owner's AvLoop in `build\bin` → `build-lab`;
  encoded vs rendered frames compared by nearest; a new enum value last).

## Verification

| Command | Result |
| --- | --- |
| `scripts\build.ps1 -BuildDir build-lab` | built Release, 0 warnings (415 steps, a fresh tree) |
| `ctest --test-dir build-lab --output-on-failure --timeout 300 -R avloop` | **24 passed**, 0 failed, 78 s (test-loopmap `none`, test-project-json `noneRoundTrip`, test-export-job `names`) |
| `scripts\format.ps1 -Check` · `check-icons` · `check-secrets` | PASS 225 files · PASS 44 icons · PASS 680 files |
| `node control/phase-tools/check-loc.mjs` | PASS: 283 code files (the largest `src/core/ClipEffects.cpp`, 425 lines) |
| `node control/phase-tools/validate-phase-system.mjs` | PASS: 31 phases, 34 requirement ids |
| README pictures | not retaken: the default UI (Crossfade) is unchanged; None shows in the walk's pictures |

## The walk

```
AVLOOP_BIN_DIR=build-lab/bin node control/lab/walk-phase.mjs execution/AV-022-walk walk-cut.json walk-cut-render.mjs
  → PHASE WALKS PASSED
walk-cut.json (7 s): the counter, In 1 s, Out 3 s — Crossfade D60 F15 T9 L45, "Loop 1.50 s · 45 frames @ 30 fps",
  …_loop_…; Mode None (data 4): L60, "Clip 2.00 s · 60 frames @ 30 fps", the fade edge at Out (no band), the name
  counter-640p30_clip_640x360.webm; F5: 60 frames cached, Around seam disabled and off; Ctrl+Z → crossfade and the
  loop's words and name; Ctrl+Y → none; Simple → Around seam enabled again. The owner's focus untouched.
walk-cut-render.mjs (7 s, 11 PASS): avloop-cli "frame 0 of 60 · 640x360 · none"; None frames 0, 30, 59 = Simple's
  byte for byte (921,600 bytes each); Crossfade 45 frames, its frame 0 = None's frame 15 (S[F]); export none.webm
  60 frames at 30/1, 220,163 bytes, checked; ffprobe vp9 640x360 30/1; the file's first frame nearest rendered frame
  0 (mean 2.93 against 12.13 for 15 and 9.71 for 30); the file says "mode": "none".
```

## The pictures

| Picture | What | What it shows — and what was seen in it |
| --- | --- | --- |
| `loop-panel-none.png` | the Loop section, None | Mode "None (the clip as cut)", Speed 1.00×, FPS method Nearest — Crossfade, Curve and Turn gone; the summary "Clip 2.00 s · 60 frames @ 30 fps" |
| `timeline-crossfade.png` / `timeline-none.png` | the band row | Crossfade: the orange gradient before Out and the dashed box after In; None: the range's orange brackets only, the band row empty |
| `preview-none.png` | the Preview, None | the counter at 00:00:01.000; "640×360 · 30 fps · 60 frames · 2.00 s · VP9"; the slider with no orange seam ticks at its ends; "Around seam" grey |
| `render-none-first.png` | avloop-cli frame 0 | the counter's frame 30 (1 s), whole 640 × 360, no blend |

## The regressions

| Walk of before | Result | Mended |
| --- | --- | --- |
| AV-006 `walk-loop.json` (the four modes by their ints) | PASSED (11 s) | — |
| AV-008 `walk-preview.json` (seam marks, Around seam) | PASSED (16 s) | — |
| AV-009 `walk-loops.mjs` (every mode exported, seamDiff, S11) | PASSED (6 s) | — |

Logs in `AV-022-walk/regressions/`; AV-009's folder put back as its phase left it.

## External freshness

- Not applicable — no dependency changed.

## Files and line counts

| File | Before | After |
| --- | --- | --- |
| `src/core/LoopMap.cpp` · `LoopSettings.cpp` / `.h` | 119 · 151 / 77 | **121 · 153 / 78** |
| `src/ui/panels/LoopPanel.cpp` | 188 | **193** |
| `src/ui/preview/PreviewWindow.cpp` | 239 | **246** |
| `src/export/FormatProfiles.cpp` | 65 | **73** |

## Recovery log

Attempt 1 RESOLVED — build: LNK1104 on `build\bin\avloop.exe` (the owner's AvLoop open from it, a project unsaved):
built into `build-lab`, walked by `AVLOOP_BIN_DIR`. Attempt 1 RESOLVED — `walk-cut-render.mjs`: the encoded first
frame against the rendered one at a fixed threshold (mean 6.69 at 64 × 36) — the codec's matrix and 4:2:0, not a
wrong frame; the check made relative (nearest of frames 0, 15, 30).

## Deviations from the phase contract

None.

## Blocked legs (if any)

None.

## Staged paths

The paths above, `docs/SPEC.md`, `docs/lore/avloop.md`, `README.md`, the programme's files.

## Next

AV-023 — the inverse selection.
