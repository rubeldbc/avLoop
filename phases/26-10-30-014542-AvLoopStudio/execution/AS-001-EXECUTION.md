# AS-001 — Execution Record

**Phase:** AS-001 — The sequence (owner item 1)
**Status:** DONE
**Date:** 2026-10-30
**Phase start commit:** 0becf4e (the plan `[AS] Plan` 9306c61, then AvLoop's `[AV-032]` redaction)
**ADR:** none owed — a model beside AvLoop's, no renderer or export change (AVADR-0001's parity untouched)
**Environment class:** local-disposable (this PC: the build in `build/`, the samples in `samples/`, the walk's folder)

## The owner's words

*"Make this application like mini adobe premiere pro … My final goal that I have to flexible edit and cutting like
hole a video and export as transparent."*

## What was built

1. `src/core/Sequence.{h,cpp}` — the model: `Clip` (media, start, In/Out, transform, keys, opacity, ClipEffects, masks,
   on/off; its own time at a sequence time, its keyed look and rectangle), `Track`, `Sequence` (frame, fps, In/Out,
   tracks ≤ 8, clips ≤ 64); their JSON; `Sequence::fromAvLoop` (AvLoop's project as a sequence, RS-9).
2. `src/core/SequenceEdit.{h,cpp}` — the edits, pure, refused whole: split / splitAll (the Razor), trimIn, trimOut,
   move, remove, rippleRemove, insert, overwrite, addTrack, removeTrack; find, clip, fits, clipAt, topClipAt.
3. `src/core/VideoLayer.{h,cpp}` — `videolayers::keyedLook` (D-029's keyed look over any key list): the layer and the
   clip share it.
4. `src/core/ProjectJson.*`, `ProjectMigrate.cpp` — version 4: `"sequence"` written beside AvLoop's keys (only when the
   source was probed: its size and length are not in the file); 3 → 4 a version bump (the sequence made from the rest).
5. `src/core/Project.*` — `Project::sequence()`, the project as a sequence (made from its fields until AS-003).
6. `src/cli/CliSequence.cpp`, `main.cpp` — `avloop-cli sequence <project> [--save x.avloop]`.
7. `tests/test-sequence.cpp` — 7 cases: own time and look; Razor and trims; moves, removes, ripple; insert and
   overwrite; tracks and hits; the file's form; AvLoop's project migrated (crop, Cut shape, inverse, keys, a layer) and
   `Project::sequence()`. `tests/test-project-json.cpp` — the version expected 4.

## Decisions recorded

- **D-031** — the sequence, its edits, the file's version 4; spec §0.
- **RS-9** — until AS-003 the sequence is made from AvLoop's model and written only for a probed source;
  `changeSequence` comes with the editor that edits it; the JSON in `Sequence.cpp` (no separate `SequenceJson`).

## Specification delta

- §0: `D-031` — the sequence (AS-001).
- README: `avloop-cli sequence` among the CLI's commands. The lore: "AvLoop Studio: the sequence" — a section derived at
  save time depends only on what the file keeps; a Qt test prints nothing to the Bash tool's console here.

## Verification

| Command | Result |
| --- | --- |
| `scripts\build.ps1` | Built Release, 0 warnings (`/W4 /WX`), 66 targets |
| `ctest --test-dir build --output-on-failure --timeout 300 -R avloop` | **26 passed**, 0 failed, 74.6 s |
| `test-sequence.exe -o <file>,txt` | 9 passed (7 cases + init/cleanup), 1 ms |
| `scripts\format.ps1 -Check`; `node tools/icons/check-icons.mjs` | PASS 259 files; PASS 61 icons |
| `node tools/check-secrets.mjs` | PASS (run on its own, exit code 0) |
| `check-loc.mjs` | PASS: 337 code files, the largest `src/core/ClipEffects.cpp` 425 |
| `validate-phase-system.mjs` | PASS: 9 phases, 9 requirement ids, 0 errors, 0 warnings |
| `git push origin main`; CI | pushed `d71708b`; CI green (run 40945823147, 9 m 34 s) |

## The walk

```
node control/lab/walk-phase.mjs execution/AS-001-walk walk-sequence.mjs → PASSED (5 s), 33 checks
  avloop-cli sequence on version-3 files written in a temp folder over samples/counter-640p30.mp4 (640×360, 10 s):
  plain     640×360 at 30/1, one track; V1 the whole video at 320,180, no mask; In/Out 0–2
  crop      a feathered ellipse (100,50 200×100, 10) with a Cut square: frame 200×100, the video at 220,130;
            masks: cut rect 120,60 | keep ellipse 100,50 feather 10
  hole      the same ellipse inverted: frame 640×360, one mask — cut ellipse (the hole)
  layers    a main keyframe (Ease) and a layer on track 3 with keys: tracks V1:1 V2:0 V3:1; the layer's start 0.5,
            In/Out 1–3, x 160, 80 %, keys as written; the main key [1,400,200,1.5,100,1]
  version1  a version-1 file with a rounded crop: read, 320×180, the crop a rounded mask (radius 20)
  each: --save → version 4 with the sequence printed; every key the file set kept (none renamed); read again — the
  same sequence; saved again — the same bytes
  crop, hole, layers: frame 30 of the version-4 file byte for byte the version-3 file's (80000, 921600, 2073600 bytes)
No avloop process, session cache, lab folder or temp file left (walk-phase.mjs).
```

## The pictures

None — AS-001 changes no UI (the sequence is beside AvLoop's model, RS-7); the walk reads the CLI and the frames' bytes.

## The regressions

| Walk of before | Result | Mended |
| --- | --- | --- |
| AV-010 `walk-project.mjs` (save, open, keys kept) | PASSED (18 s): p2 keeps every key, `sequence` among them | none |
| AV-010 `walk-recover.mjs` (autosave, recovery) | PASSED (13 s) | none |
| AV-011 `walk-keyframes.mjs` | PASSED (4 s) | none |
| AV-033 `walk-layer-keys-render.mjs` (keyedLook moved) | PASSED (6 s) | none |

Their logs: `AS-001-walk/regressions/`; the earlier walk folders put back (`git checkout`).

## External freshness

- Not applicable — no dependency changed.

## Files and line counts

| File | Before | After |
| --- | --- | --- |
| `src/core/Sequence.h` / `.cpp` | — | **82** / **242** |
| `src/core/SequenceEdit.h` / `.cpp` | — | **52** / **182** |
| `src/cli/CliSequence.cpp` | — | **62** |
| `tests/test-sequence.cpp` | — | **187** |
| `src/core/Project.h` / `.cpp` | 190 / 375 | **194** / **380** |
| `src/core/ProjectJson.cpp`; `ProjectMigrate.cpp` | 212; 52 | **218**; **57** |
| `src/core/VideoLayer.h` / `.cpp` | 84 / 149 | **88** / **154** |
| `src/cli/main.cpp` | 88 | **92** |

## Recovery log

- Attempt 1 RESOLVED — gate: `test-project-json` saveOpenSaveIsIdentical; cause: the derived `"sequence"` used the
  source's size and length, not in the file, so the second save differed; repair: written only for a probed source,
  else the one read stays; result: 26 of 26.
- Attempt 2 RESOLVED — docs written through a Bash double-quoted `node -e` lost their backticks; reverted
  (`git checkout`) and written again from a script file (the lore's rule).

## Deviations from the phase contract

- `core/SequenceJson` is not a file of its own: the JSON lives in `Sequence.cpp` (242 lines) — RS-9.
- `Project::changeSequence` is not built: until AS-003 nothing edits the sequence but AvLoop's own panels (RS-7, RS-9).

## Staged paths

- `src/core/Sequence.h`, `src/core/Sequence.cpp`, `src/core/SequenceEdit.h`, `src/core/SequenceEdit.cpp`,
  `src/core/VideoLayer.h`, `src/core/VideoLayer.cpp`, `src/core/ProjectJson.h`, `src/core/ProjectJson.cpp`,
  `src/core/ProjectMigrate.cpp`, `src/core/Project.h`, `src/core/Project.cpp`, `src/cli/CliSequence.cpp`,
  `src/cli/main.cpp`, `src/CMakeLists.txt`, `tests/test-sequence.cpp`, `tests/test-project-json.cpp`,
  `tests/CMakeLists.txt`, `docs/SPEC.md`, `docs/lore/avloop.md`, `README.md`, the programme's `READINGS.md`,
  `AUTONOMOUS_STATE.md`, `phases/AS-001-the-sequence.md`, `phases/AS-002-*.md`, `execution/AS-001-EXECUTION.md`,
  `execution/AS-001-walk/`.

## Next

AS-002 — the composite, promoted to IN_PROGRESS in the same turn. No avloop process, session cache or walk temp file
left.
