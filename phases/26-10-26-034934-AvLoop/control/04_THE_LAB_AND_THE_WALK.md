# 04 The lab and the walk

Canonical current execution state: `../AUTONOMOUS_STATE.md`

How a phase sees its **Done when** on this PC. AvLoop's lab is its own binaries and FFmpeg's readers (spec §2.6): the
headless `avloop-cli`, the GUI's `--shot` and `--walk`, ffprobe and ffmpeg, the sample videos `make-samples.mjs`
makes, and `control/lab/av-lab.mjs`. Every command carries a five-minute ceiling; a walk leaves no process, session
cache or temp file behind (`walk-phase.mjs` checks); pictures and logs stay in `execution/AV-NNN-walk/`.

## 1. The kits

| File | What it gives |
| --- | --- |
| `avloop-cli` (`src/cli/main.cpp`) | `version` (FFmpeg's configuration; refuses `--enable-gpl`), `probe <video>` (MediaProbe's JSON), `frame <video\|project> --at N -o x.png [--cpu]`, `export <project.avloop> -o <out>`; grown by the phases that build each |
| `avloop.exe --shot <file.png> [--project p] [--tool T] [--theme navy\|black]` | opens the main window maximized, waits until it is idle, grabs it by `QWidget::grab` (GL included), exits |
| `avloop.exe --walk <script.json> --walk-out <folder>` | replays steps on real widgets (QTest's mouse and keys), takes pictures, prints `PASS`/`FAIL` per `expect` and `WALK PASSED`/`WALK FAILED`, exits; `--mem-log <file>` beside it |
| `tools/media/make-samples.mjs` | the sample videos (§2) into `samples/` (git-ignored), by the system ffmpeg (a lab tool) |
| `control/lab/av-lab.mjs` | `probe(file)`, `alphaOf(file)`, `frameAt(file, n)`, `framesSmall`, `meanDiff`, `seamDiff(file)`, `pixelAt(png, x, y)`, `cli(args)`, `app(args)`, `avloopProcesses()`, `cacheSessions()`, `walkLog()` |
| `control/lab/walk-phase.mjs` | a phase's walks in one run; the leftovers' check; `walk-phase.log` |
| `tools/walks/*.json` | the GUI walks kept with the product (`readme-shots.json` makes `docs/screenshots/`) |

## 2. The samples (`make-samples.mjs`)

`testsrc2` with its frame counter (H.264 MP4, 1080p30 and 1080p60, 10 s), rotation 90° (display matrix), VFR, HDR-tagged
(HEVC 10-bit BT.2020 PQ), interlaced (MPEG-2 TFF), SAR 4:3 (anamorphic), alpha WebM (VP9 `yuva420p`), ProRes 4444 with
alpha, 4K (3840×2160), a Bengali-named path, a broken file (truncated), an audio-only file; at least one of each §3.9
family (MKV, AVI, WMV/ASF, FLV, MPEG-TS, MPEG-PS, MXF, 3GP, OGV/Theora, Y4M, animated GIF/APNG/WebP, MJPEG, DNxHD,
FFV1, QuickTime Animation, PNG-in-MOV, uncompressed, HAP and CineForm where the encoder exists); a loop that repeats
every 4 s (AV-012). Small (≤ 10 s, mostly ≤ 640 px) so the set stays under ~200 MB; never committed.

## 3. What a walk reads

- **The GUI**: a `--walk` script's steps act on widgets by `objectName` (`tool.crop-ellipse`, `canvas`,
  `timeline`, `options.scale` …) with QTest's mouse and keys — real events through the real handlers, never a setter;
  `expect` steps read the model and the widgets' shown values; `shot` steps grab pictures. A spinner typed is read
  back before it is believed.
- **The output**: ffprobe's fields (codec, size, `r_frame_rate`, `nb_read_frames`, `alpha_mode` tag, streams), the
  frames by the libvpx decoder (`alphaOf`, `frameAt`), `seamDiff`, `-stream_loop 1` frame counts.
- **The memory**: `--mem-log` lines (Private Bytes, VRAM each second); growth measured after a warm-up.
- **The pictures**: each kept PNG described in the record — what is right, what is weak.

## 4. The rules of the walk

- One walk a process; a walk that fails says `FAIL <what>: <observed>` and `WALK FAILED`; a phase closes only on
  `PHASE WALKS PASSED`.
- A GUI walk's window opens on the owner's desktop for seconds: maximized, no modal dialog left open (a file dialog
  is never opened by a walk — files are given by `--project` or a step's `open`), closed by the walk.
- Temp files of a walk live in the walk's folder or the session's temp and are deleted by it; `samples/` is reused.
- The pictures for the README are made by `tools/walks/readme-shots.json` from `tools/walks/sample.avloop`; each PNG
  under 300 KB (PNG, 8-bit, at most 1600 px wide).
