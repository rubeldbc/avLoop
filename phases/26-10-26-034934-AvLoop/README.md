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
