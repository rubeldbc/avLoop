---
phase: AV-030
title: "The timeline layers I — the clips and the composite"
status: NOT_STARTED
owner_item: 27
track: B
checkpoint_id: CP-B1
is_checkpoint: false
late: true
risk: High
permission_class: VERIFY_ENVIRONMENT_FIRST
code_budget: 1400
depends_on: [AV-029]
unlocks: [AV-031]
requirements: [REQ-AV-034]
read_scope:
  - phases/26-10-26-034934-AvLoop/control/30_SHAPES_THE_TIMELINE_LAYERS.md
  - phases/26-10-26-034934-AvLoop/adr/AVADR-0001-one-renderer-gpu-cpu-parity.md
  - phases/26-10-26-034934-AvLoop/adr/AVADR-0002-memory-budgets-and-caches.md
  - docs/SPEC.md
  - src/core/Project.h
  - src/core/Project.cpp
  - src/core/ProjectJson.cpp
  - src/core/ProjectMigrate.cpp
  - src/render/RenderJob.h
  - src/render/FrameRenderer.cpp
  - src/render/GlRenderer.cpp
  - src/render/CpuRenderer.cpp
  - src/ui/MainWindowProject.cpp
  - src/cli/CliRender.cpp
  - spec: §3.2, §3.4, §3.5, §3.7, §11.1, §11.2
---

# The timeline layers I — the clips and the composite

Canonical current execution state: `../AUTONOMOUS_STATE.md`

**Goal.** The project a sequence of video tracks and clips, each clip with its own source, place in time, In/Out,
transform, keyframes, effects and shapes; the output their composite, bottom to top, through the one renderer — in
the Preview, every export and avloop-cli; a second video added as a layer (`../README.md` §1 item 27; D-027). A late
phase, the first of two: the owner's last word of 2026-10-29, [1]; AV-031 gives the tracks on the timeline.

**Ground truth**

- The owner's words (`../README.md` §1, 2026-10-29: "multiple layer system in the time line like adobe premiere
  pro"); `../README.md` §4's "multiple video layers" gives way to it, as colour correction did to item 17.
- Spec §3.2 (the model changes only through Undo), §3.4 (one output frame), §3.5, §3.7 (every cache bounded),
  §11.1 (a format change bumps the version with a migration); AVADR-0001 and AVADR-0002; R-7 (premultiplied over).

**Do**

1. `core/Sequence` (Clip, Track), `Project` holding the tracks and the selected clip, its one-video API reading and
   writing that clip, `ProjectClips.cpp` (the clip commands) — R-97, R-98, R-100.
2. The file: `"tracks"`, version 3; `ProjectMigrate` 2 → 3 wrapping the one video; AV-023 … AV-029's files read the
   same way; the shapes a clip's (R-101).
3. The composite: `RenderJob` and `Composite` as layers; `FrameRenderer` with a clip's decoders, effects and mask;
   GlRenderer and CpuRenderer drawing the layers in order; Alpha Auto over the composite (R-102); the decoders'
   bound (8 clips open; AVADR-0002).
4. "Add as layer…" (Ctrl+Shift+O, the ☰ menu, the drop zone); the batch's snapshot and avloop-cli with the whole;
   the thumbnails and the Finder on the selected clip (R-103).
5. Tests (`test-sequence`, the renderers'); `walk-layers-render.mjs`, `walk-layers.json` (Add as layer, the
   canvas showing the composite, the Preview, Undo); the README's feature list.

**Do not**

Draw a layer anywhere but the FrameRenderer; open a decoder a clip beyond the bound; let a one-clip project render
differently by a byte from AV-029's; rename a key inside a clip; add transitions, audio, nesting or speed ramps
(R-99); pass 600 lines — `Project.cpp` (319), `FrameRenderer.cpp` (160), `GlRenderer.cpp` (315) are read first and
split at their seams.

**Done when**

Build 0 warnings; ctest; format, icons, secrets, LOC PASS; `walk-layers-render.mjs` and `walk-layers.json` PASSED;
the walks of the renderer (AV-007), the Preview (AV-008), the export (AV-009), the project file (AV-010), the
keyframes (AV-011), the batch (AV-013), the effects (AV-019) and the shapes (AV-024) PASSED again; the memory walk
flat; README current; pushed with CI green.

**Decisions this phase must record**

D-027 (spec §0, with AV-031); R-97 … R-103.

**Validation**

`node control/lab/walk-phase.mjs execution/AV-030-walk walk-layers.json walk-layers-render.mjs`; the regressions;
check-loc; check-secrets; `validate-phase-system.mjs`.

**Seam if this splits.** AV-030a — the composite in the renderers — after the model, the file and the migration
(the walk then renders the selected clip alone and says so).
