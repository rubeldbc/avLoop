# Planning validation

Does the plan hold what the owner said, and can it be built on what stands? Checked 2026-10-26, before any phase.

## The owner's words are covered

| The owner's words (`README.md` §1) | Phase | Requirement |
| --- | --- | --- |
| "take any video, cut it at a specific In/Out and screen area, and create a loop video of that size" | AV-002, AV-004, AV-005, AV-009 | REQ-AV-002, 004, 005, 009 |
| 1 "Windows only. Used in OBS." | AV-001, AV-009, AV-014 | REQ-AV-001, 009, 014 |
| 2 "All suggested features are kept; audio is dropped." | every item; none decodes audio | REQ-AV-001 … 014 |
| 3 "The Crop tool decides which part is exported. The Selection tool moves and scales the source video" | AV-003, AV-004 | REQ-AV-003, 004 |
| 4 "The crossfade happens inside the selected range" | AV-006 | REQ-AV-006 |
| 5 "Tools instead of buttons … One Preview button opens a separate window with a slider" | AV-001, AV-003 … AV-008 | REQ-AV-001, 008 |
| 6 "Output quality is adjustable. Maximum output is 1080p" | AV-004 (the size rule), AV-009 | REQ-AV-004, 009 |
| 7 "Several aspect ratios … Frame rate is adjustable" | AV-004, AV-006 | REQ-AV-004, 006 |
| 8 "Compact GUI. Orange with black or navy blue theme. The app draws its own icons." | AV-001 | REQ-AV-001 |
| 9 "'Full screen' means a maximized window" | AV-001 | REQ-AV-001 |
| 10 "No code file over 600 lines. Phases … following the owner's example" | every phase; this folder | REQ-AV-015, 021 |
| later 1 "Maximum care with memory. Use the GPU." | AV-002, AV-007, AV-008, AV-009, checkpoints | REQ-AV-016, 017 |
| later 2 "State which libraries are used and why. Any video must be importable." | AV-001 (README, notices), AV-002 | REQ-AV-018, 019 |
| later 3 "No code signing. Windows only." | AV-014 | REQ-AV-014 |
| later 4 "Autonomous mode" | the whole programme | REQ-AV-021 |
| later 5 "Auto-push … Public repository … README always professional … with app screenshots" | every close | REQ-AV-019 |
| "All suggested features": keyframes, loop finder, batch, other formats, installer | AV-011 … AV-014 | REQ-AV-011 … 014 |

### The readings are the owner's to veto

`README.md` §3, R-1 … R-14; `OWNER-ACTIONS.md` rows 4, 6 … 18. The ones the owner would notice first: **R-6 the
default output**, **R-3 Space plays**, **R-2 no distortion**, **R-9 FFmpeg 7.1.5 from a dated build**.

## It can be built on what stands

- **The toolchain is on this PC**: Visual Studio 2022 v143 with its Ninja and clang-format; CMake 4.4; Node 24;
  Python 3.14 (aqtinstall); Qt 6.8.3 `msvc2022_64` installed for this programme; `gh` 2.103 and Inno Setup 6 installed
  by winget; FFmpeg 7.1.5 LGPL shared downloaded (62 MB) from BtbN.
- **The repository exists**: `origin` = `https://github.com/rubeldbc/avLoop`, public, `main`, one commit; the git
  credential manager holds a token that `gh` accepts (`gh api user` answers).
- **The example's tools port unchanged in logic**: `lib.mjs`, `sync-progress.mjs`, `validate-phase-system.mjs`,
  `check-continuity.mjs`, `close-phase.mjs` with the prefix `AV`, two tracks and fourteen items; `check-loc.mjs` walks
  the whole tree for the spec's kinds; the lab is AvLoop's own (`walk-phase.mjs`, `av-lab.mjs`).
- **The hard parts are known parts of FFmpeg and Qt**: D3D11VA by `av_hwdevice_ctx_create`; frame-accurate seeking by
  BACKWARD seek and decode to PTS; libvpx alpha by `yuva420p`; `QOpenGLContext` + `QOffscreenSurface` + FBO + PBO for
  the GPU path; `QUndoStack` with `mergeWith`; `QSaveFile`; `QtConcurrent` for the finder.

## Line counts near the gate

Nothing stands yet; the spec §4 tree names the splits in advance (`RISKS.md` 1).
