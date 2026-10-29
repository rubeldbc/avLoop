# The readings taken (`README.md` §3)

Canonical current execution state: `AUTONOMOUS_STATE.md`

Each is built as read and open for the owner's veto (`OWNER-ACTIONS.md`; `DECISIONS.md`). AvLoop's R-1 … R-107
(`../26-10-26-034934-AvLoop/READINGS.md`) stand unless one below replaces it.

1. **RS-1** "Like mini Adobe Premiere Pro": one sequence a project, video tracks V1 … V8 (no audio, R-1's), clips equal
   to each other; the editing gestures of Premiere's timeline (select, move, trim, razor, ripple) and its Effect
   Controls (Motion, Opacity, Masks, effects, stopwatches). Not built: `README.md` §4.
2. **RS-2** The sequence frame replaces AvLoop's crop as the output's area: its size the first clip's by default, set by
   the Frame tool (presets and custom, long side ≤ 1920); scene space is the frame's.
3. **RS-3** "Cutting like a hole in a video": a clip's masks — AvLoop's shapes (AV-024 … AV-029) moved from the crop to
   each clip, in the clip's own pixels; Inverted (or Subtract) makes the hole; transparent where nothing covers.
4. **RS-4** Undo: one value command over the whole sequence a gesture (small values, 200 steps).
5. **RS-5** The file's version 4, `"sequence"`; every AvLoop file (versions 1 … 3) opens as a sequence (`control/03` §1).
6. **RS-6** Every output frame is made by one CPU pass over the clips (the GPU and the CPU cannot differ); the
   GL back end stays for the monitor's live look — AVADR-0001's parity by construction.
7. **RS-7** While AS-003 … AS-006 move the UI, AvLoop's model and panels keep working beside the sequence; each phase
   moves what it names, so main never holds a broken app.
8. **RS-8** The export's loop mode defaults to None (the sequence as cut); the loop modes stay a choice.
9. **RS-9** (AS-001) Until AS-003 the sequence is made from AvLoop's model (`Project::sequence()`, `Sequence::fromAvLoop`)
   and written to the file only when the source was probed (its size and length are not in the file); `changeSequence`
   comes with the editor that edits it (AS-003). The JSON lives in `core/Sequence.cpp` (no `SequenceJson`: 243 lines).
