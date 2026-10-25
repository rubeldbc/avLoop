# AVADR-0002 — Memory budgets and caches

Status: **Accepted 2026-10-26** by AV-002 (drafted by the plan), before the cache's code.

## Context

The owner: *"Be extremely careful about memory."* Spec §3.7 makes every rule mandatory and checked by a gate: fixed
limits for every cache and queue, no whole segment in RAM, the working resolution, RAII, lifetimes, behaviour under
pressure, visibility, and the memory walk (Private Bytes growth < 50 MB, VRAM < 32 MB after warm-up).

## Decision

1. **Every container that grows with the video has a limit** in bytes or count, enforced at insertion:

   | Item | Limit (default) | Owner |
   | --- | --- | --- |
   | `FrameCache` (RAM) | 1536 MB (Settings 256–8192) | `media/FrameCache` |
   | GPU texture pool | 512 MB or 25% of VRAM, whichever is smaller | `render/GlRenderer` |
   | Disk cache | 20 GB, oldest first | `media/DiskCache.cpp` |
   | Thumbnails | 400 at 160×90 (~23 MB) | `media/ThumbnailWorker` |
   | Decoder packet queue | 64 packets or 32 MB | `media/Decoder` |
   | Export pipeline | 6 + 6 frames (backpressure) | `export/ExportJob` |
   | Undo stack | 200 steps, values only | `core/Commands` |
   | Preview cache | a share of `FrameCache`'s budget | `ui/preview/PreviewPlayer` |
   | Fingerprints | 4 KB a frame of the search range, freed at close | `export/LoopFinder` |

2. **Working resolution**: decoded frames are scaled right after decoding to what the output needs (display size ×
   min(1, effective scale)); full-size 4K frames are never kept. The cache key carries the working size.
3. **No whole segment in RAM**: playback and export stream; the reversed half of a large Ping-pong goes GOP by GOP into
   the disk cache and is read back from there; the Interpolator's results go to the disk cache.
4. **RAII**: `media/FfmpegPtr.h` gives `std::unique_ptr` + deleter for every FFmpeg object; GL objects are owned by
   their renderer and freed with its context; frame buffers come from pools (`av_buffer_pool`, an RGBA pool).
5. **Lifetimes**: a new video releases everything of the old; closing Preview releases its player, textures and cache;
   a finished or cancelled export frees its buffers and temp files; the session's disk cache is deleted on exit and
   stale sessions at the next start.
6. **Pressure**: free RAM < 1 GB (checked each second) → budgets halved, a toast; a failed allocation in the export →
   a clean stop with a message.
7. **Visible and measured**: the status bar `RAM 812 MB · VRAM 210 MB · Cache 1.1/1.5 GB`; `--mem-log`; the memory
   walks at AV-002, AV-005, AV-007, AV-008, AV-009 and the checkpoints; the Debug CRT leak report; ASan at checkpoints.

## Consequences

- A cache miss costs a decode; the budget trades speed for a flat memory curve.
- Tests (`test-memory`) hold the bounds; a container without a limit is a review failure.
