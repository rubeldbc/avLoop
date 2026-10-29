# 01-00 Requirements

Canonical current execution state: `../AUTONOMOUS_STATE.md`

| ID | Requirement | Owner item | Phase | Evidence | S |
| --- | --- | --- | --- | --- | --- |
| REQ-AS-001 | The sequence: The model of a mini Premiere Pro: a sequence of video tracks, each of clips equal to each other — media, place in time, In/Out, motion and opacity keyframes, effects, masks; the edits as pure functions; one Undo step an edit; the file's version 4 reading every AvLoop file. | 1 | AS-001 | — | — |
| REQ-AS-002 | The composite: One renderer for a sequence time: every clip present, bottom to top, decoded, its effects, its masks, its keyed motion and opacity, over a transparent frame; the loop engine over In/Out; the Preview, the export and avloop-cli made by it. | 2 | AS-002 | — | — |
| REQ-AS-003 | The editor's shell: Premiere's layout: the Project panel (the media bin), the Program monitor (the composite at the playhead), the Effect Controls, the Timeline; the Frame tool for the sequence frame. | 3 | AS-003 | — | — |
| REQ-AS-004 | The timeline editor: Premiere's timeline: tracks V1 … V8 with headers, clip bars with thumbnails, selection, moves with snapping, trims, the Razor and Ctrl+K, Delete and ripple delete, the sequence In/Out. | 4 | AS-004 | — | — |
| REQ-AS-005 | The Effect Controls: Premiere's Effect Controls for the selected clip: Motion, Opacity, Masks, the clip effects; stopwatches and keyframes on a small lane. | 5 | AS-005 | — | — |
| REQ-AS-006 | The masks on the monitor: Masks drawn on the selected clip on the monitor — rectangles, ellipses, polygons, freehand, the Pen — Add, Subtract, Intersect, Difference, Inverted (a hole in any clip), feather, opacity; the monitor's selection moving and scaling clips. | 6 | AS-006 | — | — |
| REQ-AS-007 | The export: The sequence's In → Out (or a loop of it) exported as WebM VP9/VP8 with alpha, ProRes 4444, APNG, GIF or MP4; transparent where nothing covers or a mask cuts; the batch and avloop-cli. | 7 | AS-007 | — | — |
| REQ-AS-008 | The project: The project of a sequence: save, open (any version), autosave and recovery, Recent, presets and settings — AvLoop's, carried over. | 8 | AS-008 | — | — |
| REQ-AS-009 | CP-S1: the whole walked on one fresh build; Release, ASan, Debug-leak suites. | all | AS-009 | — | — |
