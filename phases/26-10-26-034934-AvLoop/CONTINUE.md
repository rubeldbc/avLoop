# CONTINUE

**The programme is STARTED (2026-10-26, the specification's go-ahead and the owner's "start")**; the active phase is in
`AUTONOMOUS_STATE.md`. Fourteen items: AV-001 the ground, AV-002 the media, AV-003 the canvas and the Selection, AV-004
the crop tools, AV-005 the timeline, AV-006 the loop engine, AV-007 the renderer, AV-008 the preview window, AV-009 the
export, AV-010 the project and the settings, AV-011 the keyframes, AV-012 the loop finder, AV-013 the batch and the
other formats, AV-014 the installer; the checkpoints AV-015 (CP-A1) and AV-016 (CP-B1). The order: AV-001 → … → AV-010
→ AV-015 → AV-011 → … → AV-014 → AV-016. Autonomous mode: the owner is never asked; the readings of `README.md` §3 are
open for a veto later.

Pick up here after a break or a new session.

1. Read `AUTONOMOUS_STATE.md` (state, active phase, next eligible), then `EXECUTOR-PREAMBLE.md`.
2. `node control/phase-tools/check-continuity.mjs` — it says whether a phase is open (`WORKING: AV-NNN is
   IN_PROGRESS`), an eligible phase waits (`STOPPED: …`) or the programme is finished (`FINISHED: …`).
3. **The owner vetoes a reading or adds a wish**: change the reading in its shape and phase first; a wish that is a
   new item — `control/01_PHASE_DEPENDENCY_GRAPH.md`, *an item added*; `sync-progress.mjs --fix-unlocks`;
   `validate-phase-system.mjs` PASS.
4. If a phase is open, read its file in `phases/` and its `execution/` record if one exists; `git status` and the
   last commit say how far it got; finish it, walk it, close it with `close-phase.mjs`; commit `[AV-NNN]`, push,
   watch CI.
5. At the end of every phase: `check-loc.mjs` PASS; `check-secrets.mjs` PASS; `git status -sb` reads
   `## main...origin/main` with nothing ahead; CI green; no avloop process, session cache or walk temp file left.
6. The environment, if this is a new machine: `scripts\fetch-deps.ps1` (FFmpeg), Qt 6.8.3 by
   `python -m aqt install-qt windows desktop 6.8.3 win64_msvc2022_64 -O <dir>` and `QTDIR` pointing at it, then
   `scripts\build.ps1 -Fresh`.
