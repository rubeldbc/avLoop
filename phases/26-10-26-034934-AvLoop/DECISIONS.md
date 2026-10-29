# Decisions

Reserved numbers, newest first in `docs/SPEC.md` §0 when taken — delivered behaviour only. AvLoop's series starts at
D-001: **D-001 … D-014 are this programme's, reserved 2026-10-26 as the fourteen items were planned**, one an item; D-015
the late fifteenth (AV-017); D-016 the late sixteenth (AV-018); D-017 the late seventeenth (AV-019); D-018 the late eighteenth (AV-020); D-019 the late nineteenth (AV-021); **D-020 … D-027 the late twentieth to twenty-seventh (AV-022 … AV-031), reserved 2026-10-29**. Two
ADRs are owed: **AVADR-0001** (one renderer, GPU/CPU parity), accepted by AV-007 before the code it governs, and
**AVADR-0002** (memory budgets and caches), accepted by AV-002. The readings (`README.md` §3) are listed after the
D-numbers; each is the agent's decision under autonomous mode, open for the owner's veto.

| D | Decision | The owner's words | Phase |
| --- | --- | --- | --- |
| D-001 | **The ground**: CMake/Ninja/MSVC with `/W4 /WX`, FFmpeg 7.1 LGPL and Qt 6.8 fetched by script; the maximized shell with the §5.1 layout; Navy and Black from one token table; the app's own icons; `--shot`/`--walk`; `avloop-cli`; the gates; CI; README v1. | "Compact GUI. Orange with black or navy blue theme. The app draws its own icons." "'Full screen' means a maximized window" | AV-001 |
| D-002 | **The media**: any video opened by content; frame-accurate decode; D3D11VA with fallback; normalization (rotation, SAR, deinterlace, HDR→SDR, alpha kept); the bounded frame cache with disk spill. | "Any video must be importable" | AV-002 |
| D-003 | **The canvas and the Selection**: the GL canvas in scene space, Zoom and Hand outside Undo, the Selection moving and scaling the video by its corners, the options bar, the model and Undo. | "The Selection tool moves and scales the source video on the canvas" | AV-003 |
| D-004 | **The crop tools**: Rect, Rounded, Ellipse; ratios; handles and modifiers; snapping; Feather and Radius; the mask. | "The Crop tool decides which part is exported"; "Several aspect ratios can be chosen while cropping" | AV-004 |
| D-005 | **The timeline**: ruler, thumbnails, In/Out, playhead, transport, playback dropping frames rather than stopping. | "cut it at a specific In/Out" | AV-005 |
| D-006 | **The loop engine**: Simple, Crossfade (inside the range), Ping-pong, Smooth turn; Speed; FPS choices and the three methods. | "The crossfade happens inside the selected range"; "Frame rate is adjustable" | AV-006 |
| D-007 | **The renderer**: one `FrameRenderer` for preview and export; GPU (OpenGL 3.3) with a CPU twin of the same result. | "Use the GPU"; "shows only the exported part, exactly as it will be exported" | AV-007 |
| D-008 | **The preview window**: Live and Encoded, the slider, seam marks, Around seam, backgrounds. | "One Preview button opens a separate window with a slider" | AV-008 |
| D-009 | **The export**: VP9/VP8 alpha WebM, Output and Quality panels, target size, estimate, verification, the OBS tip. | "a resized WebM is exported that feels like a loop"; "Output quality is adjustable. Maximum output is 1080p" | AV-009 |
| D-010 | **The project and the settings**: `.avloop`, migration, autosave, Recent, Settings, presets, logs, the shortcut overlay. | "All suggested features are kept" | AV-010 |
| D-011 | **The keyframes**: position and scale over time, Linear and Ease. | "All suggested features are kept" | AV-011 |
| D-012 | **The loop finder**: the best In/Out pairs by fingerprints. | "All suggested features are kept" | AV-012 |
| D-013 | **The batch and the other formats**: the queue; GIF, APNG, MP4 H.264 (no GPL), ProRes 4444. | "All suggested features are kept" | AV-013 |
| D-014 | **The installer**: Inno Setup 6, `.avloop` association, no signing. | "No code signing. Windows only." | AV-014 |
| D-015 | **The name and the About box** (late): "AvLoop by Rubel" shown in the app, its executable and its installer; the About box with the developer and the ways to reach them; version 1.0.1. | "The applicaiton name will be AvLoop by Rubel. There will be a about section where my information will be available." | AV-017 |
| D-016 | **The wheel and the keyframe copy** (late): the wheel zooms the canvas about the pointer; Alt+drag on the timeline copies a keyframe. | "mouse wheen move will zoom in out targeted to the mous pointer area in canvas"; "alt drag will copy a keyframe in time line" | AV-018 |
| D-017 | **The clip effects** (late): a Clip effects section — light, colour, creative and detail effects on the clip, the same on the canvas, in the Preview and in every export; the project file's version 2. | "add a righ side group, Clip effects. where a lots of clip effect could be applied to the clip to change or color correction the clip before export" | AV-019 |
| D-018 | **The colour theme** (late): Replace colour with an eyedropper, Colourise, Split toning, Duotone. | "say a clip is orange color but i want to change it to red color theme. this kind of effect needed" | AV-020 |
| D-019 | **The release 1.1.0** (late): version 1.1.0; the GitHub Release `v1.1.0` with its installer and SHA-256. | "bump to 1.1.0 and publish a release" | AV-021 |
| D-020 | **The clip as cut** (late): Mode None — the clip from In to Out, no loop treatment, said so everywhere. | "export without applying a loop at the end, or without adding a crossfade in the middle" | AV-022 |
| D-021 | **The inverse selection** (late): the crop's area a hole in the whole video, the feather outward; the file's version 3. | "'inverse selection' of a chosen area, ensuring that the unselected (inverted) portion is included in the output" | AV-023 |
| D-022 | **The shapes and the pen** (late): standard and custom shapes that select or cut inside the frame; the Shape tool and Illustrator's Pen. | "create custom shapes … standard shapes—such as rectangles and ellipses"; "full pen tool" | AV-024, AV-025 |
| D-023 | **The shape combinations** (late): Add, Cut, Keep only, Exclude live; Combine into one path. | "Similar to Adobe Illustrator … combining or permuting multiple existing shapes" | AV-026 |
| D-024 | **The soft parts** (late): Feather and Strength a shape. | "custom feathering or transparency, allowing any part of the selection to be made transparent" | AV-027 |
| D-025 | **The resets** (late): a label's click resets its control; a group's right-click Reset. | "Clicking on the text label next to any control should reset its value"; "right-click context menu with a 'Reset' option" | AV-028 |
| D-026 | **The align tools and the shape layers** (late): Align and Distribute; the layers panel. | "align tool set for multiple object and make a layer system for multiple objects" | AV-029 |
| D-027 | **The timeline layers** (late): tracks and clips, the composite, the tracks on the timeline. | "multiple layer system in the time line like adobe premiere pro" | AV-030, AV-031 |

## The readings (the agent's decisions)

| R | Reading | Why | Where |
| --- | --- | --- | --- |
| R-1 | UI language English; every string through `tr()` | a Bengali translation can be added later | spec §13.2 |
| R-2 | W/H unlocked never distorts the video | distortion was not asked for | spec §13.2 |
| R-3 | Space = Play/Pause; pan by middle mouse or H | Space is the player's key everywhere else | spec §13.2 |
| R-4 | MIT licence | public, open to contributions; LGPL-compatible | spec §13.2 |
| R-5 | GitHub Release `v1.0.0` with the installer at CP-B1 | the README's Download link must work | spec §13.2 |
| R-6 | Default output VP9, Medium, Alpha Auto, Crop size, Source FPS (≤ 60) | what OBS plays best | spec §13.2 |
| R-7 | Premultiplied sRGB blending | matches OBS's compositing | spec §13.2 |
| R-8 | One crop at a time; none = the whole video | §1.3 | spec §13.2 |
| R-9 | FFmpeg `n7.1.5` from BtbN's dated release, pinned with SHA-256, newest-7.1 fallback | `latest` no longer carries 7.1 | `README.md` §3 |
| R-10 | Qt 6.8.3, the last open-source 6.8 LTS patch | 6.8.4+ are commercial-only | `README.md` §3 |
| R-11 | The owner's original spec and mock stay untracked; the repository holds copies | never move the owner's files | `README.md` §3 |
| R-12 | `gh` gets `GH_TOKEN` from git's credential manager per command | §2.9's "existing git credentials"; nothing stored | `README.md` §3 |
| R-13 | The `.avloop` format is written by AV-003; AV-010 builds its UX | every walk and `avloop-cli` need projects early | `README.md` §3 |
| R-14 | Encoder and muxer built by AV-008, reused by AV-009 | Encoded preview precedes Export in the order | `README.md` §3 |
| R-15 | An animated WebP is refused with a plain message | no FFmpeg (7.1, 8.1) decodes it; no decoder of our own was asked for | `README.md` §3; AV-002 |
| R-16 | The working size before a crop: the display size, long side ≤ 1920 | spec §3.7.3, no full 4K frames kept | `README.md` §3; AV-002 |
| R-17 | `src/core` links Qt Gui for QUndoStack only | Qt 6 moved it there; §11.2 names it | `README.md` §3; AV-003 |
| R-18 | Reset and load: the video's top-left at (0,0), 100% | §3.3, §6.2 and §11.1's example agree | `README.md` §3; AV-003 |
| R-19 | Fit/Fill/Center without a crop act on the video's 100% frame | no crop = the video rectangle (R-8) | `README.md` §3; AV-003 |
| R-20 | The video border and handles show with the Selection only | the owner's mock | `README.md` §3; AV-003 |
| R-21 | Custom… takes the crop's proportions in lowest terms; typed W/H keep the top-left | §6.1 names no anchor | `README.md` §3; AV-004 |
| R-22 | The canvas shows the crop's outline; Feather is seen in Preview and export | §6.1's overlay list | `README.md` §3; AV-004 |
| R-23 | Snapping: corners, edges or centre lines to the video's edges and centre | §6.1 | `README.md` §3; AV-004 |
| R-24 | The range is In up to Out: I the first frame, O the last | §8.1's D | `README.md` §3; AV-005 |
| R-25 | Dropped = never shown; frames shown in order, dropped only beyond two frames late | §7.3 | `README.md` §3; AV-005 |
| R-26 | Timeline repaints at most 30 a second while playing | GL composition cost | `README.md` §3; AV-005 |
| R-27 | Scrub: keyframe while moving, exact at rest (150 ms) or release | §7.2 | `README.md` §3; AV-005 |
| R-28 | The default loop is Crossfade (F 0.5 s, Smooth) | the app's purpose; §11.1's example | `README.md` §3; AV-006 |
| R-29 | A new rate moves In/Out to its grid in the same Undo step | §8.3 | `README.md` §3; AV-006 |
| R-30 | The Motion pre-pass runs for the Preview and the export | §8.3, §9, §10 | `README.md` §3; AV-006 |
| R-31 | The Preview's cache: a third of the frame cache, ≤ 512 MB | §3.7.1 | `README.md` §3; AV-008 |
| R-32 | VP9 in BT.709, VP8 in BT.601; straight colour, separate alpha | what decoders read | `README.md` §3; AV-008 |
| R-33 | One render thread and GPU context for the app's life | GPU-driver memory (AV-007) | `README.md` §3; AV-008 |
| R-34 | Export opens the Save dialog each time, filled with the last export | §10.1 | `README.md` §3; AV-009 |
| R-35 | The size estimate follows changes, not a video's opening | §10.3, §3.7.5 | `README.md` §3; AV-009 |
| R-36 | A locked destination: `name (1).webm` at once, with a toast | §12.3 | `README.md` §3; AV-009 |
| R-37 | Target size in MiB; one correction pass when > 5 % off either way | §10.3 | `README.md` §3; AV-009 |
| R-38 | The `.avloop` output keys: size, w, h, lock, alpha, background; export.lastPath | §11.1 | `README.md` §3; AV-009 |
| R-39 | Version 1 is the first format; no version reads as 1 | §11.1 | `README.md` §3; AV-010 |
| R-40 | The built-in presets' sizes are long sides | §11.3; no crop in a preset | `README.md` §3; AV-010 |
| R-41 | A video without a project is titled by its name | §11.1 | `README.md` §3; AV-010 |
| R-42 | A lab run answers questions from its walk; its own data folder | §2.8.6 | `README.md` §3; AV-010 |
| R-43 | A changed source: a warning toast | §11.1 | `README.md` §3; AV-010 |
| R-44 | Settings saved at once, each applied when it can be | §11.3 | `README.md` §3; AV-010 |
| R-45 | The Debug leak gate: no heap block left by repeated cycles | §3.7.8 | `README.md` §3; AV-015 |
| R-46 | ASan judges memory errors; growth thresholds judged in Release | §3.7.8 | `README.md` §3; AV-015 |
| R-47 | Flips hold: not keyframed | §6.4 | `README.md` §3; AV-011 |
| R-48 | The keyframe acted on is the one at the playhead | §6.4 | `README.md` §3; AV-011 |
| R-49 | No crop: the frame where the video stands at In; Alpha Auto over every keyframe | §1.3, §10.2 | `README.md` §3; AV-011 |
| R-50 | The Finder's 0–100 score by the 95th percentile; ties to the shorter loop | §11.5 | `README.md` §3; AV-012 |
| R-51 | Pause stops the entry under way (Pending again); a reopened queue waits for Start | §10.5 | `README.md` §3; AV-013 |
| R-52 | Ctrl+Shift+E adds a snapshot, a free name; Export and the queue never at once | §10.5 | `README.md` §3; AV-013 |
| R-53 | GIF: 1-bit alpha at 128, the palette of every frame, at most 50 fps | §10.4 | `README.md` §3; AV-013 |
| R-54 | MP4: the preset's bitrate, h264_mf else OpenH264; ProRes its profile; GIF/APNG none | §10.4 | `README.md` §3; AV-013 |
| R-55 | queue.json beside the settings, locked; a second AvLoop's queue not saved | §10.5 | `README.md` §3; AV-013 |
| R-56 | Install scope: per user by default, every user by choice | §1.2 item 14 | `README.md` §3; AV-014 |
| R-57 | Uninstall keeps %APPDATA%\AvLoop only; Qt's shared shader cache stays | §1.2 item 14 | `README.md` §3; AV-014 |
| R-58 | No network module; VC++ runtime app-local; the version from the exe | §3.1, §3.8 | `README.md` §3; AV-014 |
| R-59 | "AvLoop by Rubel" shown; folders, AppId, association and file names stay "AvLoop" | settings keys never renamed; updates over 1.0.0 | `README.md` §3; AV-017 |
| R-60 | About's details from the owner's gLayer About; the email the owner's correction; no website | the owner's words | `README.md` §3; AV-017 |
| R-61 | The secrets gate allows that one address in AboutDialog.cpp only | the gate stays whole for everything else | `README.md` §3; AV-017 |
| R-62 | Version 1.0.1, released as `v1.0.1` with its installer | a build unlike 1.0.0 is not 1.0.0; the README's Download link (R-5) | `README.md` §3; AV-017 |
| R-63 | The canvas's wheel zooms about the pointer, Ctrl or not, a step a notch; Shift/sideways pans across; the timeline's wheel unchanged | the owner's words name the canvas | `README.md` §3; AV-018 |
| R-64 | Alt+drag copies a keyframe, one Undo step; a refused drag keeps its own keyframe | After Effects' and Premiere's Alt+drag | `README.md` §3; AV-018 |
| R-65 | No new version, no Release for item 16 | a Release is published only when asked or at a checkpoint | `README.md` §3; AV-018 |
| R-66 | Eighteen effects in four groups, On and Reset; not keyframed; not in presets | an editor's basic correction and looks | `README.md` §3; AV-019 |
| R-67 | The output's effects on the CPU before both back ends; the canvas a close GPU look | AVADR-0001's parity kept; a live canvas | `README.md` §3; AV-019 |
| R-68 | The file's version 2; 1.0.1 refuses it with its message | never drop the effects silently | `README.md` §3; AV-019 |
| R-69 | Opacity < 100 % turns Alpha Auto on; thumbnails and the Finder read the source | see-through needs alpha; they find frames | `README.md` §3; AV-019 |
| R-70 | The fifth section, after Quality, open; British words; no Release | the four before keep their places | `README.md` §3; AV-019 |
| R-71 | Four colour-theme effects; Replace first, on the clip's own colours | "orange … to red color theme" | `README.md` §3; AV-020 |
| R-72 | The eyedropper picks the clip as it is; a colour at 0 % sets 100 % | a picked colour must be found | `README.md` §3; AV-020 |
| R-73 | Version 2 kept; 44 icons; no Release; the shutdown after green CI | AV-019's version unreleased | `README.md` §3; AV-020 |
| R-74 | 1.1.0, a minor step; the Release `v1.1.0` as R-5 and R-62, latest | features and file version 2 since 1.0.1 | `README.md` §3; AV-021 |
| R-75 | No silent install here; the build, deployment and installer checked by their details | the owner's own install shares the AppId | `README.md` §3; AV-021 |
| R-76 | AV-017's walks read the version from `CMakeLists.txt` | the next version changes no walk | `README.md` §3; AV-021 |
| R-77 | Mode None: the clip as cut, L = D, said "Clip"; Simple kept | "without applying a loop … without adding a crossfade" | `READINGS.md`; AV-022 |
| R-78 | "none" under version 2 until AV-023's version 3; no Release between | 1.1.0 would read it as Crossfade | `READINGS.md`; AV-022 |
| R-79 | Inverse: the whole video kept, the crop's shape a hole, the feather outward | "the unselected (inverted) portion is included in the output" | `READINGS.md`; AV-023 |
| R-80 | The file's version 3; 1.1.0 refuses it | a new key, never dropped silently | `READINGS.md`; AV-023 |
| R-81 | Delete/Clear remove the inverse; a redrawn crop keeps it; the 45th icon | no crop, nothing to invert | `READINGS.md`; AV-023 |
| R-82 | Shapes inside the one frame (the whole video with no crop); Select or Cut; not keyframed | "used to define a selection or an inverse selection" | `READINGS.md`; AV-024 |
| R-83 | `"shapes"` under crop; roles `"add"`/`"cut"`; every later field written from the start | no written value renamed later | `READINGS.md`; AV-024 |
| R-84 | One running coverage bottom to top, the crop's multiplying; QPainterPath in core | one mask (AVADR-0001), bounded memory | `READINGS.md`; AV-024 |
| R-85 | S; its bar types one standard shape's place; no snapping for shapes | a snap would fight clicks and freehand | `READINGS.md`; AV-024 |
| R-87 | Icons 46 … 49 | the app draws its own icons | `READINGS.md`; AV-024 |
