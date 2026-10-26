---
phase: AV-008
title: "The preview window"
status: IN_PROGRESS
owner_item: 8
track: A
checkpoint_id: CP-A1
is_checkpoint: false
risk: Medium
permission_class: VERIFY_ENVIRONMENT_FIRST
code_budget: 500
depends_on: [AV-007]
unlocks: [AV-009, AV-015]
requirements: [REQ-AV-008, REQ-AV-016]
read_scope:
  - phases/26-10-26-034934-AvLoop/control/11_SHAPES_THE_PREVIEW.md
  - docs/SPEC.md
  - spec: §9, §10.3
  - after AV-007: src/render/FrameRenderer.h
---

# The preview window

Canonical current execution state: `../AUTONOMOUS_STATE.md`

**Goal.** One Preview button (F5) opens a separate window that shows only the exported part, exactly as it will be
exported, with a frame-accurate slider, the seam marks, Around seam, backgrounds and zoom; Live renders on the GPU
and caches; Encoded encodes a temporary WebM and plays it through the libvpx decoder as OBS will
(`../README.md` §2 item 8; D-008).

**Ground truth**

- Spec §9 entire; §10.3's encoder parameters (the Encoded mode encodes with the current Output and Quality).
- The `FrameRenderer` (AV-007).

**Do**

1. `ui/preview/PreviewWindow` (non-modal, one at a time, F5 or the button brings it to the front, position and size
   remembered, the app icon; Live updates 250 ms after a change) + `PreviewPlayer` (always loops; Space; ←/→; the
   cached bar; "Caching…" when it cannot keep time; Around seam = the last 2 s + the first 2 s) +
   `PreviewBackground.cpp` (Checker, Black, White, Green `#00FF00`, Navy, Image…; zoom Fit, 50, 100, 200%).
2. The seam marks (the slider's ends; Crossfade's orange strip; Ping-pong's turn points).
3. `media/Encoder` + `EncoderOptions.cpp` + `media/Muxer` (VP9 / VP8 with alpha per §10.3: `yuva420p`, CFR `pts = k`,
   one GOP, first frame a keyframe, no audio) — used by Encoded now and by Export at AV-009; Encoded's temp file
   deleted at close; "Out of date" when settings change; the actual size shown.
4. The info row (W×H, FPS, frames, duration, format, alpha, size); closing releases the player, textures and cache.

**Do not**

The Export button's job, the Output/Quality panels' full rows, target size, the estimator, verification (AV-009).

**Done when**

Build, ctest, gates PASS; S9 — the preview shows only the exported part (screenshot); Live frame N equals
`avloop-cli frame N` (mean difference ≤ 1.0); Encoded keeps alpha (`alphaOf` of the temp file); opening and closing
Preview 5 times returns the memory (the §3.7 walk's part).

**Decisions this phase must record**

D-008 (spec §0); where the encoder lives (shared with AV-009).

**Validation**

`scripts\build.ps1`; `ctest … -R "renderer|export"`; `node control/lab/walk-phase.mjs execution/AV-008-walk
walk-preview.json walk-preview-check.mjs`; format; check-loc; check-secrets.

**Seam if this splits.** AV-008a — the Encoded mode and the encoder — after Live, the slider, the seam marks and
the backgrounds.
