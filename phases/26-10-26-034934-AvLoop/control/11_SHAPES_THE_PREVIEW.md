# 11 Shapes — the preview window (item 8, AV-008)

Canonical current execution state: `../AUTONOMOUS_STATE.md`

Spec §9, §10.3. D-008. Reading R-14 (the encoder lives here first).

## 1. The window (`ui/preview/PreviewWindow`)

- Opened by the Preview button or F5; non-modal, resizable, one at a time (again = to the front); default size the
  output size + controls, at most 80% of the screen; position and size remembered; the app icon.
- Top row: Live | Encoded (segmented), Background (Checker, Black, White, Green `#00FF00`, Navy, Image…), Zoom (Fit,
  50%, 100%, 200%; pixel-exact at 100%).
- The view: the output frame over the background (the checker = transparent).
- The bottom: Play/Pause, the slider 0 … L−1 (frame-accurate, ←/→), time `2.31 / 4.97 s`, Around seam; the seam marks
  under the slider; the cached-part bar; the info row `640×640 · 30 fps · 149 frames · VP9 alpha · ~1.8 MB`.
- Live updates 250 ms after a change in the main window (debounced); closing releases the player, the textures and
  its cache.

## 2. The player (`PreviewPlayer`)

- Always loops (O[0] follows O[L−1]); Space toggles; the clock by `QElapsedTimer`.
- Live: frames from the `FrameRenderer` on a worker thread, stored in the preview's own bounded cache (frames at the
  output size; within the FrameCache's budget share); first pass slower than real time if needed — "Caching…" shown,
  never dropping — then real time.
- Around seam: plays the last 2 s + the first 2 s repeatedly (the whole loop if it is shorter than 4 s).
- Seam marks: the slider's two ends; Crossfade: an orange strip over the fade part (the last F frames of the output
  hold the blend: indices D − 2F … L − 1); Ping-pong: the two turn points (k = 0 and k = D − 1).

## 3. Encoded

- `media/Encoder` + `EncoderOptions.cpp` + `media/Muxer`: RGBA → `swscale` → `yuva420p` (or `yuv420p` with Alpha
  Off) → `libvpx-vp9` / `libvpx` with §10.3's parameters (`-b:v 0 -crf -deadline good -row-mt 1 -threads <cores>
  -tile-columns <log2(W/256)> -g <L> -auto-alt-ref 1 -lag-in-frames 25`; VP8: `-auto-alt-ref 0`, `crf` + `b:v`); the
  WebM muxer with `alpha_mode` set; `pts = k`, `time_base = 1/fps`; one GOP; no audio stream.
- The temp file in the session's temp folder, deleted at close; progress shown; the player decodes it with the libvpx
  decoder (alpha) and loops it; "Out of date" when settings change (no auto re-encode); the actual size next to it;
  the encoded result kept (path + settings hash) so Export with the same settings copies it (AV-009 uses this).

## 4. Tests and walks

`test-export` starts here: the encoder writes a 10-frame VP9 alpha WebM read back with alpha by libvpx. Walks:
`walk-preview.json` (open, F5, Live pictures with each background, Around seam, Encoded, close ×5 with `--mem-log`);
`walk-preview-check.mjs` (Live frame N grabbed by the walk vs `avloop-cli frame N`: mean ≤ 1.0; Encoded's temp file's
alpha by `alphaOf` before close; memory back after closes).
