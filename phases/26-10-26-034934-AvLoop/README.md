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

## 2. The items

Fourteen, in two tracks; a checkpoint closes each track.

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

## 4. What is not built

- Audio: no audio is decoded, played or exported. Output above 1080p.
- A video rotation tool (rotation metadata is only corrected), colour correction, text or stickers, multiple video
  layers. Multiple crop areas at once; keyframes of the crop frame.
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
