# 08 Shapes — the timeline (item 5, AV-005)

Canonical current execution state: `../AUTONOMOUS_STATE.md`

Spec §6.4, §7. D-005.

## 1. Time

- `Timecode`: seconds ↔ `HH:MM:SS.FF` at the output FPS (rational) and ↔ frame numbers; parse what `TimeEdit` accepts
  (`12.4`, `00:00:12.12`, `372f`).
- The range in seconds (source time); `snapToGrid(t)` to the output FPS grid measured from the source's start;
  `SetRange` enforces In < Out and the minimum of 2 output frames (§8.1's D ≥ 2); In set after Out moves Out to the
  file's end; Out set before In moves In to the start; Alt+I / Alt+O clear to the start / end.

## 2. The widget (`ui/timeline/`)

- Rows (top to bottom): the ruler 18 px (timecode or frame numbers, double-click switches; tick spacing from the zoom
  — 1 frame up to 10 minutes — chosen so labels never overlap: the label width measured, the step raised until it fits);
  the thumbnail strip 54 px; the In/Out overlay (In–Out `accentSoft` fill + 3 px orange bar on top; outside 50% black;
  10 px bracket handles); the crossfade band (AV-006); the keyframe row 16 px (AV-011); the playhead (orange line +
  triangle) over all.
- `TimelinePaint.cpp` paints; `TimelineInput.cpp` handles: click/drag on the ruler or strip scrubs (while dragging the
  canvas shows the nearest keyframe's frame — the fast preview —, on release the exact frame); Ctrl+wheel zooms about
  the cursor (the whole file … 12 px a frame); wheel / Shift+wheel scroll; `\` fits the file, Shift+`\` the In–Out;
  dragging a handle changes In/Out (one Undo step; the canvas shows that frame); dragging the bar moves the range
  keeping its length; the playhead snaps to In/Out/keyframes within 8 px.
- `ThumbStrip.cpp` draws the thumbnails (`panel2` blocks until ready).

## 3. Thumbnails (`media/ThumbnailWorker`)

Its own decoder (software, keyframe seeks only — `AVSEEK_FLAG_BACKWARD` then the first frame), 160×90 fitted, at most
400 cached (≈ 23 MB); requests for the visible part first, the rest after; cancelled when the video changes; results
as `QImage` delivered to the UI thread.

## 4. Transport and playback

- `Transport`: Go to In, Prev frame, Play/Pause, Next frame, Go to Out, Loop on/off (icons); `TimeEdit` (type to jump)
  and the total duration; the In, Out and length readout (Consolas).
- Shortcuts: Space, ←/→ (1 frame), Shift+←/→ (10), Home/End (In/Out), Ctrl+Home/End (file), L (loop), I/O.
- The playback clock: `QElapsedTimer` + a `QTimer` at the display refresh; the source time = start + elapsed × Speed;
  the decoder is asked for the frame at that time; if it is not ready the clock never waits — the last frame stays,
  the frame is counted dropped (the status bar `Dropped: N`); with loop on it cycles In → Out. Playback shows the
  source (no loop mode; Speed applied from AV-006).
- The decoder's playback mode: decode ahead sequentially into a small ring (6 frames) in display order.

## 5. Tests and walks

`test-timecode` (formats, parsing, grid), `test-commands` grows SetRange's rules. Walks: `walk-timeline.json` (I/O by
the tool, the shortcut and dragging; Alt+I/O; the bar dragged; zoom levels with pictures for label overlap);
`walk-playback.mjs` (`avloop.exe --walk` with a play step of 10 s on the 1080p30 and 1080p60 samples, the dropped
counter read from the log; `--mem-log` flat over a minute of loop playback).
