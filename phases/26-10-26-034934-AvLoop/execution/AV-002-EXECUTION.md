# AV-002 — Execution Record

**Phase:** AV-002 — The media (owner item 2)
**Status:** DONE
**Date:** 2026-10-26
**Phase start commit:** `7b0946f` (*[AV-001] The ground*)
**ADR:** AVADR-0002 (memory budgets and caches) accepted before the cache's code; indexed in `adr/README.md`
**Environment class:** local-disposable (this PC: `build/`, `samples/` made by `make-samples.mjs`, the walk's folder)

## The owner's words

*"Maximum care with memory (§3.7). Use the GPU (§3.6)."* — *"State which libraries are used and why (§3.8). Any video
must be importable (§3.9)."*

## What was built

1. `src/media/FfmpegPtr.h` — `unique_ptr` + deleter for every FFmpeg object; `avError`. `MediaInfo.h` — the reading and
   the shared, pooled `Frame` (RGBA premultiplied; pixels return to libavfilter's pool with the last copy).
   `src/core/Rational.h`.
2. `MediaProbe` — content probing (50 MB / 10 s), the default video stream, display matrix as ffmpeg's autorotate,
   SAR → square even sizes, VFR, HDR, interlace, alpha (pix fmt or WebM `alpha_mode`), the status line, plain refusals.
3. `Decoder` (pimpl; `DecoderImpl.h`) + `Decoder.cpp` (open, codec, read/decode, normalize on demand) +
   `DecoderSeek.cpp` (BACKWARD seek, forward within 2 s, retries earlier, byte rewind of MPEG-TS/PS, wait for a key
   packet) + `DecoderHw.cpp` (D3D11VA, frames transferred only when shown, software fallback once a file).
4. `SourceNormalizer` — one AVFilterGraph a source: bwdif (a graph a frame with its field neighbours), zscale +
   tonemap=hable, transpose/flips, scale to the working size, rgba; premultiply.
5. `FrameCache` (LRU in bytes, 256–8192 MB, halve/restore, per-entry spill) + `DiskCache` (session folder, 20 GB,
   oldest first, stale sessions by PID and start time, full disk → RAM only); `SystemMemory` (Private Bytes, DXGI VRAM,
   free RAM, the adapter's name); `DecodeWorker` (a decoder on its own thread, coalesced requests, frames into the
   cache).
6. `src/ui/MainWindowActions.cpp` — Open (dialog), Ctrl+O, drops on the window and the zone, `avloop.exe <file>`; the
   probe on a worker; toasts; the frame shown (until the canvas); the status bar each second; memory pressure; the
   walk's `open` step and media probes. `src/app/main.cpp` — `--mem-log`, stale sessions removed at start.
7. `src/cli/CliMedia.cpp` — `avloop-cli probe` (JSON) and `frame --at N -o x.png [--hw] [--size] [--max]`.
8. `tests/test-probe.cpp` (11), `test-decoder-seek.cpp` (7), `test-memory.cpp` (9), `tests/Samples.h` (the burned
   counter's reader); `tools/media/make-samples.mjs` (40 samples + the Bengali path + a broken file).
9. CI: actions bumped (checkout v7, setup-node v7 with Node 22, cache v6); the samples made before the tests.

## Decisions recorded

- **D-002** — the media as delivered; spec §0. **AVADR-0002** accepted.
- **R-15** — an animated WebP is refused in plain words: FFmpeg 7.1 cannot decode it, and neither can 8.1 (downloaded
  and tried on the same file: "image data not found", width 0); a decoder of our own was not asked for. Staying on 7.1.
- **R-16** — the working size before a crop: the display size, long side ≤ 1920.
- **The burned counter**: the counter samples carry 16 black/white stripes (bit k = stripe k) under testsrc2's own
  digits, so a decoded frame reads its own number without OCR — "the counter burned into testsrc2" of spec §7.3.
- **The decode memory walk** is two legs: `test-memory` (600 random seeks into a 256 MB cache, Private Bytes after
  warm-up) and the GUI open cycle under `--mem-log` (three rounds of four videos, round peaks compared).
- **Files beyond the §4 tree**: `DecoderImpl.h` (the decoder's state shared by its three files), `DiskCache.h`,
  `DecodeWorker.*` (the decoder's thread, spec §3.5), `SystemMemory.*` (§3.7.7's status line), `core/Rational.h`,
  `cli/CliMedia.cpp`, `tests/Samples.h`.

## Specification delta

- §0: `D-002` — the media.
- `docs/lore/avloop.md`: eight new rules (TS/PS rewind by byte; keyframes unflagged only after a rewind; buffersrc colour
  properties; bwdif's halved time base; animated WebP; Qt's log without a console; heredocs and backslashes; memory
  walks compare like with like).

## Verification

| Command | Result |
| --- | --- |
| `scripts\build.ps1` (Release) | built, **0 warnings** |
| `ctest --test-dir build --output-on-failure --timeout 300 -R avloop` | **5 passed** (theme 9, icons 6, probe 11, decoder-seek 7, memory 9 checks), 0 failed, 50.7 s |
| test-decoder-seek | 300 frames sequential, counter = index; **200 random seeks: 0 wrong** in software (640p, 138 real seeks) and **0 wrong with D3D11VA** (1080p, 133 real seeks, hardware on); VFR by PTS 100/100; −1 s → frame 0, 99 s → frame 299 |
| test-memory | cache never over budget (400 × 1 MB into 256 MB → 256 kept, oldest gone); halve → ≤ 128 MB; spill 300 frames → read back from disk; session folder gone with the cache; stale session removed; **decode 300 random seeks: Private Bytes +15 MB (software), +0 MB (D3D11VA) after warm-up**, cache 253/256 MB |
| `scripts\format.ps1 -Check` | PASS: 53 files |
| `node tools/icons/check-icons.mjs` | PASS: 43 icons |
| `node tools/check-secrets.mjs` | PASS: 210 files |
| `node control/phase-tools/check-loc.mjs` | PASS: 83 code files, the largest `src/ui/MainWindow.cpp` 325 lines |
| AV-001's CI run (40240619068) | green, 2 m 29 s; annotation: Node 20 actions deprecated → actions bumped here |
| `git push origin main`; `gh run watch` | recorded at AV-003's close if red |

## The walk

```
node control/lab/walk-phase.mjs execution/AV-002-walk walk-import.mjs walk-seek.mjs walk-open.json walk-memory-decode.mjs
walk-import.mjs (31 s): every file of samples/ through avloop-cli — 39 open, first/middle/last frames decoded; audio-only,
  broken (truncated MP4: "moov atom not found") and the animated WebP refused with their reasons. The special cases:
  rotate90 360×640, mean 1.38 from ffmpeg's own autorotated frame; sar43 854×480; interlaced frame 24 mean 1.34 from
  ffmpeg's bwdif, 2.40 from the woven fields; HDR mean 0.96 from ffmpeg's zscale+hable frame, 3.60 from an untone-mapped
  decode; alpha corner 0 / centre 255 (VP9 via libvpx, ProRes 4444); 4K → 1920×1080; HW on/off frame 200: identical
  (mean 0.000, D3D11VA used).
walk-seek.mjs (24 s): 40 cold seeks of counter-1080p30 through avloop-cli, each a new process: 40/40 exact.
walk-open.json (10 s, real window): open counter-640p30 → frame 640×360, status "counter-640p30.mp4 · 640×360 · 30.00 fps ·
  H.264", "GPU: NVIDIA GeForce RTX 2060 SUPER · HW decode on", "RAM 131 MB · VRAM 16 MB · Cache 0.0/1.5 GB"; broken.mp4 →
  toast "Cannot open broken.mp4: Invalid data found when processing input …", the video stays; audio-only → "This file
  has no video AvLoop can decode (container: QuickTime / MOV)."; rotate90 → 360×640; the Bengali path opens and shows
  "ভিডিও.mp4" in the status; alpha WebM → HW decode off (libvpx), "· alpha"; HDR → "· HDR → SDR".
walk-memory-decode.mjs (27 s): 3 rounds × 4 videos (1080p, 4K, HDR, alpha) under --mem-log: Private Bytes round peaks
  254 / 270 / 260 MB (+16 MB), VRAM 305 / 305 / 305 MB (+0).
No avloop process, session cache or temp file left (PHASE WALKS PASSED).
```

## The pictures

| Picture | What | What it shows — and what was seen in it |
| --- | --- | --- |
| `AV-002-walk/open-counter.png` | the window with a video open | the 640×360 testsrc2 frame 0 centred in the canvas area, its stripes at the foot all black (0); the status bar's three readings |
| `AV-002-walk/open-broken-toast.png` | the refusal | the error toast (red edge) bottom-right with the reason; the frame of before still shown |
| `AV-002-walk/open-rotate90.png` | a video with display rotation 90° counter-clockwise | portrait 360×640: the bars run across, the counter strip (all black at frame 0) stands on the right edge and testsrc2's clock reads upward at the bottom-left — turned as the metadata asks, as ffmpeg's autorotate turns it (mean 1.38) |
| `AV-002-walk/open-hdr.png` | HDR → SDR | the bars in full, natural colours after tone mapping (no washed-out grey); the moving diagonal bar shows block artefacts of the 187 KB HEVC sample itself |
| `AV-002-walk/interlaced.mpg-middle.png` | frame 24 of the 25i sample (testsrc2's 50p frame 48) | the diagonal bar's edges smooth — no comb teeth |
| `AV-002-walk/sar43.mp4-first.png` | SAR 4:3 | 854×480: the bars and the checkerboard widened by 4/3 — square pixels |
| `AV-002-walk/alpha-vp9.webm-*.png`, `prores4444.mov-*.png` | alpha | corner pixels alpha 0, centre 255 (read by av-lab) |
| `AV-002-walk/rotate90.mp4-*.png`, `hdr-pq.mkv-*.png`, `uhd-4k.mp4-*.png` | first/middle/last | as above; the 4K frames 1920×1080 |
| `docs/screenshots/main-window.png` | README | a soft orange/green gradient video open in the Navy window (295 KB) |

Weak: the frame preview is a scaled picture until AV-003's GL canvas; it does not follow a window resize.

## The regressions

| Walk of before | Result | Mended |
| --- | --- | --- |
| `AV-001-walk` walk-shell (json + mjs) | not re-run as a set; its checks are unchanged code; the gates and ctest's theme/icons pass | none |

## External freshness

- FFmpeg n7.1.5 LGPL (unchanged); FFmpeg n8.1 LGPL (BtbN `latest`, 2026-10-25) downloaded once to test animated WebP,
  then deleted — not used.
- GitHub Actions: checkout v7.0.1 (v7.0.0 2026-06-18), setup-node v7.1.0 (v7.0.0 2026-07-14), cache v6.1.0 (v6.0.0
  2026-06-23), install-qt-action v4.5.0 — read 2026-10-26 by `gh api`.
- The lab's ffmpeg on PATH: `N-120424-g03b9180fe3` (GPL, winget) — sample making only.

## Files and line counts

| File | Before | After |
| --- | --- | --- |
| `src/ui/MainWindow.cpp` | 301 | **325** |
| `src/media/Decoder.cpp` | — | **251** |
| `src/media/MediaProbe.cpp` | — | **198** |
| `src/ui/MainWindowActions.cpp` | — | **197** |
| `src/media/SourceNormalizer.cpp` | — | **184** |
| `src/media/DiskCache.cpp` | — | **162** |
| `src/media/FrameCache.cpp` | — | **149** |
| `tests/test-memory.cpp` | — | **137** |
| `src/app/main.cpp` | 105 | **135** |
| every other new file | — | under 135 |

## Recovery log

Attempt 1 RESOLVED — gate: walk-import; causes: (a) MPEG-TS/PS "seek to the start" landed after the only IDR — byte
rewind + wait for a key packet; (b) the wait concluded "keys unflagged" after a mid-GOP seek — only after a rewind;
(c) zscale "no path between colorspaces" and bwdif's property change — buffersrc colour parameters; (d) bwdif's output
pts in a halved time base — rescaled. Result: every family decodes.
Attempt 2 RESOLVED — gate: walk-import's own checks; cause: weak metrics (whole-frame combing, a fixed HDR threshold);
repair: comparison with ffmpeg's own bwdif and tone-mapped frames (must match) and the raw decode (must differ).
Attempt 3 RESOLVED — gate: walk-memory-decode; cause: compared a peak against one arbitrary moment; repair: round
peaks against round peaks. (Distinct causes, not three attempts on one failure.)

## Deviations from the phase contract

- **Code budget**: 600 planned; ~2,610 new lines (media ~1,900 with tests and the sample tool) + ~140 changed. The
  contract's **Do** lists the probe, the decoder with seek and D3D11VA, the normalizer, both caches, the window's open
  path, the CLI and three tests; the named split (AV-002a: disk cache + D3D11VA) would move ~400 lines, not the bulk.
  Recorded instead of split.
- **Animated WebP** (§3.9's list) is refused, not opened — R-15.

## Blocked legs (if any)

None.

## Staged paths

- `.github/workflows/ci.yml`, `README.md`, `docs/SPEC.md`, `docs/lore/avloop.md`, `docs/screenshots/main-window.png`,
  `src/`, `tests/`, `tools/media/make-samples.mjs`, `tools/walks/readme-shots.json`, `phases/26-10-26-034934-AvLoop/`.

## Next

AV-003 — The canvas and the Selection, promoted to IN_PROGRESS in the same turn. No avloop process, session cache or
walk temp file left.
