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
10. **RS-10** (AS-002) AvLoop's crop is the sequence's **matte**, over every track in frame px — as AvLoop drew it over
    the main video and the layers (a clip mask would move with its clip and leave the layers uncut). RS-6 refined: one
    plain clip keeps AvLoop's own path and its GPU back end (its bytes, its speed; the composite agrees within 1/255);
    any other sequence is the composite's, on the CPU — so "draw an output frame only in the SequenceRenderer" reads
    "only in the FrameRenderer, by the composite beyond one plain clip".
11. **RS-11** (AS-002) A loop's blend mixes whole composites: a crossfade fades the layers with the main video (AvLoop
    drew them unblended at the dominant frame's time). A migrated file with layers differs in the blend's frames only.
12. **RS-12** (AS-002) The FPS method runs on each clip by its own media's frame times; Motion Interpolation's pre-pass
    stays the main video's (V1's clip from the source's start), other clips blend.
13. **RS-13** (AS-002) Each clip is decoded at its own working size (R-16's rule; AvLoop's layers were decoded whole);
    V1's clips may use the hardware decoder, the tracks above the software one; a clip whose media is offline or past
    its end is not drawn (as AvLoop's layers), never an error.
