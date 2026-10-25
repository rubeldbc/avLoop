---
phase: AV-002
title: "The media"
status: IN_PROGRESS
owner_item: 2
track: A
checkpoint_id: CP-A1
is_checkpoint: false
risk: High
permission_class: VERIFY_ENVIRONMENT_FIRST
code_budget: 600
depends_on: [AV-001]
unlocks: [AV-003, AV-015]
requirements: [REQ-AV-002, REQ-AV-016, REQ-AV-017, REQ-AV-018, REQ-AV-020]
read_scope:
  - phases/26-10-26-034934-AvLoop/control/05_SHAPES_THE_MEDIA.md
  - phases/26-10-26-034934-AvLoop/adr/AVADR-0002-memory-budgets-and-caches.md
  - docs/SPEC.md
  - spec: §3.5, §3.6, §3.7, §3.8, §3.9, §12.1
  - after AV-001: CMakeLists.txt
  - after AV-001: cmake/FindFFmpeg.cmake
  - after AV-001: src/ui/MainWindow.cpp
---

# The media

Canonical current execution state: `../AUTONOMOUS_STATE.md`

**Goal.** Any video opens by its content and gives frame-accurate, normalized RGBA frames within fixed memory:
`MediaProbe`, the `Decoder` (seek to the exact frame, D3D11VA with a software fallback), the `SourceNormalizer`
(rotation, SAR, deinterlace, HDR→SDR, alpha kept), the `FrameCache` with its disk spill, the sample videos and the
import walk (`../README.md` §2 item 2; D-002; AVADR-0002 accepted).

**Ground truth**

- FFmpeg 7.1 LGPL shared (libavformat, libavcodec, libavutil, libswscale, libavfilter; libvpx, dav1d, zimg inside).
- Spec §3.9's containers and codecs; §12.1's source cases; §3.7's limits (the cache 1536 MB, the packet queue 64
  packets or 32 MB, the disk cache 20 GB); the libvpx decoder for WebM alpha.

**Do**

1. `media/FfmpegPtr.h` (RAII deleters for every FFmpeg object), `MediaInfo.h`, `MediaProbe` (probesize 50 MB,
   analyzeduration 10 s, the default video stream, rotation from the display matrix, SAR, VFR, HDR, interlace,
   alpha, a clear error naming container/codec/reason).
2. `Decoder` + `DecoderSeek.cpp` (seek BACKWARD to the keyframe, decode to the target PTS, forward moves within the
   GOP without seeking, corrupt frames skipped and counted) + `DecoderHw.cpp` (D3D11VA, fallback logged once a file).
3. `SourceNormalizer` (`bwdif`, `zscale`+`tonemap=hable`, rotation and flips, SAR → square pixels; RGBA8 out,
   scaled to the working size), `FrameCache` (LRU with a byte budget; memory-pressure halving) + `DiskCache.cpp`
   (`%LOCALAPPDATA%\AvLoop\cache\<session>\`, oldest first, stale sessions removed at start).
4. Open in the main window: the Open button, Ctrl+O, drag and drop, `avloop.exe <file>`; the status bar's source
   line, `RAM · VRAM · Cache` line and `GPU: <adapter> · HW decode on`; the error toast; `--mem-log`.
5. `avloop-cli probe <video>` and `avloop-cli frame <video> --at N -o x.png`; `tools/media/make-samples.mjs`;
   `tests/test-probe.cpp`, `test-decoder-seek.cpp` (200 random seeks), `test-memory.cpp` (the cache's bound).

**Do not**

Draw the frame on a GL canvas (AV-003), build thumbnails or playback (AV-005), decode audio — ever.

**Done when**

Build, ctest, gates PASS; S2 — every sample of `make-samples` opens; rotation, SAR, interlacing, HDR come out right on
the first/middle/last frame PNGs; the Bengali path works; a broken file gives a message; HW decode on and off give the
same frames; S3's seek half — 200 random seeks give the right testsrc2 frame number; the decode memory walk flat
(Private Bytes growth < 50 MB after warm-up).

**Decisions this phase must record**

D-002 (spec §0); AVADR-0002 accepted; the working-size rule; External freshness (FFmpeg's configuration line).

**Validation**

`scripts\build.ps1`; `ctest … -R "probe|decoder|memory"`; `node tools/media/make-samples.mjs`; `node control/lab/
walk-phase.mjs execution/AV-002-walk walk-import.mjs walk-seek.mjs walk-memory-decode.mjs`; check-loc; check-secrets.

**Seam if this splits.** AV-002a — the disk cache and D3D11VA — after the probe, the software decoder, the
normalizer and the RAM cache.
