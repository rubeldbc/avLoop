# 02 Major checkpoints

Canonical current execution state: `../AUTONOMOUS_STATE.md`

Two checkpoints: CP-A1 (AV-015) after track A, CP-B1 (AV-016) after track B. Every phase runs the gates its change
touches **and its own walk**; a checkpoint runs the set below and walks every situation on one fresh Release build.
Every result is written in the execution record with its observed value; every command carries a five-minute timeout
(spec §14).

## 1. The gates

| Gate | Command / measure | When |
| --- | --- | --- |
| **Line limit** | `node control/phase-tools/check-loc.mjs` PASS: no code file > 600 | Every commit |
| Build | `scripts\build.ps1` (Release), zero warnings | Every phase |
| Unit | `ctest --test-dir build -C Release --output-on-failure --timeout 300`, all pass | Every phase |
| Form | `scripts\format.ps1 -Check` (clang-format), `node tools/icons/check-icons.mjs` | Every phase |
| Secrets | `node tools/check-secrets.mjs`: no tokens, emails or personal paths | Every commit |
| Licence | The bundled FFmpeg DLLs have no `--enable-gpl` (`avloop-cli version` reads `avcodec_configuration()`) | AV-001, checkpoints |
| Memory | The §3.7 walk: Private Bytes growth < 50 MB, VRAM < 32 MB; zero Debug leaks | AV-002, AV-007, AV-008, AV-009, checkpoints |
| ASan | All unit tests in a `/fsanitize=address` build | Checkpoints |
| GPU parity | Same project GPU vs CPU: mean difference ≤ 1.0, maximum ≤ 4 | From AV-007 |
| CI | GitHub Actions green (`gh run watch`) | Every push |
| README | Screenshots are of the current UI (`--walk tools/walks/readme-shots.json`); the feature list is true | Every phase that changes the UI |
| Phase system | `validate-phase-system.mjs` PASS, `check-continuity.mjs` | Every phase |

## 2. The situations

| S | Situation | What must happen |
| --- | --- | --- |
| S1 | The app | Second start < 2 s; maximized; screenshots of the Navy and Black themes at 100/150/200% DPI; all icons visible; only Open/Preview/Export buttons in the top bar |
| S2 | Import | Every file from `make-samples` opens; rotation, SAR, interlacing and HDR come out upright/correct (checked on first/middle/last frame PNGs); a Bengali path works; a broken file gives a message, not a crash; HW decode on and off give the same frames |
| S3 | Seek | Correct frame over 200 random seeks (testsrc2 frame number); 1080p H.264 at 30/60 fps plays 10 s with 0 dropped frames |
| S4 | Selection | Moving/scaling the video, Fit/Fill/Center/Flip; options bar values match; upscale warning; the crop stays still |
| S5 | Crop | All three shapes, every ratio, Shift/Alt, snapping, arrows; in the export, Ellipse corner pixels have alpha = 0 and the centre alpha = 255; with Feather, the edge steps down gradually |
| S6 | Timeline | In/Out via the tool, the shortcut and dragging; dragging the crossfade band changes F; no label overlap at any zoom |
| S7 | Loop modes | In all four modes the exported frame count = L from §8.2; `seamDiff` (last→first frame difference) ≤ 1.5 × the median difference between neighbouring frames; no duplicate frame at Ping-pong turns |
| S8 | FPS and Speed | 15/24/30/60 and Custom 12; all three methods; `r_frame_rate` correct in ffprobe, CFR; correct length at Speed 0.5× and 2× |
| S9 | Preview | Shows only the exported part; Live frame N = exported frame N (mean difference ≤ 1.0, excluding compression); Encoded keeps alpha; memory returns on close |
| S10 | Export | ffprobe: `codec_name=vp9`, W×H, `alpha_mode=1` (when Alpha is On), no audio stream, first frame is a keyframe; alpha plane present via the libvpx decoder; no `.part` after Cancel; Target size within ±5%; VP8 the same |
| S11 | OBS compatibility | The exported file read twice in a loop through FFmpeg's libvpx decoder (`-stream_loop 1`) gives 2L frames and no error at the seam; if OBS is installed, play it in a Media Source via obs-websocket and confirm transparency in a screenshot |
| S12 | Project | Save → open gives the same render (identical pixels); moving the source triggers relativePath/Locate; Undo/Redo for every command; autosave recovery after a crash |
| S13 | Memory and GPU | The §3.7 and §3.6 walks; the whole set again with the GPU off |
| S14 | Keyframes, Loop Finder, Batch, formats | Positions between keyframes as expected; on a known loop (a generated sample repeating every 4 s) the Finder ranks it first (±2 frames); a 3-entry Batch; GIF/APNG/MP4/ProRes correct in ffprobe |
| S15 | Installer | Silent install (`/VERYSILENT`), launch, `.avloop` opens on double-click, uninstall leaves nothing behind (except settings) |

No walk leaves a test process, temp file or cache behind; screenshots and logs stay in `execution/AV-NNN-walk/`, each
with a description of what it shows.

## 3. Which phase walks which situation first

| S | First walked | Again at |
| --- | --- | --- |
| S1 | AV-001 | AV-015, AV-016 |
| S2 | AV-002 | AV-015, AV-016 |
| S3 | AV-002 (seek), AV-005 (playback) | AV-015, AV-016 |
| S4 | AV-003 | AV-015, AV-016 |
| S5 | AV-004 (canvas, mask), AV-009 (export alpha) | AV-015, AV-016 |
| S6 | AV-005 (In/Out, labels), AV-006 (band) | AV-015, AV-016 |
| S7, S8, S10, S11 | AV-009 | AV-015, AV-016 |
| S9 | AV-008 | AV-015, AV-016 |
| S12 | AV-010 | AV-015, AV-016 |
| S13 | AV-007 (render), AV-008, AV-009 (memory legs) | AV-015, AV-016 |
| S14 | AV-011, AV-012, AV-013 | AV-016 |
| S15 | AV-014 | AV-016 |
