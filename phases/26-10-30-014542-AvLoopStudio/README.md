# AvLoop Studio — AvLoop restructured as a mini Premiere Pro

Programme `AVSTUDIO`, prefix `AS`. Canonical current execution state: `AUTONOMOUS_STATE.md`.

Planned 2026-10-30 (01:45, Asia/Dhaka) on the owner's word, after AvLoop's programme (`../26-10-26-034934-AvLoop`,
33 phases, the last deferred to this one). The app keeps its name, its media, renderer, effects, shapes, Pen, export
and gates; what changes is its **structure**: one video with a crop and layers on top becomes a **sequence of tracks
and clips**, every clip equal, edited as in Premiere Pro.

## 1. The owner's words

2026-10-30, verbatim: *"I think the applicaiton stracture is not good enough to edit. Make this application like mini
adobe premiere pro. Make the planning in a new and different folder. the start coding yourself. My final goal that I
have to flexible edit and cutting like hole a video and export as transparent. do the planning and continue to code"*.

Standing from AvLoop's programme (its `README.md` §1): Windows only; the app draws its own icons; no audio; no output
above 1080p; no code file over 600 lines; autonomous mode (decide, record, continue); every phase pushed to `origin`
`main` with CI green; the README professional and current.

## 2. The items

| # | Item | What it gives |
| --- | --- | --- |
| 1 | **The sequence** (AS-001) | the model: a sequence of video tracks, each of clips; a clip its media, place in time, In/Out, motion and opacity keyframes, effects, masks; the edits (split, trim, move, ripple delete, insert, overwrite) as pure functions; one Undo step an edit; the file's version 4 reading every AvLoop file |
| 2 | **The composite** (AS-002) | one renderer for a sequence time: every clip present, bottom to top — decoded, its effects, its masks, its keyed motion and opacity — over a transparent frame; the loop modes and the output rate over the sequence's In/Out; the Preview, the export and `avloop-cli` on it |
| 3 | **The editor's shell** (AS-003) | Premiere's layout: the Project panel (the media bin: import, list, drag to the timeline), the Program monitor (the composite at the playhead), the Effect Controls, the Timeline; the sequence frame set by the Frame tool |
| 4 | **The timeline editor** (AS-004) | tracks V1 … V8 with headers (eye, lock, add, remove), clip bars with thumbnails, selection (click, Shift, marquee), move with snapping across tracks, trims, the Razor (C) and Ctrl+K split, Delete and ripple delete, the sequence In/Out |
| 5 | **The Effect Controls** (AS-005) | for the selected clip: Motion (position, scale), Opacity, Masks, the clip effects — each with a stopwatch, its keyframes on a small lane; an edit at the playhead keys |
| 6 | **The masks on the monitor** (AS-006) | the shape tools and the Pen draw masks on the selected clip; Add, Subtract, Intersect, Difference, Inverted — **a hole cut in any clip** —, feather, opacity; the monitor's selection, handles, align |
| 7 | **The export** (AS-007) | the sequence's In → Out (or a loop of it) as WebM VP9/VP8 with alpha, ProRes 4444, APNG, GIF, MP4; transparent where no clip covers or a mask cuts; the batch and `avloop-cli export` of a sequence |
| 8 | **The project** (AS-008) | `.avloop` version 4: save, open, autosave and recovery, Recent; an AvLoop 1.x file opened as a sequence; presets and settings kept |

Checkpoint: **CP-S1** (AS-009) walks the whole on one fresh build — every walk that still means something, the new ones,
the memory walk, ASan and Debug leaks — and finishes AvLoop's deferred AV-032.

## 3. The readings taken

In `READINGS.md`. Each is the agent's under autonomous mode, open for the owner's veto.

## 4. What is not built

- Audio of any kind; transitions; nested sequences; speed ramps; markers; titles and text; output above 1080p.
- A second sequence in a project; a rotation property (AvLoop never had a rotation tool).
- macOS or Linux; an updater; licence keys; telemetry; code signing.

## 5. Where things are

The shapes `control/03_SHAPES_THE_EDITOR.md`; the order `control/01_PHASE_DEPENDENCY_GRAPH.md`; the checkpoint
`control/02_CHECKPOINT.md`; `READINGS.md`, `DECISIONS.md`, `RISKS.md`, `OWNER-ACTIONS.md`,
`requirements/01-00_REQUIREMENTS.md`; the tools `control/phase-tools` and the lab `control/lab` (AvLoop's, prefix `AS`).
The code is AvLoop's tree (`src/core`, `src/media`, `src/render`, `src/export`, `src/ui`, `src/cli`).
