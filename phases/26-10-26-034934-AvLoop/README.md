# AvLoop — the seamless loop maker for OBS

Programme `AVLOOP`, prefix `AV`. Canonical current execution state: `AUTONOMOUS_STATE.md`.

Planned 2026-10-26 (03:49, Asia/Dhaka) from the owner's specification (`docs/SPEC.md`, a copy of *AvLoop — Software
Specification*, Oct 26, 2026). **A Windows desktop app (C++20, Qt 6, FFmpeg) that cuts a chosen time range (In to Out)
and a chosen screen area (crop) out of any video and turns it into a seamless looping WebM for OBS Studio's Media
Source.** The specification is the owner's word to code (§2.8 of it); this programme runs in autonomous mode.

## 1. The owner's words

The ask (spec §1.1, translated): *"I want to build a piece of software called AvLoop. Its main job is to take any
video, cut it at a specific In/Out and screen area, and create a loop video of that size. … The point is that a
resized WebM is exported that feels like a loop."*

Decided with the owner in discussion:

1. Windows only. Used in OBS.
2. All suggested features are kept; audio is dropped.
3. The Crop tool decides which part is exported. The Selection tool moves and scales the source video on the canvas.
4. The crossfade happens inside the selected range (its end blends into its start).
5. Tools instead of buttons wherever possible. One Preview button opens a separate window with a slider that shows
   only the exported part, exactly as it will be exported.
6. Output quality is adjustable. Maximum output is 1080p (long side 1920 px). No 4K output.
7. Several aspect ratios can be chosen while cropping. Frame rate is adjustable.
8. Compact GUI. Orange with black or navy blue theme. The app draws its own icons.
9. "Full screen" means a maximized window, not true full screen.
10. No code file over 600 lines. Phases are created following the owner's example (the CabTV AirTicker2 programme).

Added by the owner later (2026-10-26):

1. Maximum care with memory (§3.7). Use the GPU (§3.6).
2. State which libraries are used and why (§3.8). Any video must be importable (§3.9).
3. No code signing. Windows only.
4. Autonomous mode: the AI does all coding, testing, e2e and walks itself, decides on its own when stuck, and asks the
   owner nothing (§2.8).
5. Auto-push to `https://github.com/rubeldbc/avLoop` after every phase. Public repository; anyone can contribute. The
   README is always professional and up to date, with app screenshots (§2.9).

Autonomous mode, in the owner's words (spec §2.8): *"In autonomous mode, from start to finish, the AI does all coding
and testing, e2e and walks by itself. If there is a problem it solves it on its own. If it gets stuck anywhere it
makes the decision itself and gets the work done. It will not ask me anything. Once started, it finishes the whole job
by itself."* The go: **"start"**, 2026-10-26.

Added by the owner after the programme closed (2026-10-26), item 15, verbatim: *"The applicaiton name will be AvLoop
by Rubel. There will be a about section where my information will be available. You can get the information from
glayer.app which is in my repo"*; then *"Email: [the owner's address — written in
`src/ui/dialogs/AboutDialog.cpp` alone, R-61], website not required."*

Added by the owner on 2026-10-27, item 16, verbatim: *"mouse wheen move will zoom in out targeted to the mous pointer
area in canvas"*; then, while it was being built, *"alt drag will copy a keyframe in time line"*. Two wishes of one
sitting: one item, one late phase.

Added by the owner on 2026-10-27, item 17, verbatim: *"add a righ side group, Clip effects. where a lots of clip effect
could be applied to the clip to change or color correction the clip before export"*.

Added by the owner on 2026-10-27, item 18, verbatim: *"need more effect like i want to change the color theme of a clip,
say a clip is orange color but i want to change it to red color theme. this kind of effect needed"*; then *"after
completing everything push to main then forcly shutdown my pc"*.

## 2. The items

Eighteen, in two tracks; a checkpoint closes each track. The fifteenth to the eighteenth came after both had
closed: late phases.

| # | Item | What it gives |
| --- | --- | --- |
| 1 | **The ground** (AV-001) | CMake project, dependencies by script, maximized main window with the §5 layout, the Navy and Black themes, the app's own icons, the gate tools, `--shot`/`--walk`, `avloop-cli`, open-source files, CI, README v1 |
| 2 | **The media** (AV-002) | open any video by content, probe, frame-accurate decode with D3D11VA, normalize (rotation, SAR, deinterlace, HDR), the bounded frame cache with disk spill, the sample videos |
| 3 | **The canvas and the Selection** (AV-003) | the GL canvas in scene space, Zoom and Hand, the Selection with its options bar, the model, Undo |
| 4 | **The crop tools** (AV-004) | Rect, Rounded Rect, Ellipse; ratios; handles, modifiers, snapping; Feather and Radius; the mask |
| 5 | **The timeline** (AV-005) | ruler, thumbnails, In/Out, playhead, transport, playback, the I and O tools |
| 6 | **The loop engine** (AV-006) | Simple, Crossfade, Ping-pong, Smooth turn; Speed; FPS and its three methods; the Loop panel |
| 7 | **The renderer** (AV-007) | one `FrameRenderer` for preview and export, GPU with a CPU fallback of the same result |
| 8 | **The preview window** (AV-008) | Live and Encoded, the slider, seam marks, Around seam, backgrounds |
| 9 | **The export** (AV-009) | VP9/VP8 alpha WebM, Output and Quality panels, target size, estimate, verification |
| 10 | **The project and the settings** (AV-010) | `.avloop`, Undo/Redo everywhere, presets, autosave, Recent, Settings, logs |
| 11 | **The keyframes** (AV-011) | keyframes of the video's position and scale, Linear and Ease |
| 12 | **The loop finder** (AV-012) | the best In/Out pairs found automatically |
| 13 | **The batch and the other formats** (AV-013) | the export queue; GIF, APNG, MP4 H.264, ProRes 4444 |
| 14 | **The installer** (AV-014) | an Inno Setup installer with the `.avloop` association |
| 15 | **The name and the About box** (AV-017, late) | "AvLoop by Rubel" in the app, its executable and its installer; the About box with the developer and the ways to reach them |
| 16 | **The wheel and the keyframe copy** (AV-018, late) | the wheel zooms the canvas about the pointer; Alt+drag on the timeline copies a keyframe |
| 17 | **The clip effects** (AV-019, late) | a Clip effects section: light, colour, creative and detail effects on the clip, on the canvas, in the Preview and in every export |
| 18 | **The colour theme** (AV-020, late) | Replace colour (an orange made red; the colour picked from the clip), Colourise, Split toning, Duotone |

Checkpoints: CP-A1 (AV-015) walks track A on one fresh build; CP-B1 (AV-016) walks the whole, installed, and makes
the GitHub Release `v1.0.0`.

## 3. The readings taken

Each is built as read and open for the owner's veto (`OWNER-ACTIONS.md` rows 11 on; `DECISIONS.md`).

1. **R-1** UI language is English; all strings go through `tr()`, so a Bengali translation can be added later.
2. **R-2** Unlocking Selection's W/H never changes the video's ratio (distortion was not asked for).
3. **R-3** Space = Play/Pause; pan = middle mouse or H.
4. **R-4** Licence MIT (public, open to everyone's contributions).
5. **R-5** At CP-B1, a GitHub Release `v1.0.0` is created with the installer, so the README's Download link works.
6. **R-6** Default output: VP9, Medium, Alpha Auto, Crop size, Source FPS (maximum 60).
7. **R-7** Blending in premultiplied sRGB (not linear light), matching how OBS composites.
8. **R-8** One crop at a time; with no crop, the whole video.
9. **R-9** FFmpeg 7.1 is the newest **7.1** LGPL shared build BtbN still publishes — `n7.1.5` of the dated release
   `autobuild-2026-07-31-14-10` —, pinned by URL and SHA-256 in `scripts/fetch-deps.ps1`, which falls back to the newest
   `n7.1` LGPL shared asset when the pin is pruned (the `latest` release no longer carries 7.1).
10. **R-10** Qt 6.8.3 — the last open-source patch release of the 6.8 LTS — installed by `aqtinstall`; CI by
    `jurplel/install-qt-action`.
11. **R-11** The owner's original specification and mock stay where the owner put them, untracked; the repository
    holds their copies `docs/SPEC.md` and `docs/mock/avloop-gui-mock.html`.
12. **R-12** `gh` uses the token git's credential manager already holds (passed per command as `GH_TOKEN`, never
    stored or printed); the owner's machine credentials are what §2.9 names.
13. **R-13** The `.avloop` file's format is written first by AV-003 (the model's serialization, needed by every walk and
    by `avloop-cli`); AV-010 builds the file's user-facing side (Save, Open, migration, autosave, Recent).
14. **R-14** The encoder and muxer are built by AV-008 for the Encoded preview and reused by Export at AV-009.
15. **R-15** An animated WebP is refused with a plain message ("FFmpeg reads still WebP images only. Convert it to GIF,
    APNG or a video first."): no FFmpeg decodes animated WebP — 7.1 and 8.1 were both tried (AV-002) — and a decoder
    of AvLoop's own was not asked for. Every other family of §3.9 opens.
16. **R-16** Until a crop exists the working size is the display size with its long side capped at 1920 (spec §3.7.3:
    a 4K source is scaled down right after decoding); the renderer sets it from the crop and scale from AV-007.
17. **R-17** `src/core` links Qt Gui for `QUndoStack`/`QUndoCommand` only (Qt 6 moved them out of Widgets); spec §3.2's
    "Qt Core only" holds for everything else, and §11.2 names `QUndoStack`.
18. **R-18** "Reset: 100%, (0,0)" (§6.2) and "on load the video sits at (0,0)" (§3.3) put the video's top-left at (0,0)
    — the project file's example (x 960, y 540 for 1920×1080) agrees.
19. **R-19** Fit, Fill and Center with no crop act on the video's own frame at 100% (0,0,w,h): with no crop the export
    is the video rectangle itself (R-8), so there is nothing else to fit to.
20. **R-20** The video's dashed border and its handles show while the Selection is active (the owner's mock draws them
    so); the crop's outline (AV-004) shows in every tool.
21. **R-21** Custom… starts from the crop's own proportions in lowest terms (180 × 320 → 9:16; 1:1 with no crop); a W
    or H typed with a ratio set moves the other side, the top-left staying (§6.1 names no anchor for typing).
22. **R-22** The canvas shows the crop by its outline and the dim outside; the Feather's softness is seen in the
    Preview and the export, which apply the mask (§6.1's overlay list names no soft edge on the canvas).
23. **R-23** What snaps (§6.1 says "to the video's edges and centre"): a drawing's corners, a moved crop's edges or
    centre lines, a resized crop's moving edges — to the transformed video's edges and centre lines.
24. **R-24** The range covers the frames from In up to Out (§8.1's D = (Out − In) × fps): I takes the frame under the
    playhead as the first, O as the last (Out = the playhead + one frame); Go to Out shows the range's last frame.
25. **R-25** "Dropped" counts frames never shown: the clock shows frames in order, so a late screen refresh is caught
    up rather than skipped, and drops a frame only when it is more than two frames late with a newer one due.
26. **R-26** During playback the timeline and the time field follow at most 30 times a second; the canvas shows every
    frame (a raster repaint beside the GL canvas costs a whole-window composition).
27. **R-27** The scrub's two speeds (§7.2): while the pointer moves, the keyframe at or before it (the frame on screen
    when already near); after 150 ms at rest or on release, the exact frame.
28. **R-28** The default loop is Crossfade (F 0.5 s, Smooth; T 0.3 s; Speed 1; Nearest): the point of AvLoop is a
    seamless loop, and the spec's project example uses it (§8 names defaults for F, T and the method, not the mode).
29. **R-29** A new frame rate moves In and Out to the nearest frame of its grid ("seconds stay, on the grid", §8.3), in
    the same Undo step; F and T stay in seconds and their frames are worked out again.
30. **R-30** Motion Interpolation's pre-pass runs when the Preview or the export needs its frames (AV-008, AV-009),
    with their progress and cancel; AV-006 builds and tests it.
31. **R-31** The Preview's own cache holds a third of the frame cache's budget, at most 512 MB; a loop larger than that
    keeps playing through "Caching…" (§3.7.1 names no Preview limit).
32. **R-32** VP9 is written in BT.709, VP8 in BT.601 (VP8 cannot carry BT.709 and every decoder reads it as BT.601);
    the colour planes are straight, the alpha separate (WebM's alpha is unassociated).
33. **R-33** The app's output frames are made on one render thread with one GPU context that live with the window
    (the Preview now, the export at AV-009): a thread or a context made per use leaves GPU-driver memory behind (AV-007).
34. **R-34** Export always opens the Save dialog (§10.1's "the first time" read as: the dialog comes first), filled with
    the project's last export, else `<source>_loop_<W>x<H>.webm` in the last folder; a walk answers it with a path.
35. **R-35** The size estimate is measured 1 s after a change of the range, crop, transform, loop, output or quality, or
    when the Quality section is opened — not when a video is only opened, which must stay cheap (§3.7.5); a measure
    makes one frame at a time and gives VP9 no more look-ahead than its ten frames.
36. **R-36** A destination still locked after the five tries is written as `name (1).webm` at once, with a warning toast
    that says so — no question in the middle of a finished export (§12.3's "offer").
37. **R-37** Target size counts MB as 1,048,576 bytes; when the first two passes land more than 5 % off either way, one
    more pass at the corrected bitrate (aiming 3 % under when over). A target above what the loop takes at the best
    quality comes out smaller: libvpx cannot spend bits a loop does not need.
38. **R-38** The `.avloop` `output` keeps `size` (`crop`, `long-<N>`, `custom`), `w`/`h` (the Custom size), `lock`,
    `alpha` (`auto`, `on`, `off`), `background` (`#rrggbb`); `export.lastPath` the last file written.
39. **R-39** Version 1 is the first `.avloop` format: `ProjectMigrate`'s chain starts with "no version = version 1";
    a newer version is refused with §11.1's message.
40. **R-40** The built-in presets' sizes are long sides (512, 1280, 1080): a preset holds no crop, so a fixed W×H
    would stretch one; the names keep the spec's ratios as the crop to pair them with.
41. **R-41** A video open without a project file is titled by the video's name; its Save asks for a file (Save As).
42. **R-42** A lab run answers every question from its walk (`answer`), asks for the recovery offer itself
    (`recover`) and keeps its settings, autosave and logs in its own folder (`AVLOOP_DATA_DIR`).
43. **R-43** A source whose size or date changed since the save opens with a warning toast — the status bar's left
    side already shows the source, so the toast is where §11.1's warning is seen.
44. **R-44** Settings is a window whose changes are saved at once (no OK/Cancel): the theme and the RAM cache apply
    at once, decoding and GPU rendering from the next video, the disk cache from the next start (said in the tab).
45. **R-45** §3.7.8's "zero CRT leak report in Debug" is checked as no live debug-heap block left by repeated cycles of
    each subsystem (`test-leaks`): an exit-time report would list Qt's own process-lifetime allocations.
46. **R-46** Under AddressSanitizer the process-memory growth thresholds are reported, not judged (ASan's quarantine
    keeps freed memory); they are judged in the Release run. §3.7.8's memory walk: after warm-up — a minute of playback,
    the first export, the first Preview — phase against phase.
47. **R-47** Flips are not keyframed: they hold (the one transform's flips apply at every time); a flip made with
    keyframes changes them, not the keyframes.
48. **R-48** The keyframe the options and Delete act on is the one at the playhead (shown solid); a diamond clicked takes
    the playhead to it. Deleting the last keyframe leaves the video where it was shown.
49. **R-49** Without a crop the export's frame is the video where it stands at In; Alpha Auto looks at every keyframe
    (between two, the video never goes beyond them).
50. **R-50** The Loop Finder's score: 100 × (1 − raw / the 95th percentile of every pair's raw score), at least 0; at a
    tie the shorter loop, then the earlier; a pair (a, b) is the loop In = frame a, Out = frame b (b the frame after the
    last), a from the range's second frame (its motion needs the one before); thumbnails of frames a and b − 1.
51. **R-51** Pause stops the batch entry under way — an encode cannot wait half-written — and it goes back to Pending
    (it starts over); a reopened queue waits for Start. An entry whose file the check finds wrong is Failed with "The
    check found: …".
52. **R-52** Ctrl+Shift+E adds the project as it is now (a snapshot), written where Export would suggest (the last
    export's name in the format's extension, else the name template in the export folder); a name already on disk or
    in the queue takes the first free "name (n)". Export waits while the queue runs, the
    queue does not start while Export runs, and quitting during a batch asks first.
53. **R-53** GIF's transparency is 1-bit at alpha 128; the export makes its palette from every frame (a first pass), the
    size estimate and the Encoded preview from the first frame. Its delays are hundredths (30 fps: 3, 3, 4 …); a rate
    above 50 is written at 50 (the Output section says so).
54. **R-54** MP4 H.264 takes the preset's bitrate (Low 2, Medium 6, High 12 Mbit/s; Custom: Max bitrate), no B-frames,
    one GOP; Media Foundation's encoder, else OpenH264 (BSD). ProRes is profile 4444 in yuva444p10le (yuv444p10le
    without alpha). GIF and APNG have no quality settings, ProRes its profile (the Quality section says so).
55. **R-55** queue.json lives beside the settings (`%APPDATA%\AvLoop`; AVLOOP_DATA_DIR when set) and is locked while an
    AvLoop holds it: a second AvLoop's queue works but is not saved (its window says so).
56. **R-56** The installer installs for the user alone by default (no administrator; `%LOCALAPPDATA%\Programs\AvLoop`)
    and for every user by the user's choice (Inno's dialog); the association goes where the install goes (HKA).
57. **R-57** Uninstalling removes the program, the association, `%LOCALAPPDATA%\AvLoop` (autosave, logs) and
    `%LOCALAPPDATA%\cache\AvLoop` (the disk cache), and keeps `%APPDATA%\AvLoop` (settings, presets, queue). Qt's
    shader cache beside it (`qtshadercache-*`) is shared by every Qt program and stays.
58. **R-58** The deployment carries no network module (Qt's TUIO touch plugin, TLS and network information left out),
    the VC++ runtime beside the exe, and the version as avloop.exe's VERSIONINFO, which the installer reads.
59. **R-59** "AvLoop by Rubel" is the name shown — the window, the top bar, the menu, the About box, the questions'
    titles, avloop.exe's details, the installer, its shortcuts and Apps & features. The identity stays "AvLoop": the
    settings and data folders, the install folder, the AppId, the `.avloop` association, the executables' names, the
    project file's keys — renaming them would lose the owner's settings and break an update over 1.0.0.
60. **R-60** The About box's details are those of the owner's gLayer About box (name, Bangladesh, the Facebook
    profile, WhatsApp), the email the owner's correction, no website; the publisher and copyright the owner's name.
61. **R-61** The secrets gate allows the owner's address in `src/ui/dialogs/AboutDialog.cpp` alone — the owner's public
    contact at the owner's request; every other address, or this one elsewhere, still fails. The programme's documents
    and walks name it, and the WhatsApp number, by their row, never by their value.
62. **R-62** The renamed build is version 1.0.1 (it differs from the Release 1.0.0) and is released as `v1.0.1` with
    its installer and SHA-256, as R-5 did for 1.0.0, so the README's Download link gives the app as it stands; an
    install over 1.0.0 removes 1.0.0's "AvLoop" shortcuts.
63. **R-63** The canvas's wheel zooms about the pointer with Ctrl or without, a step of §6.3's list a notch; a finer
    wheel or a touchpad adds up to a notch. Shift+wheel and a sideways wheel still pan across; panning up and down by
    the wheel gives way to the zoom (a middle-drag or the Hand pan every way). Ctrl+wheel is kept: the hand that
    learned it, and the walks, still zoom. The timeline's wheel is unchanged — the owner named the canvas.
64. **R-64** Alt+drag on a keyframe's diamond copies it, as After Effects and Premiere do: the first step off its frame
    makes the copy (position, scale, interpolation), the original stays, the copy follows on the grid and is refused
    where another keyframe is; one Undo step, "Copy a keyframe"; Alt+click alone copies nothing. A drag refused over
    another keyframe now keeps its own (AV-011's drag picked the other one up).
65. **R-65** No new version and no Release for item 16: main runs ahead of `v1.0.1` until the next Release carries it.
66. **R-66** "Lots of clip effects" read as eighteen values in four groups, as an editor's basic correction and looks:
    Light (Exposure, Contrast, Highlights, Shadows, Whites, Blacks), Colour (Temperature, Tint, Saturation, Vibrance,
    Hue), Creative (Sepia, Invert, Posterize, Vignette and its size, Opacity), Detail (Sharpen, Blur); with On (off
    keeps the values, shows the clip as it is) and Reset. Constant over the clip, not keyframed; a new video starts
    with none; presets keep Loop, Output and Quality only.
67. **R-67** The output's effects are made on the CPU on each source frame before either back end draws it, so the
    GPU and the CPU agree by construction (AVADR-0001 kept): Blur is three box passes each way (σ = radius / 2, the
    edges held, so an opaque clip stays opaque), Sharpen an unsharp mask of σ 1 source px. The canvas draws a close
    look on the GPU (a 64³ colour table, 7 × 7-tap Gaussians) so editing stays live at any size; the Preview is exact.
68. **R-68** The project file's version 2 carries `"effects"`; a version-1 file opens without effects; AvLoop 1.0.1
    refuses a version-2 file with its "saved by a newer AvLoop" message rather than drop the effects silently.
69. **R-69** Opacity under 100 % turns Alpha Auto on (the clip is see-through). The timeline's thumbnails and the Loop
    Finder read the source as it is: they find frames, not looks.
70. **R-70** The section is the fifth, "Clip effects", after Quality and open by default — it is long, and the four
    before it keep their places; its words in the app's British English ("Colour"). No new version and no Release
    (as R-65).
71. **R-71** "Change the colour theme" read as four effects in the Clip effects section, after Colour: Replace colour
    — From (picked from the clip) and To, Hue range, Softness, Amount: a pixel's hue near From's is turned by the
    From → To turn and its saturation and value scaled by To's over From's, greys kept —; Colourise (every pixel one
    hue, its lightness kept); Split toning (shadow and highlight tints); Duotone (black and white become two colours).
    Replace comes first, on the clip's own colours, so a colour picked from it is found; the others after Saturation.
72. **R-72** The eyedropper beside From takes the clip's colour as it is (before effects) where the canvas is
    clicked; Esc or a click beside the video picks nothing, and the pick's click is not the tool's. Each colour is a
    hex field, a swatch (the colour dialog) — a colour chosen while its effect's Amount is 0 sets it to 100 %.
73. **R-73** The file stays at version 2 (AV-019's version 2 was never released); an effect at 0 % keeps its colours
    without counting (the frame untouched). The eyedropper is the app's 44th icon. No Release (as R-65). The owner's
    "forcly shutdown my pc" after the push: `shutdown /s /f` once CI is green, the open AvLoop's autosave kept.

## 4. What is not built

- Audio: no audio is decoded, played or exported. Output above 1080p.
- A video rotation tool (rotation metadata is only corrected), text or stickers, multiple video layers. (Colour
  correction came with item 17.) Multiple crop areas at once; keyframes of the crop frame.
- macOS or Linux builds, an auto-updater, licence keys, telemetry. Code signing.
- Anything the specification does not name.

## 5. Where things are

- The order `control/01`; the gates and situations `control/02`; the lab `control/04`; the shapes `control/03`
  (item 1), `05` … `17` (items 2 … 14).
- `DECISIONS.md`, `requirements/01-00_REQUIREMENTS.md`, `RISKS.md`, `OWNER-ACTIONS.md`, `PLANNING-VALIDATION.md`,
  `adr/AVADR-0001-one-renderer-gpu-cpu-parity.md`, `adr/AVADR-0002-memory-budgets-and-caches.md`.
- The code (spec §4): `src/core`, `src/media`, `src/render`, `src/export`, `src/ui`, `src/app`, `src/cli`; `tests/`;
  `tools/` (`media/make-samples.mjs`, `icons/check-icons.mjs`, `check-secrets.mjs`, `walks/*.json`); `scripts/`;
  `cmake/`; `resources/`; `installer/`; `docs/SPEC.md` §0 (D-001 … D-014), `docs/lore/avloop.md`; `CLAUDE.md`.
