---
phase: AV-031
title: "The timeline layers II — the tracks"
status: NOT_STARTED
owner_item: 27
track: B
checkpoint_id: CP-B1
is_checkpoint: false
late: true
risk: High
permission_class: VERIFY_ENVIRONMENT_FIRST
code_budget: 1300
depends_on: [AV-030]
unlocks: []
requirements: [REQ-AV-034]
read_scope:
  - phases/26-10-26-034934-AvLoop/control/30_SHAPES_THE_TIMELINE_LAYERS.md
  - phases/26-10-26-034934-AvLoop/control/08_SHAPES_THE_TIMELINE.md
  - docs/SPEC.md
  - after AV-030: src/core/Sequence.h
  - src/ui/timeline/Timeline.h
  - src/ui/timeline/TimelinePaint.cpp
  - src/ui/timeline/TimelineInput.cpp
  - src/media/ThumbnailWorker.h
  - src/ui/tools/SelectionTool.cpp
  - src/ui/MainWindowTimeline.cpp
  - src/ui/panels/SidePanel.cpp
  - spec: §5.1, §7, §7.1, §7.2, §11.2
---

# The timeline layers II — the tracks

Canonical current execution state: `../AUTONOMOUS_STATE.md`

**Goal.** The tracks on the timeline as Premiere Pro's: V1 … Vn with their headers (name, eye, lock), each clip a bar
with its name and thumbnails, dragged in time and across tracks with snapping, trimmed at its ends, copied with Alt,
deleted; the selected clip on the canvas, in the keyframe row and in the side panel (`../README.md` §1 item 27;
D-027). A late phase, the second of two.

**Ground truth**

- The owner's words (`../README.md` §1, 2026-10-29); AV-030's model and composite.
- Spec §7 (the timeline's rows and input), D-005 (the ruler, the thumbnails, the drags, R-24 … R-27), R-64 (the
  Alt+drag copy); Premiere Pro's track lanes, headers and trims.

**Do**

1. `ui/timeline/TrackLanes.cpp` (the lanes, the headers, the clip bars, their thumbnails) and `TrackInput.cpp` (the
   drags, the trims, the snapping, the copy, Delete, the double-click); the timeline's height and scroll; the
   keyframe row of the selected clip.
2. The canvas: the Selection's click selecting the topmost visible clip; the tools and the shapes on it; the status
   bar's "V2 · name"; the side panel's Clip line.
3. The track menu (Add track above/below, Remove, Rename); the overlap rule; the thumbnails' shared budget.
4. Tests; `walk-tracks.json`, `walk-tracks-memory.mjs`; the README's feature list and the timeline picture; the
   shortcut overlay.

**Do not**

Let a clip overlap another on its track; move the range's In/Out handles or the playhead's behaviour; draw a lane's
thumbnails by a second decoder of the same source (one worker a clip); pass 600 lines — `Timeline.cpp` (185),
`TimelinePaint.cpp` (187), `TimelineInput.cpp` (207) are read first; the lanes are their own files.

**Done when**

Build 0 warnings; ctest; format, icons, secrets, LOC PASS; `walk-tracks.json` and `walk-tracks-memory.mjs` PASSED;
the walks of the timeline (AV-005), playback (AV-005), the keyframes (AV-011), the keyframe copy (AV-018), the
shapes (AV-024) and the layers (AV-030) PASSED again; README current; pushed with CI green.

**Decisions this phase must record**

D-027 (spec §0, with AV-030); the readings AV-030 left open, if any (R-104 …).

**Validation**

`node control/lab/walk-phase.mjs execution/AV-031-walk walk-tracks.json walk-tracks-memory.mjs`; the regressions;
check-loc; check-secrets; `validate-phase-system.mjs`.

**Seam if this splits.** AV-031a — the drags across tracks and the trims — after the lanes, the bars and the
selection.
