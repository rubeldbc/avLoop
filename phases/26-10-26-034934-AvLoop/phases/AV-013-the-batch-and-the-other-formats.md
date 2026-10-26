---
phase: AV-013
title: "The batch and the other formats"
status: IN_PROGRESS
owner_item: 13
track: B
checkpoint_id: CP-B1
is_checkpoint: false
risk: Medium
permission_class: VERIFY_ENVIRONMENT_FIRST
code_budget: 550
depends_on: [AV-012]
unlocks: [AV-014, AV-016]
requirements: [REQ-AV-013]
read_scope:
  - phases/26-10-26-034934-AvLoop/control/16_SHAPES_THE_BATCH_AND_FORMATS.md
  - docs/SPEC.md
  - spec: §10.4, §10.5
  - after AV-009: src/export/ExportJob.h
---

# The batch and the other formats

Canonical current execution state: `../AUTONOMOUS_STATE.md`

**Goal.** Several exports run one after another from a queue that survives a restart, and four more formats are
written: GIF, APNG, MP4 H.264 (Media Foundation, no GPL) and MOV ProRes 4444 (`../README.md` §2 item 13; D-013).

**Ground truth**

- Spec §10.4 (encoders, alpha, notes), §10.5 (the queue, the window, `%APPDATA%\AvLoop\queue.json`, one at a time,
  failures move on); the `ExportJob` of AV-009; no libx264/libx265 (§3.1).

**Do**

1. `export/FormatProfiles` grows GIF (`palettegen` stats diff + `paletteuse` bayer; 1-bit alpha at 50%; `-loop 0`;
   ≤ 50 fps), APNG (`-plays 0`), MP4 (`h264_mf`, `yuv420p`, `+faststart`, the Background colour), ProRes 4444
   (`prores_ks -profile:v 4444`, `yuva444p10le`); the Output panel's Format row offers them.
2. `export/ExportQueue` (entries = a project snapshot + an output path; one at a time; the queue file locked; Pending
   on reopen; a failure's reason kept and the next entry run).
3. `ui/export/BatchWindow` (list: name, format, size, state, progress; drag to reorder; Start, Pause, Remove, Clear
   done); Ctrl+Shift+E adds to the queue; Menu → Batch Queue.
4. `tests/test-export.cpp` grows each format.

**Do not**

Link any GPL encoder; run two exports at once.

**Done when**

Build, ctest, gates PASS; S14's formats and batch — a 3-entry batch finishes; GIF, APNG, MP4, ProRes correct in
ffprobe (codec, size, frame count, alpha where full); the batch window pictured; the queue back as Pending after a
restart.

**Decisions this phase must record**

D-013 (spec §0); `h264_mf`'s availability on the build's FFmpeg recorded.

**Validation**

`scripts\build.ps1`; `ctest … -R export`; `node control/lab/walk-phase.mjs execution/AV-013-walk walk-formats.mjs
walk-batch.json`; format; check-loc; check-secrets.

**Seam if this splits.** AV-013a — the batch queue and its window — after the four formats.
