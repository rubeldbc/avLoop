# 12 Shapes — the export (item 9, AV-009)

Canonical current execution state: `../AUTONOMOUS_STATE.md`

Spec §10.1–§10.3, §12.3. D-009. Reading R-6 (default output).

## 1. The job (`export/ExportJob`)

- Started by Export / Ctrl+E (the Save dialog the first time: `<source>_loop_<W>x<H>.webm`, the last folder, overwrite
  confirmation) or `avloop-cli export`. A snapshot of the project is taken; editing during the export changes nothing
  of it.
- A `QThread`; stages with bounded queues (decode → render 6 frames, render → encode 6 frames, backpressure): the
  decoder feeding the `FrameRenderer` (GPU; PBO readback) → swscale `yuva420p` → `Encoder` → `Muxer`; every frame
  `pts = k`, `time_base = 1/fps`.
- `<name>.part`, then `MoveFileExW(MOVEFILE_REPLACE_EXISTING)`; a locked target (OBS) → 5 retries 1 s apart, then
  `name (1).webm` offered; Cancel or error → `.part` deleted; disk full → stop, delete, "needs N MB more"; no write
  permission → a message and the Save dialog again; an allocation failure → a clean stop with a message; GPU lost →
  CPU from that frame.
- Encoded preview reuse: the same settings hash → the temp file copied instead of encoding.

## 2. Verify (`export/OutputVerify`)

Reopen the file: frame count = L, FPS = the output's, W×H, codec, alpha (first frame by libvpx, the alpha plane read)
when Alpha is on or needed, the first frame a keyframe. A mismatch → an error toast and a log entry.

## 3. Panels

- Output (`OutputPanel`): Format (WebM VP9 default, WebM VP8 — GIF, APNG, MP4 H.264, MOV ProRes 4444 at AV-013),
  Size (Crop size, Long side 1920/1280/1080/720/512/256, Custom W×H with lock), Alpha (Auto, On, Off), Background
  (colour, default black), FPS / FPS method. All sizes even, ≥ 16, long side ≤ 1920 with a note when scaled down.
- Quality (`QualityPanel`): Low / Medium / High / Custom per §10.3's table (VP9: crf 40/31/24, cpu-used 4/2/1; VP8:
  crf 30/15/6, b:v 2M/6M/12M); Custom: CRF 4–63, Speed 0–8, Max bitrate (optional), Two-pass, Target size (MB).
- Target size: two-pass with `b:v = target_bytes × 8 / duration × 0.9`; > 5% over → once more at a corrected rate.
- `SizeEstimator`: 3 spread parts × 10 frames encoded quickly, extrapolated; 1 s after a change; `~1.8 MB`.

## 4. The UI around it

`ExportBar` in the status bar (`Exporting 86/149 · 58% · 41 fps · ETA 0:02` + Cancel); success toast `Exported
640×640 · 4.97 s · 1.8 MB` + *Show in folder*; the OBS tip the first time a session ("Media Source → Loop on · Use
hardware decoding off (keeps transparency)", *Don't show again*); closing during an export asks "Export in progress —
cancel and quit?"; Export/Preview disabled with the reason when D < 2 or the source is a single image; "Output is
fully transparent" asks for confirmation.

## 5. Tests and walks

`test-export` (VP9 and VP8 small exports: frame count, keyframe, alpha; Cancel leaves no `.part`; the size rule).
Walks: `walk-export.mjs` (S10 by ffprobe and `alphaOf`, Cancel, Target size ±5%, VP8), `walk-loops.mjs` (S7: four
modes, frame counts = L, `seamDiff`, duplicates), `walk-fps.mjs` (S8: 15/24/30/60/Custom 12, three methods,
`r_frame_rate`, Speed 0.5×/2×; S11 `-stream_loop 1` = 2L), `walk-export-ui.json` (the panels, the progress bar, the
toast, pictures), three exports in a row with `--mem-log`.
