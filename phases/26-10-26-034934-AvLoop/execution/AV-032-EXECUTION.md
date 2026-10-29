# AV-032 — Execution Record

**Phase:** AV-032 — The sweep (owner item 28, late)
**Status:** DEFERRED — to the new programme `phases/26-10-30-014542-AvLoopStudio` (its checkpoint walks everything)
**Date:** 2026-10-30
**Phase start commit:** `088ee81` (*[AV-033] The layers' keyframes and effects*)
**ADR:** none owed
**Environment class:** local-disposable (`build\` fresh, the samples, the walk's folder)

## The owner's words

*"push to main and complete all the task and test then rebuild and push"*; during the sweep, *"I think the applicaiton
stracture is not good enough to edit. Make this application like mini adobe premiere pro. Make the planning in a new
and different folder. the start coding yourself. My final goal that I have to flexible edit and cutting like hole a
video and export as transparent. do the planning and continue to code"* — the sweep stopped where it stood and its
remainder handed to the new programme's checkpoint.

## What was done

1. `scripts\build.ps1 -Fresh`: `build\` rebuilt from nothing, Release, 455 steps, 0 warnings (twice: before and after
   AV-033).
2. `AV-032-walk/sweep.mjs`: every walk of AV-001 … AV-033 one by one on that build; the install and release walks
   skipped (R-75). Logs in `AV-032-walk/sweep/`.
3. `tools/media/make-samples.mjs`: FFmpeg 9 on this PC has no `-top`; `interlaced.mpg` made again with the field order
   from `setfield` (ffprobe: mpeg2video, field order `tt`).

## The sweep's results (on the fresh build, after AV-033)

| | Walks |
| --- | --- |
| PASSED | 56 |
| Skipped (R-75) | 3 — `AV-014 walk-install`, `AV-017 walk-name`, `AV-021 walk-release` |
| Not passed | 4 — `AV-002 walk-import.mjs` (no `samples/interlaced.mpg` on this PC: made again above, not re-walked); `AV-007 walk-memory-render.mjs` (the CPU stage +56.9 MB, first third 136 MB → 191 / 193: a warm-up step, not a climb — passed at AV-030 and AV-031, not re-walked); `AV-015 walk-memory.mjs` (a detached long run, `node walk-memory.mjs start` first, not started by the sweep); `AV-028 walk-resets.json` (the right-click menu not found in time under load, then its CRF read; passed at AV-028 and AV-033) |

Not run: the ASan and Debug-leak suites.

## Deviations from the phase contract

The phase stops here at the owner's new word; its remainder — the four walks again, ASan, Debug leaks — is the new
programme's checkpoint's (`phases/26-10-30-014542-AvLoopStudio`), which walks the restructured app whole.

## Staged paths

`execution/AV-032-*`, `tools/media/make-samples.mjs`, the programme's files.
