---
phase: AV-009
title: "The export"
status: IN_PROGRESS
owner_item: 9
track: A
checkpoint_id: CP-A1
is_checkpoint: false
risk: High
permission_class: VERIFY_ENVIRONMENT_FIRST
code_budget: 600
depends_on: [AV-008]
unlocks: [AV-010, AV-015]
requirements: [REQ-AV-009, REQ-AV-016, REQ-AV-020]
read_scope:
  - phases/26-10-26-034934-AvLoop/control/12_SHAPES_THE_EXPORT.md
  - docs/SPEC.md
  - spec: §10.1, §10.2, §10.3, §12.3
  - after AV-008: src/media/Encoder.h
  - after AV-007: src/render/FrameRenderer.h
---

# The export

Canonical current execution state: `../AUTONOMOUS_STATE.md`

**Goal.** Export writes a VP9 (or VP8) alpha WebM that OBS's Media Source plays as a seamless transparent loop: the
background job from a project snapshot with bounded queues, `.part` and the atomic rename, progress and Cancel,
verification, the Output and Quality panels, Target size by two passes, the size estimate, the OBS tip
(`../README.md` §2 item 9; D-009).

**Ground truth**

- Spec §10.1–10.3, §12.3; the encoder and muxer of AV-008; the renderer of AV-007.
- Default output (reading R-6): VP9, Medium, Alpha Auto, Crop size, Source FPS (≤ 60).

**Do**

1. `export/ExportJob` (a `QThread`; Decoder → FrameRenderer (GPU) → PBO → swscale `yuva420p` → Encoder → Muxer;
   decode→render 6 frames, render→encode 6 frames; `<name>.part`, then `MoveFileEx` with REPLACE_EXISTING; 5 rename
   retries 1 s apart, then `name (1).webm`; Cancel or error deletes `.part`; disk-full message with the space needed;
   allocation failure stops cleanly; GPU loss continues on the CPU from that frame).
2. `export/OutputVerify` (frame count = L, FPS, W×H, codec, alpha by the libvpx decoder, first frame a keyframe);
   `export/SizeEstimator` (3 spread parts × 10 frames, 1 s after a change, `~1.8 MB`); `export/FormatProfiles`.
3. `ui/panels/OutputPanel.cpp` (Format: VP9, VP8 — the others at AV-013 —, Size: Crop, Long side 1920 … 256,
   Custom W×H with lock; Alpha Auto/On/Off; Background; FPS rows) and `QualityPanel.cpp` (Low, Medium, High,
   Custom: CRF 4–63, Speed 0–8, Max bitrate, Two-pass, Target size MB).
4. `ui/export/ExportBar` (status bar: `Exporting 86/149 · 58% · 41 fps · ETA 0:02` + Cancel); the Save dialog
   (`<source>_loop_<W>x<H>.webm`, the last folder); the success toast with *Show in folder*; the OBS tip once a
   session with *Don't show again*; "Export in progress — cancel and quit?"; Export disabled with the reason when D < 2.
5. `avloop-cli export <project.avloop> -o <out>`; `tests/test-export.cpp`.

**Do not**

GIF/APNG/MP4/ProRes and the batch queue (AV-013); the project file's UX (AV-010).

**Done when**

Build, ctest, gates PASS; S10 — ffprobe `codec_name=vp9`, W×H, `alpha_mode=1` when Alpha is On, no audio stream,
first frame a keyframe, alpha plane by the libvpx decoder, no `.part` after Cancel, Target size within ±5%, VP8 the
same; S7 — every mode's frame count = L, `seamDiff` ≤ 1.5 × the median neighbour difference, no duplicate at the
turns; S8 — 15/24/30/60 and Custom 12, all three methods, `r_frame_rate` right, CFR, Speed 0.5× and 2× lengths;
S11 — `-stream_loop 1` gives 2L frames without error; three exports in a row hold the memory flat.

**Decisions this phase must record**

D-009 (spec §0); reading R-6.

**Validation**

`scripts\build.ps1`; `ctest … -R export`; `node control/lab/walk-phase.mjs execution/AV-009-walk walk-export.mjs
walk-loops.mjs walk-fps.mjs walk-export-ui.json`; format; check-loc; check-secrets.

**Seam if this splits.** AV-009a — Target size, the estimator and the OBS tip — after the job, verification and the
two panels.
