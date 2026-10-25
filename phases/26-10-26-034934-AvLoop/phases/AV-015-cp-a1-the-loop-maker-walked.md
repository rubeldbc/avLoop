---
phase: AV-015
title: "CP-A1 the loop maker walked"
status: NOT_STARTED
owner_item: 0
track: A
checkpoint_id: CP-A1
is_checkpoint: true
risk: High
permission_class: VERIFY_ENVIRONMENT_FIRST
code_budget: 200
depends_on: [AV-001, AV-002, AV-003, AV-004, AV-005, AV-006, AV-007, AV-008, AV-009, AV-010]
unlocks: [AV-011, AV-016]
requirements: [REQ-AV-016, REQ-AV-017, REQ-AV-018, REQ-AV-019, REQ-AV-020, REQ-AV-021]
read_scope:
  - phases/26-10-26-034934-AvLoop/control/02_MAJOR_CHECKPOINTS.md
  - phases/26-10-26-034934-AvLoop/control/04_THE_LAB_AND_THE_WALK.md
  - docs/SPEC.md
  - spec: §14
---

# CP-A1 the loop maker walked

Canonical current execution state: `../AUTONOMOUS_STATE.md`

**Goal.** Track A (items 1–10) proved whole on one fresh Release build: every gate of `control/02` §1 and the
situations S1–S13 walked, the memory and GPU walks, the unit tests under AddressSanitizer, the import walk, fresh
README screenshots by `tools/walks/readme-shots.json`.

**Ground truth**

- `control/02_MAJOR_CHECKPOINTS.md` (the gates, the situations); every execution record AV-001 … AV-010; the walks
  each phase left in `execution/AV-NNN-walk/`.

**Do**

1. `scripts\build.ps1 -Fresh -Config Release`; the whole `ctest`; an ASan build (`-Sanitize`) and its `ctest`; a
   Debug build's CRT leak report empty.
2. Every phase's walks again on this build, then the checkpoint's own: the §3.7 memory walk (10 minutes of loop
   playback + 3 exports + Preview opened and closed 5 times; Private Bytes growth < 50 MB, VRAM < 32 MB); the §3.6
   GPU walk (GPU on vs off: mean ≤ 1.0, max ≤ 4); S13's set again with the GPU off; S1 at 100/150/200% DPI.
3. `tools/walks/readme-shots.json` → `docs/screenshots/` (main window, ellipse crop, timeline, preview, export; each
   PNG under 300 KB); README's feature list checked against what works.
4. A failure found becomes a corrective `AV-0NNa` before this closes.

**Do not**

Build a feature; weaken a gate; carry a failure.

**Done when**

S1–S13 each with its observed values in the record; ASan, leak, memory and GPU gates PASS; CI green; README current.

**Decisions this phase must record**

The checkpoint's findings; any corrective phase and its D-number.

**Validation**

`control/02` §1 entire; `node control/lab/walk-phase.mjs execution/AV-015-walk <every walk>`.

**Seam if this splits.** AV-015a — the ASan and leak legs — after the walks of S1–S13.
