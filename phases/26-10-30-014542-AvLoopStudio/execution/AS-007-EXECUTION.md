# AS-007 — Execution Record

**Phase:** AS-007 — The export (owner item 7)
**Status:** DONE
**Date:** 2026-10-30
**Phase start commit:** 1a9b572 (`[AS-006]`)
**ADR:** none owed — the export is AvLoop's ExportJob over the composite (D-032); Alpha Auto widened, not changed
**Environment class:** local-disposable (this PC: the build in `build/`, the samples in `samples/`, the walk's folder)

## The owner's words

*"… cutting like hole a video and export as transparent."*

## What was built

1. `src/core/SequenceView.*` — `leavesClear(sequence, frame)`: whether the composite can leave the frame clear between
   In and Out (V1 hidden, a gap on V1, a V1 clip not covering the frame at its place or a key's, a V1 mask, an opacity
   under 100 % — its own, a key's, its clip effects').
2. `src/render/RenderJob.cpp` — Alpha Auto on also when the composite leaves the frame clear;
   `src/core/Project.cpp` — `outputAlpha()` the same (the Output section's "Auto (on)").
3. `tests/test-sequence.cpp` — `leavesClear`: one opaque clip, a masked faded layer over it, a gap, a V1 mask, a key's
   opacity, a key moving the clip off the frame's edge.

## Decisions recorded

- **D-037** — the export; spec §0. **RS-24** — RS-8 refined: the loop mode stays Crossfade for a new project; None
  exports the sequence as cut.

## Specification delta

- §0: `D-037`. README: Export a sequence with transparency.

## Verification

| Command | Result |
| --- | --- |
| `scripts\build.ps1` | Built Release, 0 warnings (`/W4 /WX`) |
| `ctest --test-dir build --output-on-failure --timeout 300 -R avloop` | **27 passed**, 0 failed |
| `test-sequence` (by `spawnSync`) | 13 passed (11 cases + init/cleanup) |
| `scripts\format.ps1 -Check`; `node tools/icons/check-icons.mjs` | PASS 269 files; PASS 61 icons |
| `node tools/check-secrets.mjs` | PASS (on its own, exit code 0) |
| `check-loc.mjs` | PASS: the largest code file `src/core/ClipEffects.cpp`, 425 lines |
| `validate-phase-system.mjs` (both programmes) | PASS: 33 phases; PASS: 9 phases |
| `git push origin main`; CI | pushed `36d8564`; CI green (run 40963434399, 10 m 23 s) |

## The walk

```
node control/lab/walk-phase.mjs execution/AS-007-walk walk-export.mjs → PASSED, 32 checks
  a version-5 project made by hand: V1 the counter 0–1 s, then the counter from its 5 s with an ellipse Cut; V2 the
  1080p counter at 12.5 % in the bottom-right corner; In 0, Out 2, loop None, Alpha Auto, Background #ff00ff
  avloop-cli export --format, each read back (ffprobe; the alpha-keeping decoder):
  webm-vp9  vp9 640×360, 60 frames, no audio; the foot's low stripes 15 | 165 (the cut); alpha 0 in the hole at
            1.5 s, 255 beside it and at 0.5 s; V2 251,0,0 vs V1 252,252,0
  webm-vp8  the same (vp8, 230,718 bytes); apng (rgba, 2,192,165 bytes); mov-prores (yuva444p12le) — the same
  gif       60 frames bgra, 2 passes; the hole clear (1-bit)
  mp4-h264  h264_mf; the hole #ff00ff (253,0,253)
  the VP9 file's frame 45 kept: export-vp9-1.5s.png
No avloop process, session cache, lab folder or temp file left (walk-phase.mjs).
```

## The pictures

| Picture | What | What it shows — and what was seen in it |
| --- | --- | --- |
| `AS-007-walk/export-vp9-1.5s.png` | the VP9 file at 1.5 s | the second clip — "00:00:05.500", 165 —, the ellipse clear (white in the viewer), V2's small counter in the bottom-right corner |

## The regressions

| Walk of before | Result | Mended |
| --- | --- | --- |
| AV-009 `walk-export.mjs`, `walk-loops.mjs`, `walk-export-ui.json`; AV-013 `walk-formats.mjs`, `walk-batch.json`; AV-022 cut render; AV-023 inverse render; AV-007 render; AV-004 crop; AV-019 effects render; AV-030, AV-033 render; AV-012 finder | 13 PASSED | none |

Their logs: `AS-007-walk/regressions/`; AvLoop's walk folders put back.

## External freshness

- Not applicable — no dependency changed.

## Files and line counts

| File | Before | After |
| --- | --- | --- |
| `src/core/SequenceView.cpp` | 167 | **188** |
| `src/render/RenderJob.cpp`; `src/core/Project.cpp` | 96; 317 | **104**; **318** |
| `tests/test-sequence.cpp` | 318 | **344** |

## Recovery log

- Attempt 1 RESOLVED — the walk read the counter's 16 stripes where V2 covers the last five (bits 13, 15 set): it
  reads the stripes left of V2 (the low 11 bits).

## Deviations from the phase contract

- The loop mode None is not the default (RS-24): the Loop section's None exports the sequence as cut.

## Staged paths

- `src/core/SequenceView.h`, `SequenceView.cpp`, `Project.cpp`; `src/render/RenderJob.cpp`; `tests/test-sequence.cpp`;
  `docs/SPEC.md`, `README.md`; the Studio's `READINGS.md`, `AUTONOMOUS_STATE.md`, `PROGRESS.json`,
  `control/00_PHASE_INDEX.md`, `phases/AS-007-*.md`, `phases/AS-008-*.md`, `execution/AS-007-EXECUTION.md`,
  `execution/AS-007-walk/`.

## Next

AS-008 — the project, promoted to IN_PROGRESS in the same turn. No avloop process, session cache or walk temp file
left.
