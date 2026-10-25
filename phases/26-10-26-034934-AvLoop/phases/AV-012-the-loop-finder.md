---
phase: AV-012
title: "The loop finder"
status: NOT_STARTED
owner_item: 12
track: B
checkpoint_id: CP-B1
is_checkpoint: false
risk: Medium
permission_class: VERIFY_ENVIRONMENT_FIRST
code_budget: 400
depends_on: [AV-011]
unlocks: [AV-013, AV-016]
requirements: [REQ-AV-012]
read_scope:
  - phases/26-10-26-034934-AvLoop/control/15_SHAPES_THE_LOOP_FINDER.md
  - docs/SPEC.md
  - spec: §11.5
  - after AV-002: src/media/Decoder.h
---

# The loop finder

Canonical current execution state: `../AUTONOMOUS_STATE.md`

**Goal.** Find the best In/Out pairs automatically: 64×64 greyscale fingerprints of the crop region, every pair scored
in parallel with progress and Cancel, the best five shown with thumbnails; a click sets In/Out, a double-click also
opens Preview (`../README.md` §2 item 12; D-012).

**Ground truth**

- Spec §11.5 entire (search range, min/max length 2–10 s, score = mean absolute difference + motion difference,
  QtConcurrent, best 5 at least 1 s apart, score 0–100, fingerprints released at close).

**Do**

1. `export/LoopFinder` (fingerprints from its own decoder — 4 KB a frame —, the scoring of a pair, all pairs under
   QtConcurrent with progress and cancel, the best 5 by non-maximum suppression within 1 s, the 0–100 score).
2. `ui/export/LoopFinderWindow` (Menu → Loop Finder, Ctrl+L; range and lengths; the list with In, Out, length,
   score and both ends' thumbnails; click = SetRange (one Undo step), double-click = set and Preview).
3. `tools/media/make-samples.mjs` grows a known loop (a generated sample repeating every 4 s).
4. `tests/test-loopfinder.cpp` (the known loop ranked first within ±2 frames).

**Do not**

Change In/Out without Undo; keep fingerprints after the window closes.

**Done when**

Build, ctest, gates PASS; S14's finder part — on the 4 s sample the Finder ranks the true loop first (±2 frames);
the window pictured; the memory back after closing.

**Decisions this phase must record**

D-012 (spec §0); the score's scaling to 0–100.

**Validation**

`scripts\build.ps1`; `ctest … -R loopfinder`; `node control/lab/walk-phase.mjs execution/AV-012-walk
walk-finder.json walk-finder.mjs`; format; check-loc; check-secrets.

**Seam if this splits.** AV-012a — the window — after the finder's core and its test.
