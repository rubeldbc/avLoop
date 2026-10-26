---
phase: AV-005
title: "The timeline"
status: DONE
owner_item: 5
track: A
checkpoint_id: CP-A1
is_checkpoint: false
risk: High
permission_class: VERIFY_ENVIRONMENT_FIRST
code_budget: 600
depends_on: [AV-004]
unlocks: [AV-006, AV-015]
requirements: [REQ-AV-005, REQ-AV-016]
read_scope:
  - phases/26-10-26-034934-AvLoop/control/08_SHAPES_THE_TIMELINE.md
  - docs/SPEC.md
  - spec: §6.4, §7
  - after AV-002: src/media/Decoder.h
  - after AV-003: src/core/Project.h
---

# The timeline

Canonical current execution state: `../AUTONOMOUS_STATE.md`

**Goal.** A frame-accurate timeline with thumbnails, In/Out, the transport and playback: the ruler, the thumbnail
strip, the In/Out overlay and handles, the playhead, the In (I) and Out (O) tools, the transport row and its
shortcuts, the playback clock that drops frames rather than stopping (`../README.md` §2 item 5; D-005).

**Ground truth**

- Spec §6.4 (In/Out rules), §7 (rows, input, transport, playback, seek); §3.7 (thumbnails: 400, 160×90).
- The decoder (AV-002); the model and Undo (AV-003).

**Do**

1. `core/Timecode` (`HH:MM:SS.FF`, frame numbers, the output FPS grid), the range in the model + SetRange (In < Out,
   minimum 2 output frames, setting In after Out moves Out to the end and the reverse, Alt+I/Alt+O clear).
2. `ui/timeline/Timeline` + `TimelinePaint.cpp` (ruler with non-overlapping labels, In/Out overlay with the
   `accentSoft` fill and 3 px bar, 50% black outside, handles, playhead) + `TimelineInput.cpp` (scrub — fast
   keyframe preview while dragging, exact on release —, Ctrl+wheel zoom down to 12 px a frame, wheel scroll, `\` and
   Shift+`\`, dragging handles and the bar, snapping within 8 px) + `ThumbStrip.cpp`.
3. `media/ThumbnailWorker` (its own decoder, keyframe seeks, visible part first, 400 thumbnails at most).
4. `ui/timeline/Transport` (Go to In, Prev, Play/Pause, Next, Go to Out, Loop; `TimeEdit`; In, Out, length) and the
   playback clock (`QElapsedTimer` + refresh; drops counted in the status bar; loop between In and Out; Speed applied
   once AV-006 gives it); `ui/widgets/TimeEdit`; `InOutTool.cpp`.
5. Shortcuts: Space, ←/→, Shift+←/→, Home/End, Ctrl+Home/End, L, I, O.

**Do not**

The crossfade band and the loop's output (AV-006, AV-008); the keyframe row's behaviour (AV-011) — the row is drawn
empty.

**Done when**

Build, ctest, gates PASS; S6's half — In/Out by the tool, the shortcut and dragging; no label overlap at any zoom
(screenshots at five zooms); S3's half — 1080p H.264 at 30 and 60 fps plays 10 s with 0 dropped frames (the status
bar's counter in the log); the memory flat over a minute of loop playback.

**Decisions this phase must record**

D-005 (spec §0); the scrub's two-speed rule.

**Validation**

`scripts\build.ps1`; `ctest … -R "timecode|commands"`; `node control/lab/walk-phase.mjs execution/AV-005-walk
walk-timeline.json walk-playback.mjs`; format; check-loc; check-secrets.

**Seam if this splits.** AV-005a — the thumbnail strip and its worker — after the ruler, In/Out, playhead,
transport and playback.
