# 01 Phase dependency graph

Canonical current execution state: `../AUTONOMOUS_STATE.md`

```
Track A — the loop maker (items 1-10)
AV-001 ──► AV-002 ──► AV-003 ──► AV-004 ──► AV-005 ──► AV-006 ──► AV-007 ──► AV-008 ──► AV-009 ──► AV-010 ──► AV-015 (CP-A1; every item of A also ──► AV-015)
Track B — the extras (items 11-14)
AV-015 ──► AV-011 ──► AV-012 ──► AV-013 ──► AV-014 ──► AV-016 (CP-B1; every item of B and AV-015 also ──► AV-016)
Late — the owner's wishes after both checkpoints (items 15 … 18)
AV-016 ──► AV-017 ──► AV-018 ──► AV-019 ──► AV-020
```

| Phase | Depends on | Why |
| --- | --- | --- |
| AV-001 | — | the repository, the build, the shell and the gates must stand before anything of the app is made |
| AV-002 | AV-001 | the media needs the build with FFmpeg and the window to open files into |
| AV-003 | AV-002 | the canvas draws the frames the decoder gives |
| AV-004 | AV-003 | the crop is a tool on the canvas, over the video the Selection placed |
| AV-005 | AV-004 | the timeline sets the range of what the crop cuts; playback shows the canvas |
| AV-006 | AV-005 | the loop's maths read the range, the FPS grid and the timeline's band |
| AV-007 | AV-006 | the renderer makes output frame k by LoopMap and TimeMap, with the crop's mask |
| AV-008 | AV-007 | the Preview window shows the renderer's frames |
| AV-009 | AV-008 | the export writes what the Preview shows, by its encoder |
| AV-010 | AV-009 | the project file's UX keeps every setting the items before made |
| AV-015 | AV-001 … AV-010 | CP-A1 walks track A on one fresh build |
| AV-011 | AV-015 | keyframes change the transform the renderer reads — after the loop maker is proved |
| AV-012 | AV-011 | the finder sets In/Out (one Undo step) and opens Preview |
| AV-013 | AV-012 | the batch runs ExportJob entries; the formats extend its profiles |
| AV-014 | AV-013 | the installer ships the app as it finally stands |
| AV-016 | AV-011 … AV-014, AV-015 | CP-B1 walks the whole, installed, and releases `v1.0.0` |
| AV-017 | AV-016 | late: the name and the About box change the app as CP-B1 left it, and walk it themselves |
| AV-018 | AV-017 | late: the wheel and the keyframe copy change the app as AV-017 left it, and walk it themselves |
| AV-019 | AV-018 | late: the clip effects change the frames every output is made of, and walk the earlier outputs again |
| AV-020 | AV-019 | late: the colour theme adds to the clip effects AV-019 made |

Only the lowest ordinal of each track may have no dependency: AV-001 (track A). Track B's lowest, AV-011, depends on
AV-015 by the spec's order (§13).

## While the programme runs — a reading vetoed, an item added

The owner vetoes a reading (`../OWNER-ACTIONS.md` 11 on) or adds a wish. Each, before the next phase:

1. a vetoed reading changes its shape (`control/NN_SHAPES_…`) and its phase first, its row in `../README.md` §3 and
   `../OWNER-ACTIONS.md` saying so;
2. a new item: its words verbatim under `../README.md` §1 and its row in §2; its readings in §3; its shape in a
   `control/NN_SHAPES_…` file; its phase file `phases/AV-NNN-<kebab-title>.md`, **NNN the next item number** — the
   checkpoint files renamed to the ordinals after it, their `phase:` and titles kept, their `depends_on` grown by the
   new phase; the new phase depends on the item before it (the chain);
3. `lib.mjs`'s `OWNER_ITEMS` raised; its requirement rows; its D-number in `../DECISIONS.md` (the next after D-014);
   its rows in `../OWNER-ACTIONS.md`; its risks;
4. `node control/phase-tools/sync-progress.mjs --fix-unlocks`, then `validate-phase-system.mjs` PASS.

## The order of work

`sync-progress.mjs` and `check-continuity.mjs` pick the next phase by the chain rule — a FAILED_RETRYABLE phase first,
then a checkpoint whose track is done, then the lowest eligible ordinal: AV-001 → … → AV-010 → AV-015 → AV-011 → … →
AV-014 → AV-016. The checkpoints are not stops (autonomous mode).

## Checkpoint rule

A checkpoint takes the next free ordinal after the last item (14 items → AV-015, AV-016) and moves up while items are
added.

## Late phases and correctives

An item the owner adds after a checkpoint has closed is a late phase: `late: true` in its front matter, the next free
ordinal above the checkpoints, a dependency on the last checkpoint or on the late phase before it. A corrective a
walk finds takes the next free letter of the ordinal it corrects (`AV-004a`), names `parent_phase`, and runs before
the next phase. **What `../README.md` §4 leaves out is the owner's word, not a late phase of this one.**
