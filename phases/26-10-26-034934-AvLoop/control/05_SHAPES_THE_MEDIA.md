# 05 Shapes — the media (item 2, AV-002)

Canonical current execution state: `../AUTONOMOUS_STATE.md`

Spec §3.5–§3.9, §12.1. D-002; AVADR-0002 (memory budgets and caches) accepted here.

## 1. Probe (`media/MediaProbe`, `MediaInfo.h`)

- `avformat_open_input` with `probesize` 50 MB, `analyzeduration` 10 s; the path as UTF-8 (`\\?\` long paths work by
  FFmpeg's own Windows wide-char handling); `avformat_find_stream_info`; the default video stream
  (`av_find_best_stream`); an audio-only file or an unknown codec → `"This file has no video AvLoop can decode (codec:
  X)"`.
- `MediaInfo`: path, container (long name), codec (long name and id), coded and display size (after rotation and SAR),
  SAR, rotation (display matrix: 0/90/180/270 and flips), `avg_frame_rate`, `r_frame_rate`, VFR flag (the two differ
  or the first 64 frames' durations do), duration, frame count estimate, time base, start time, pixel format, bit
  depth, has alpha (pix fmt alpha, or VP8/VP9 `alpha_mode` tag), HDR (PQ/HLG transfer, BT.2020), interlaced (field
  order), the status-bar line `1920×1080 · 29.97 fps · H.264 · VFR → CFR`.

## 2. Decode (`media/Decoder`, `DecoderSeek.cpp`, `DecoderHw.cpp`)

- One thread a decoder; a bounded packet queue (64 packets or 32 MB); `frameAt(pts)` returns the normalized frame whose
  PTS is nearest at or before the target per the caller's rule.
- Seek: `av_seek_frame(AVSEEK_FLAG_BACKWARD)` to the target, flush, decode until the frame whose interval holds the
  target; a forward move within the current GOP (target ahead and before the next keyframe's PTS, or within 2 s)
  decodes on without seeking. Timestamps by `best_effort_timestamp` minus the stream's start time.
- VP8/VP9 with alpha: the decoder is `libvpx-vp9` / `libvpx` by name (the native drops alpha).
- D3D11VA: `av_hwdevice_ctx_create(AV_HWDEVICE_TYPE_D3D11VA)`, `get_format` picking `AV_PIX_FMT_D3D11`,
  `av_hwframe_transfer_data` to NV12/P010; any failure → software decode, logged once a file; "Hardware decoding: Auto
  / Off".
- Corrupt packets/frames: skipped, the previous good frame stands, a counter in the log.

## 3. Normalize (`media/SourceNormalizer`)

- An `AVFilterGraph` built per source: `bwdif=mode=send_frame` when interlaced; `zscale` + `tonemap=hable` + `zscale`
  to BT.709 when HDR; `transpose`/`hflip`/`vflip` for rotation; `scale` to square pixels (SAR) and to the working
  size; `format=rgba` (premultiplied later by the renderer). The graph is rebuilt when the working size changes.
- Working size: the display size × min(1, the effective scale needed) — a 4K source is scaled down right after
  decoding; full 4K frames are never kept (§3.7.3). Until a crop exists the working size is the display size capped
  at 1920 on the long side.

## 4. Cache (`media/FrameCache`, `DiskCache.cpp`)

- RAM LRU keyed by (source, working size, PTS); the budget in bytes (1536 MB default, 256–8192); `QImage`-free
  storage (aligned RGBA buffers from a pool); hits and misses counted for the status line.
- Disk spill: `%LOCALAPPDATA%\AvLoop\cache\<session>\` raw RGBA files per frame (crop region at output size later);
  20 GB, oldest first; the session folder deleted on exit; stale sessions (no live PID) deleted at start; disk full →
  stop writing, RAM only.
- Memory pressure: `GlobalMemoryStatusEx` each second; free RAM < 1 GB → budgets halved and a toast.

## 5. The window and the CLI

- Open: the button, Ctrl+O (`All media (*.*)` + known extensions), drag and drop, `avloop.exe <file>`; opening a new
  video releases everything of the previous one; the status line; the drop zone hidden.
- `--mem-log <file>`: Private Bytes (`GetProcessMemoryInfo`), VRAM (`IDXGIAdapter3::QueryVideoMemoryInfo`) every second.
- `avloop-cli probe <video>` (JSON of `MediaInfo`), `avloop-cli frame <video> --at N -o x.png` (normalized frame N).

## 6. Tests and walks

`test-probe` (the samples' fields), `test-decoder-seek` (200 random seeks on testsrc2: the frame number read from the
burned counter by comparing against a reference decode of the same frame index, sequentially), `test-memory` (the
cache never exceeds its budget; eviction order). Walks: `walk-import.mjs` (every sample; first/middle/last frame PNGs
for the special ones described), `walk-seek.mjs`, `walk-memory-decode.mjs` (`--mem-log`).
