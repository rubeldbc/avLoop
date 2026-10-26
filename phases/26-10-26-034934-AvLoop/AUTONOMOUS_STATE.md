# Autonomous State

Program: AVLOOP
Programme version: 1.0.0
State: **STARTED 2026-10-26** on the owner's specification (*AvLoop — Software Specification*, Oct 26, 2026; its §2.8
"The go-ahead is given") and the owner's word "start". Was: **PLANNED 2026-10-26, 03:49 (Asia/Dhaka).** The owner's
words: *"I want to build a piece of software called AvLoop. Its main job is to take any video, cut it at a specific
In/Out and screen area, and create a loop video of that size. … The point is that a resized WebM is exported that
feels like a loop."* — with the ten decisions and the five later words of `README.md` §1. Fourteen items, two tracks,
a checkpoint after each.
Active phase: AV-008
Next eligible: AV-008 (in progress)
Completed: 7 of 16
Start commit: the plan's commit `[AV] Plan` on `main`. Planning baseline: `d98849f` on `main` (*Initial commit*).

## Plan in one paragraph

Fourteen items, sixteen phases, two tracks. Track A makes the loop maker: AV-001 lays the ground (CMake, the fetched
FFmpeg and Qt, the maximized shell with the §5 layout, the two themes, the app's own icons, the lab's `--shot` and
`--walk`, the gates, CI and README v1); AV-002 opens any video and gives exact, normalized frames within fixed memory
(AVADR-0002); AV-003 puts it on a GL canvas the Selection moves and scales, with the model and Undo; AV-004 adds the
three crop tools and the mask; AV-005 the frame-accurate timeline, In/Out and playback; AV-006 the loop's maths (four
modes, Speed, FPS methods) and the Loop panel; AV-007 the one `FrameRenderer` shared by preview and export, GPU with
a CPU twin (AVADR-0001); AV-008 the Preview window, Live and Encoded; AV-009 the VP9/VP8 alpha export with its panels
and verification; AV-010 the project file's UX, autosave, presets, settings and logs. CP-A1 (AV-015) walks S1–S13 on
one fresh build with the memory, GPU and ASan gates. Track B adds the keyframes (AV-011), the Loop Finder (AV-012), the
batch queue and four more formats (AV-013), the installer (AV-014); CP-B1 (AV-016) walks everything installed and
publishes `v1.0.0`. The order is `control/01_PHASE_DEPENDENCY_GRAPH.md`; the shapes `control/03`, `05` … `17`; where it
is walked `control/04`.

## Grants

- Start (the word to code): **given** 2026-10-26 — the specification §2.8 and "start".
- Commit the plan and push it: **given** — spec §2.8 item 1.
- Commit per phase and push to `origin` `main`: **standing** (spec §1.1 later word 5, §2.9).
- Install the environment on this PC (CMake, Ninja, Qt, FFmpeg, Inno Setup, Node, `gh`): **given** — spec §2.8 item 5.
- Use the machine's existing git credentials for `git push` and `gh` (`GH_TOKEN` per command, never stored): **given**
  — spec §2.9 ("the machine's existing git credentials or `gh auth`"); R-12.
- Create the GitHub Release `v1.0.0` at CP-B1: **decided by the agent** (R-5).
- **Not given, and never taken**: code signing; a force-push; a secret, personal path, email or token in the
  repository; anything outside the repository's working copy, the session's temp and `%LOCALAPPDATA%\AvLoop` /
  `%APPDATA%\AvLoop` but the tools' installs.

## Waiting legs

What a walk leaves for a hand, an eye or the owner's deed. Written by the phases as they close.

| Leg | What to do | What to see | Why it waits |
| --- | --- | --- | --- |
| (none yet) | | | |

## The lab here

This PC: Windows 11 Pro; PowerShell 5.1 and Git Bash; Visual Studio 2022 Community 17.14 (v143) and Build Tools; CMake
4.4.0; Ninja of Visual Studio's CMake; Qt 6.8.3 `msvc2022_64` (aqtinstall, outside the repository); Node 24.12; Python
3.14; `gh` 2.103.2; Inno Setup 6; the system ffmpeg on PATH is a GPL build used only as a lab tool, the app's FFmpeg is
the fetched LGPL build; an NVIDIA GeForce RTX 2060 SUPER; 12 logical cores; 32 GB RAM. The owner's desktop session is
where `--shot` and `--walk` open their window for seconds and close it.

## Completion record

New records are written here, newest first. Each phase's full record is its own `execution/<phase>-EXECUTION.md`.
- **AV-007** DONE 2026-10-26 — The renderer (D-007, AVADR-0001 accepted): one FrameRenderer for every output frame — a GPU back end (OpenGL 3.3, one output pass, texture pool, double-PBO readback) and a CPU twin within 2/255 (measured mean ≤ 0.075, max 1) — avloop-cli frame/frames for projects; video opens no longer leak (shared D3D11 device, one-thread filter graphs, persistent worker threads) and HW = SW frames again; 16 test suites, walk-render and walk-memory-render PASSED, every earlier walk re-run. Record: `execution/AV-007-EXECUTION.md`. Record: `execution/AV-007-EXECUTION.md`.
- **AV-006** DONE 2026-10-26 — The loop engine (D-006): LoopMap and TimeMap as tested pure functions for the four modes and three FPS methods, Speed, the output frame rate (R-28, R-29), the Motion Interpolation pre-pass with progress and cancel (R-30), the Loop panel with its summary and the Output FPS row, the crossfade band dragged on the timeline; 14 test suites, walk-loop 74 steps PASSED. Record: `execution/AV-006-EXECUTION.md`. Record: `execution/AV-006-EXECUTION.md`.
- **AV-005** DONE 2026-10-26 — The timeline (D-005): the output frame grid and timecode, In/Out by keys, tools and drags (R-24), the ruler with labels that never overlap, thumbnails on their own decoder, the transport and TimeEdit, playback decoding ahead into a ring with drops counted (R-25, R-26), the scrub's two speeds (R-27); 11 test suites, walk-timeline 153 steps and walk-playback (0 dropped at 30/60 fps, memory flat) PASSED. Record: `execution/AV-005-EXECUTION.md`. Record: `execution/AV-005-EXECUTION.md`.
- **AV-004** DONE 2026-10-26 — The crop tools (D-004): Rect/Rounded/Ellipse drawn, moved and resized with ratios (Custom in lowest terms), Shift/Alt/Ctrl, snapping, arrows, Esc, Delete, double-click; the dim and outline overlay; MaskBuilder (1 px edge, Feather inward, cached); options and Crop panel with Alpha Auto and the transparency notices; 10 test suites, walk-crop 177 steps PASSED. Record: `execution/AV-004-EXECUTION.md`. Record: `execution/AV-004-EXECUTION.md`.
- **AV-003** DONE 2026-10-26 — The canvas and the Selection (D-003): the OpenGL 3.3 canvas with the shared transform shader, Zoom/Hand, the Selection with handles and options bar, StepSpinBox, the model with Undo (200 steps, merging), .avloop v1 read/write; lab walks never take focus; 8 test suites, walk-selection 75 steps PASSED. Record: `execution/AV-003-EXECUTION.md`.
- **AV-002** DONE 2026-10-26 — The media (D-002, AVADR-0002): any video opened by content, frame-accurate decoding with D3D11VA and software fallback, normalization (rotation, SAR, bwdif, HDR tone-map, alpha), the bounded RAM cache with disk spill, status bar and --mem-log, avloop-cli probe/frame, 40 samples; 5 test suites, 4 walks PASSED (200 seeks exact sw+hw, memory +16 MB). Record: `execution/AV-002-EXECUTION.md`.
- **AV-001** DONE 2026-10-26 — The ground (D-001): the CMake/MSVC/Qt 6.8.3/FFmpeg 7.1.5 LGPL build with its gates, the maximized shell with the §5 layout, Navy and Black from one token table, 43 own icons and avloop.ico, --shot/--walk, avloop-cli version (licence gate), CI, open-source files, README v1; 2 test suites, walk-shell PASSED. Record: `execution/AV-001-EXECUTION.md`.
