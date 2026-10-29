# AS-008 — Execution Record

**Phase:** AS-008 — The project (owner item 8)
**Status:** DONE
**Date:** 2026-10-30
**Phase start commit:** 36d8564 (`[AS-007]`)
**ADR:** none owed — the project file's version 5 is AS-003's (D-033); its open and save are AvLoop's
**Environment class:** local-disposable (this PC: the build in `build/`, the samples in `samples/`, the walk's folder)

## The owner's words

*"Make this application like mini adobe premiere pro … do the planning and continue to code."*

## What was built

1. `src/core/ProjectJson.*` — `ProjectData::readVersion`: a file's version before its migration.
2. `src/ui/MainWindowProject.cpp` — an AvLoop project of before opened says so (a toast); its file written only by Save.
3. `src/ui/MainWindowActions.cpp` — a playhead past the opened project's (or video's) length goes back to 0.
4. `src/ui/timeline/SequenceLanes.*` — repainting the playhead's columns only as it moves (the whole on a zoom, a
   scroll or a model change): AS-004's lanes had grown VRAM by 72 MB during playback (AV-005's walk).

## Decisions recorded

- **D-038** — the project; spec §0. No new reading.

## Specification delta

- §0: `D-038`. README: Projects of sequences. The lore: a widget repainted every frame in a window with a
  QOpenGLWidget costs VRAM; a walk that "opens" while the last state is still "ready" waits for nothing.

## Verification

| Command | Result |
| --- | --- |
| `scripts\build.ps1` | Built Release, 0 warnings (`/W4 /WX`) |
| `ctest --test-dir build --output-on-failure --timeout 300 -R avloop` | **27 passed**, 0 failed |
| `scripts\format.ps1 -Check`; `node tools/icons/check-icons.mjs` | PASS 269 files; PASS 61 icons |
| `node tools/check-secrets.mjs` | PASS (on its own, exit code 0) |
| `check-loc.mjs` | PASS: the largest code file `src/core/ClipEffects.cpp`, 425 lines |
| `validate-phase-system.mjs` (both programmes) | PASS: 33 phases; PASS: 9 phases |
| `git push origin main`; CI | see the commit `[AS-008]` and its run |

## The walk

```
node control/lab/walk-phase.mjs execution/AS-008-walk walk-project-prepare.mjs walk-project.json
  walk-project-render.mjs → PASSED
  walk-project-prepare.mjs: avloop-v3.avloop written (version 3: the counter, an ellipse crop with a feather, In 0 →
    Out 4, a layer at half size on V2 from 1 s)
  walk-project.json (1440 × 900): the counter; import the 1080p counter; Insert at 2 s; C, a cut at 16 s; V; the last
    clip selected, Ellipse, Mode Cut → V1:0.00-2.00,2.00-12.00,12.00-16.00,16.00-20.00
    Save As studio.avloop (project.path, not dirty); ☰ → New (one placeholder clip, video none); the project opened →
    the same four clips, two media, the last clip's ellipse:cut, the composite, not dirty; a picture at 17 s
    avloop-v3.avloop opened: the toast "… version 3, opened as a sequence: Save writes version 5."; V1:0.00-10.00
    V2:1.00-4.00; 1 layer; crop ellipse; output 400x280; the playhead back at 0 (it was at 17 s)
  walk-project-render.mjs: studio.avloop version 5, four clips on V1, two media; 13 s frame 90 (the cut clip from its
    3 s), 17 s frame 210 with the hole clear; avloop-v3.avloop still version 3, no sequence; both removed
No avloop process, session cache, lab folder or temp file left (walk-phase.mjs).
```

## The pictures

| Picture | What | What it shows — and what was seen in it |
| --- | --- | --- |
| `AS-008-walk/reopened-17s.png` | the reopened project at 17 s | V1's four bars, the last selected; the monitor the last clip with its hole; the Effect Controls its |
| `AS-008-walk/avloop-v3-opened.png` | the version-3 file opened | the ellipse crop (feather 8, 400 × 280 · alpha), the layer on V2, the toast; first taken with the playhead at 17 s of a 10 s project — mended |

## The regressions

| Walk of before | Result | Mended |
| --- | --- | --- |
| AV-010 project, recover, settings; AV-002 open; AV-005 timeline; AV-011 keyframes; AV-001 shell; AV-008 preview; AV-013 batch restart; AV-012 finder | 11 PASSED | none |
| AV-005 `walk-playback.mjs` | FAILED twice: VRAM +72 MB (162 → 234) — the lanes' full repaint each frame (AS-004); a variant without lanes flat | the playhead's columns repainted only → PASSED, VRAM 162 flat |
| AV-031, AV-030, AV-033 lanes walks; AV-005 timeline; AS-004, AS-003 walks (after the repaint change) | PASSED | none |

Their logs: `AS-008-walk/regressions/`; AvLoop's walk folders put back.

## External freshness

- Not applicable — no dependency changed.

## Files and line counts

| File | Before | After |
| --- | --- | --- |
| `src/ui/timeline/SequenceLanes.cpp` / `.h` | 239 / 79 | **253** / **82** |
| `src/ui/MainWindowProject.cpp`; `MainWindowActions.cpp` | 401; 272 | **408**; **274** |
| `src/core/ProjectJson.cpp` / `.h` | 245 / — | **246** / + `readVersion` |

## Recovery log

- Attempt 1 RESOLVED — the walk's Ctrl+N (New has no shortcut) did nothing, so the reopen was not waited for: ☰ → New
  and `video.state none` before the open.
- Attempt 2 RESOLVED — the picture of the version-3 file showed the playhead at 17 s of 10: reset on open.
- Attempt 3 RESOLVED — AV-005's playback walk: VRAM +72 MB, twice; bisected by a build without the lanes (flat);
  repaint of the playhead's columns only.

## Deviations from the phase contract

- Autosave and recovery are AvLoop's (AV-010's `walk-recover.mjs` PASSED); the snapshot they write holds the sequence.

## Staged paths

- `src/core/ProjectJson.h`, `ProjectJson.cpp`; `src/ui/MainWindowProject.cpp`, `MainWindowActions.cpp`;
  `src/ui/timeline/SequenceLanes.h`, `SequenceLanes.cpp`; `docs/SPEC.md`, `docs/lore/avloop.md`, `README.md`; the
  Studio's `AUTONOMOUS_STATE.md`, `PROGRESS.json`, `control/00_PHASE_INDEX.md`, `phases/AS-008-*.md`,
  `phases/AS-009-*.md`, `execution/AS-003-walk/`, `execution/AS-004-walk/`, `execution/AS-006-EXECUTION.md`,
  `execution/AS-007-EXECUTION.md`, `execution/AS-008-EXECUTION.md`, `execution/AS-008-walk/`.

## Next

AS-009 — CP-S1, the studio walked, promoted to IN_PROGRESS in the same turn. No avloop process, session cache or walk
temp file left.
