# Autonomous State

Program: AVLOOP
Programme version: 1.0.0
State: **ACTIVE 2026-10-29** — the owner's late wishes (items 20 … 27): AV-022 … AV-031, confirmed by the owner's
"commit, push and start and go till end of all phases".
Was: **PLANNED 2026-10-29, awaiting the owner's confirmation** — items 20 … 27 shaped as AV-022 … AV-031.
Was: **COMPLETE 2026-10-27** — every phase DONE (AV-021, the owner's late item 19; `v1.1.0` released, R-74).
Was: **ACTIVE 2026-10-27** — the owner's late wish (item 19): AV-021, the release 1.1.0.
Was: **COMPLETE 2026-10-27** — every phase DONE (AV-020, the owner's late item 18; no Release, R-73).
Was: **ACTIVE 2026-10-27** — the owner's late wish (item 18): AV-020, the colour theme.
Was: **COMPLETE 2026-10-27** — every phase DONE (AV-019, the owner's late item 17; no Release, R-70).
Was: **ACTIVE 2026-10-27** — the owner's late wish (item 17): AV-019, the clip effects.
Was: **COMPLETE 2026-10-27** — every phase DONE (AV-018, the owner's late item 16; no Release, R-65).
Was: **ACTIVE 2026-10-27** — the owner's late wishes (item 16): AV-018, the wheel and the keyframe copy.
Was: **COMPLETE 2026-10-26** — every phase DONE, `v1.0.1` released (AV-017, the owner's late item 15).
Was: **ACTIVE 2026-10-26** — the owner's late wish (item 15): AV-017, the name and the About box.
Was: **COMPLETE 2026-10-26** — every phase DONE, `v1.0.0` released (AV-016).
Was: **STARTED 2026-10-26** on the owner's specification (*AvLoop — Software Specification*, Oct 26, 2026; its §2.8
"The go-ahead is given") and the owner's word "start". Was: **PLANNED 2026-10-26, 03:49 (Asia/Dhaka).** The owner's
words: *"I want to build a piece of software called AvLoop. Its main job is to take any video, cut it at a specific
In/Out and screen area, and create a loop video of that size. … The point is that a resized WebM is exported that
feels like a loop."* — with the ten decisions and the five later words of `README.md` §1. Fourteen items, two tracks,
a checkpoint after each.
Active phase: AV-024
Next eligible: AV-024 (in progress)
Completed: 23 of 31
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
- Create the GitHub Release `v1.0.0` at CP-B1: **decided by the agent** (R-5); `v1.0.1` at AV-017 likewise (R-62); `v1.1.0` at AV-021 on the owner's word (R-74).
- Item 15 (2026-10-26, after the close): the owner's words; the owner's contact made public in the About box — the
  owner told so before the push, the next word "continue" (`OWNER-ACTIONS.md` 33).
- **Not given, and never taken**: code signing; a force-push; a secret, personal path, email or token in the
  repository; anything outside the repository's working copy, the session's temp and `%LOCALAPPDATA%\AvLoop` /
  `%APPDATA%\AvLoop` but the tools' installs.

## Waiting legs

What a walk leaves for a hand, an eye or the owner's deed. Written by the phases as they close.

| Leg | What to do | What to see | Why it waits |
| --- | --- | --- | --- |
| AV-021: 1.1.0 over the owner's own 1.0.1 | Run `AvLoop-1.1.0-setup.exe` (the Release) with AvLoop closed | Apps & features 1.1.0; About "Version 1.1.0"; settings, presets and projects kept | the owner's own install; a walk's install would take over its entry (R-75) |

## The lab here

This PC: Windows 11 Pro; PowerShell 5.1 and Git Bash; Visual Studio 2022 Community 17.14 (v143) and Build Tools; CMake
4.4.0; Ninja of Visual Studio's CMake; Qt 6.8.3 `msvc2022_64` (aqtinstall, outside the repository); Node 24.12; Python
3.14; `gh` 2.103.2; Inno Setup 6; the system ffmpeg on PATH is a GPL build used only as a lab tool, the app's FFmpeg is
the fetched LGPL build; an NVIDIA GeForce RTX 2060 SUPER; 12 logical cores; 32 GB RAM. The owner's desktop session is
where `--shot` and `--walk` open their window for seconds and close it.

## Completion record

**The programme, complete (2026-10-26).** All fourteen items of the specification are built, in sixteen phases, each
closed with its gates, its own walk and its record, and pushed with CI green; the two checkpoints walked every
situation (S1–S13 on a fresh build at CP-A1; S1–S15 on the installed 1.0.0 at CP-B1). AvLoop 1.0.0: any video in;
In/Out and a rectangle, rounded or ellipse crop with feather; the GPU canvas and the Selection; the frame-accurate
timeline with playback; four loop modes, Speed and three FPS methods; one FrameRenderer for preview and export (GPU
with a CPU twin); the Preview window, Live and Encoded; VP9/VP8 alpha WebM checked after writing, Target size, the
estimate; GIF, APNG, MP4 H.264 (Media Foundation) and MOV ProRes 4444; the batch queue; keyframes; the Loop Finder;
projects, autosave and recovery, presets, Settings, logs and minidumps; `avloop-cli`; the Inno Setup installer with
the `.avloop` association; the GitHub Release `v1.0.0` with the installer and its SHA-256.

**Item 15, late (AV-017, 2026-10-26).** The owner's wish after the close: the app is shown as "AvLoop by Rubel" — the
window, the About box, avloop.exe's details, the installer and its shortcuts — while its folders and association keep
"AvLoop"; the About box gives the developer and the ways to reach them. Version 1.0.1, the Release `v1.0.1`.

**Item 16, late (AV-018, 2026-10-27).** The owner's two wishes of one sitting: the canvas's wheel zooms about the
pointer (Ctrl or not; Shift+wheel pans across), and an Alt+drag on the timeline copies a keyframe in one Undo step.
No new version: main runs ahead of `v1.0.1`.

**Item 17, late (AV-019, 2026-10-27).** The owner's wish for clip effects: a Clip effects section — eighteen effects
of light, colour, looks and detail with On and Reset — applied to every source frame before it is placed, the same on
the canvas, in the Preview, in every export and in avloop-cli (the GPU and the CPU given the same frame); the project
file's version 2. Mended on the way: a crash at quit with the Preview open.

**Item 18, late (AV-020, 2026-10-27).** The colour theme: Replace colour (a colour of the clip — picked from it by
the eyedropper — turned into another, an orange made red), Colourise, Split toning and Duotone, in the Clip effects
section; the file still version 2.

**Item 19, late (AV-021, 2026-10-27).** Version 1.1.0 — main since `v1.0.1`, items 16 … 18 — and the GitHub Release
`v1.1.0` with `AvLoop-1.1.0-setup.exe` and its SHA-256; the README's Download link goes to it.

Readings taken: R-1 … R-76 (`README.md` §3; `DECISIONS.md`; `OWNER-ACTIONS.md`) — every one open for the owner's
veto. Dropped: nothing the specification names. Not done, and why: OBS itself was never driven (S11's second half needs
obs-websocket, not on this PC; the libvpx half passes); no code signing (§1.3, the owner's word). `BLOCKED_EXTERNAL`:
none. Waiting legs: one — the owner's own 1.0.1 upgraded to 1.1.0 (AV-021, R-75).

New records are written here, newest first. Each phase's full record is its own `execution/<phase>-EXECUTION.md`.
- **AV-023** DONE 2026-10-29 — The inverse selection (late, item 21): Inverse on the crop — the whole video kept, the area a hole, the feather outward; file version 3; walks inverse and inverse-render PASSED, five earlier walks again Record: `execution/AV-023-EXECUTION.md`.
- **AV-022** DONE 2026-10-29 — The clip as cut (late, item 20): Mode None — In to Out as cut, no loop, no crossfade; Clip in the panel, no seam in the Preview, _clip_ in the name; walks cut and cut-render PASSED, AV-006/008/009 again Record: `execution/AV-022-EXECUTION.md`.
- **AV-021** DONE 2026-10-27 — The release 1.1.0 (late, item 19): version 1.1.0 from CMakeLists.txt alone; the Release v1.1.0 with AvLoop-1.1.0-setup.exe and its SHA-256; walk release PASSED, AV-017's About again; no install over the owner's own AvLoop Record: `execution/AV-021-EXECUTION.md`.
- **AV-020** DONE 2026-10-27 — The colour theme (late, item 18): Replace colour with an eyedropper (orange → red), Colourise, Split toning, Duotone; walks theme and render PASSED, five earlier walks again Record: `execution/AV-020-EXECUTION.md`.
- **AV-019** DONE 2026-10-27 — The clip effects (late, item 17): a Clip effects section — light, colour, creative, detail — on the canvas, in the Preview and every export, GPU = CPU; the file's version 2; walks effects and render PASSED, eleven earlier walks again Record: `execution/AV-019-EXECUTION.md`.
- **AV-018** DONE 2026-10-27 — The wheel and the keyframe copy (late, item 16): the wheel zooms the canvas about the pointer; Alt+drag copies a keyframe, one Undo step; walks wheel and key-copy PASSED, AV-003 and AV-011 again Record: `execution/AV-018-EXECUTION.md`.
- **AV-017** DONE 2026-10-26 — The name and the About box (late, item 15): AvLoop by Rubel in the app, its exe and its installer; About with the developer's contact; 1.0.1; walks about and name PASSED, the old name's walks again Record: `execution/AV-017-EXECUTION.md`.
- **AV-016** DONE 2026-10-26 — CP-B1: everything walked on the installed 1.0.0 — unit, ASan, Debug leaks, every walk, the memory walk, GPU off, S15; the Release v1.0.0 Record: `execution/AV-016-EXECUTION.md`.
- **AV-014** DONE 2026-10-26 — A Windows installer: Inno Setup 6, per user or every user, the .avloop association, a clean uninstall Record: `execution/AV-014-EXECUTION.md`.
- **AV-013** DONE 2026-10-26 — GIF, APNG, MP4 H.264 and MOV ProRes 4444 besides WebM; a batch queue that survives a restart Record: `execution/AV-013-EXECUTION.md`.
- **AV-012** DONE 2026-10-26 — The Loop Finder: fingerprints, every pair scored in parallel, the best five, the window and avloop-cli find Record: `execution/AV-012-EXECUTION.md`.
- **AV-011** DONE 2026-10-26 — The keyframes: K, the Keyframe tool, the keyframe row, Linear/Ease, the renderer interpolating Record: `execution/AV-011-EXECUTION.md`.
- **AV-015** DONE 2026-10-26 — CP-A1: track A walked whole on one fresh build — unit, ASan, Debug leaks, every walk, the memory walk, GPU off, README pictures Record: `execution/AV-015-EXECUTION.md`.
- **AV-010** DONE 2026-10-26 — The project and the settings: .avloop UX, autosave and recovery, Settings, presets, logs and minidumps, F1 Record: `execution/AV-010-EXECUTION.md`.
- **AV-009** DONE 2026-10-26 — The export: VP9/VP8 alpha WebM in the background, .part and rename, checked; Output and Quality panels, Target size, the estimate, avloop-cli export Record: `execution/AV-009-EXECUTION.md`.
- **AV-008** DONE 2026-10-26 — The Preview window (D-008): F5 opens one window showing only the export — Live from the FrameRenderer (= avloop-cli frame byte for byte) with a bounded cache that waits instead of dropping, Encoded through a temporary WebM played by libvpx with its size, seam marks, Around seam, backgrounds, zoom; the VP9/VP8 alpha encoder shared with the export (R-14, R-32); one render thread and GPU context (R-33); 17 test suites, walk-preview 87 steps and walk-preview-check PASSED (six opens +3.2 MB). Record: `execution/AV-008-EXECUTION.md`. Record: `execution/AV-008-EXECUTION.md`.
- **AV-007** DONE 2026-10-26 — The renderer (D-007, AVADR-0001 accepted): one FrameRenderer for every output frame — a GPU back end (OpenGL 3.3, one output pass, texture pool, double-PBO readback) and a CPU twin within 2/255 (measured mean ≤ 0.075, max 1) — avloop-cli frame/frames for projects; video opens no longer leak (shared D3D11 device, one-thread filter graphs, persistent worker threads) and HW = SW frames again; 16 test suites, walk-render and walk-memory-render PASSED, every earlier walk re-run. Record: `execution/AV-007-EXECUTION.md`. Record: `execution/AV-007-EXECUTION.md`.
- **AV-006** DONE 2026-10-26 — The loop engine (D-006): LoopMap and TimeMap as tested pure functions for the four modes and three FPS methods, Speed, the output frame rate (R-28, R-29), the Motion Interpolation pre-pass with progress and cancel (R-30), the Loop panel with its summary and the Output FPS row, the crossfade band dragged on the timeline; 14 test suites, walk-loop 74 steps PASSED. Record: `execution/AV-006-EXECUTION.md`. Record: `execution/AV-006-EXECUTION.md`.
- **AV-005** DONE 2026-10-26 — The timeline (D-005): the output frame grid and timecode, In/Out by keys, tools and drags (R-24), the ruler with labels that never overlap, thumbnails on their own decoder, the transport and TimeEdit, playback decoding ahead into a ring with drops counted (R-25, R-26), the scrub's two speeds (R-27); 11 test suites, walk-timeline 153 steps and walk-playback (0 dropped at 30/60 fps, memory flat) PASSED. Record: `execution/AV-005-EXECUTION.md`. Record: `execution/AV-005-EXECUTION.md`.
- **AV-004** DONE 2026-10-26 — The crop tools (D-004): Rect/Rounded/Ellipse drawn, moved and resized with ratios (Custom in lowest terms), Shift/Alt/Ctrl, snapping, arrows, Esc, Delete, double-click; the dim and outline overlay; MaskBuilder (1 px edge, Feather inward, cached); options and Crop panel with Alpha Auto and the transparency notices; 10 test suites, walk-crop 177 steps PASSED. Record: `execution/AV-004-EXECUTION.md`. Record: `execution/AV-004-EXECUTION.md`.
- **AV-003** DONE 2026-10-26 — The canvas and the Selection (D-003): the OpenGL 3.3 canvas with the shared transform shader, Zoom/Hand, the Selection with handles and options bar, StepSpinBox, the model with Undo (200 steps, merging), .avloop v1 read/write; lab walks never take focus; 8 test suites, walk-selection 75 steps PASSED. Record: `execution/AV-003-EXECUTION.md`.
- **AV-002** DONE 2026-10-26 — The media (D-002, AVADR-0002): any video opened by content, frame-accurate decoding with D3D11VA and software fallback, normalization (rotation, SAR, bwdif, HDR tone-map, alpha), the bounded RAM cache with disk spill, status bar and --mem-log, avloop-cli probe/frame, 40 samples; 5 test suites, 4 walks PASSED (200 seeks exact sw+hw, memory +16 MB). Record: `execution/AV-002-EXECUTION.md`.
- **AV-001** DONE 2026-10-26 — The ground (D-001): the CMake/MSVC/Qt 6.8.3/FFmpeg 7.1.5 LGPL build with its gates, the maximized shell with the §5 layout, Navy and Black from one token table, 43 own icons and avloop.ico, --shot/--walk, avloop-cli version (licence gate), CI, open-source files, README v1; 2 test suites, walk-shell PASSED. Record: `execution/AV-001-EXECUTION.md`.
