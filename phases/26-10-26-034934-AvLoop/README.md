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

Added by the owner on 2026-10-27, item 19, verbatim: *"pull everyhitng from main and create a installer"* (an installer
built from main, still 1.0.1); then *"bump to 1.1.0 and publish a release"*.

Added by the owner on 2026-10-29, items 20 … 27, verbatim — the first message, seven numbered wishes: *"[1] Users
should be able to export without applying a loop at the end, or without adding a crossfade in the middle. [2] A
feature should be implemented to allow for the "inverse selection" of a chosen area, ensuring that the unselected
(inverted) portion is included in the output. [3] Users should be able to create custom shapes, which can then be
used to define a selection or an inverse selection. Additionally, standard shapes—such as rectangles and ellipses—can
also be created and used for selection or inverse selection. [4] Similar to Adobe Illustrator, it should be possible
to create new shapes by combining or permuting multiple existing shapes; these new shapes can then be used for
selection. [5] It should be possible to select specific areas within a shape and apply custom feathering or
transparency, allowing any part of the selection to be made transparent. [6] Clicking on the text label next to any
control should reset its value. [7] Each group on the right-hand side should include a right-click context menu with
a 'Reset' option, which will reset the settings for that group. Please execute these plans accordingly."*; then
*"make phase files first, then wait for my confirmation"*; then *"[1] there will be cutoma shape creation like
illustrator will be available with full pen tool [2] There will be align tool set for multiple object and make a layer
system for multiple objects"*; then *"[1] as well as make multiple layer system in the time line like adobe premiere
pro"*. Read as eight items (§2, items 20 … 27): [1] item 20; [2] item 21; [3] with the pen item 22; [4] item 23; [5]
item 24; [6] and [7] item 25 (one wish of one kind, as item 16); the align and the shape layers item 26; the timeline
layers item 27 (R-77 … R-103 are theirs). The phase files were written first; then the go: *"commit, push and start and go till end of all phases"*.

## 2. The items

Twenty-seven, in two tracks; a checkpoint closes each track. The fifteenth to the twenty-seventh came after both had
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
| 19 | **The release 1.1.0** (AV-021, late) | version 1.1.0 and the GitHub Release `v1.1.0`: main since `v1.0.1` as an installer |
| 20 | **The clip as cut** (AV-022, late) | Mode None: the clip from In to Out as cut — no crossfade, no return, no seam — said so in the panel, the Preview and the export's name |
| 21 | **The inverse selection** (AV-023, late) | Inverse on the crop: the whole video kept, the chosen area a transparent hole, the feather outward; the project file's version 3 |
| 22 | **The shapes and the pen** (AV-024, AV-025, late) | shapes inside the frame that select or cut: rectangles, rounded, ellipses, polygons, freehand (S) and Illustrator's Pen with anchors and handles (P); the mask of them all |
| 23 | **The shape combinations** (AV-026, late) | Pathfinder: Add, Cut, Keep only, Exclude live; Combine the selected shapes into one path by Unite, Minus front, Intersect, Exclude |
| 24 | **The soft parts** (AV-027, late) | Feather and Strength a shape: any part of a selection partly or wholly transparent, its edge softened; exact in the Preview and the export |
| 25 | **The resets** (AV-028, late) | a click on a control's label resets it; a right click on a group of the right panel offers "Reset <group>"; each one Undo step |
| 26 | **The align tools and the shape layers** (AV-029, late) | Align and Distribute the selected shapes to each other or the frame; a layers panel for them: order, eye, lock, names, duplicate |
| 27 | **The timeline layers** (AV-030, AV-031, late) | video tracks and clips as Premiere Pro's: each clip its own source, place, In/Out, transform, keyframes, effects, shapes; the composite rendered; the tracks on the timeline with drags, trims, snapping |

Checkpoints: CP-A1 (AV-015) walks track A on one fresh build; CP-B1 (AV-016) walks the whole, installed, and makes
the GitHub Release `v1.0.0`.

## 3. The readings taken

In `READINGS.md` (R-1 … R-76 as of 2026-10-29; the README passed 300 lines, spec §2.4 — every reference to
"`README.md` §3" means that file). Each is built as read and open for the owner's veto (`OWNER-ACTIONS.md` rows 11
on; `DECISIONS.md`). Items 20 … 27 reserve R-77 … R-103, taken as their phases close.

## 4. What is not built

- Audio: no audio is decoded, played or exported. Output above 1080p.
- A video rotation tool (rotation metadata is only corrected), text or stickers. (Colour correction came with item
  17; multiple video layers with item 27; shapes within the one crop frame with items 21 … 26.) A second crop frame;
  keyframes of the crop frame or of a shape; audio on the tracks; transitions between clips.
- macOS or Linux builds, an auto-updater, licence keys, telemetry. Code signing.
- Anything the specification does not name.

## 5. Where things are

- The order `control/01`; the gates and situations `control/02`; the lab `control/04`; the shapes `control/03`
  (item 1), `05` … `17` (items 2 … 14), `18` … `22` (items 15 … 19), `23` … `30` (items 20 … 27).
- `READINGS.md` (§3), `DECISIONS.md`, `requirements/01-00_REQUIREMENTS.md`, `RISKS.md`, `OWNER-ACTIONS.md`, `PLANNING-VALIDATION.md`,
  `adr/AVADR-0001-one-renderer-gpu-cpu-parity.md`, `adr/AVADR-0002-memory-budgets-and-caches.md`.
- The code (spec §4): `src/core`, `src/media`, `src/render`, `src/export`, `src/ui`, `src/app`, `src/cli`; `tests/`;
  `tools/` (`media/make-samples.mjs`, `icons/check-icons.mjs`, `check-secrets.mjs`, `walks/*.json`); `scripts/`;
  `cmake/`; `resources/`; `installer/`; `docs/SPEC.md` §0 (D-001 … D-014), `docs/lore/avloop.md`; `CLAUDE.md`.
