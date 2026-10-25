# Executor Preamble — AvLoop

**STARTED 2026-10-26** (the specification is the word to code; the owner's "start"). Started: validate and close each
successful phase, commit and push it under the owner's standing order, then continue to the next eligible phase
without stopping, through both checkpoints. Canonical state: `AUTONOMOUS_STATE.md`. Owner scope: `README.md` §1, §2,
§3.

## 1. Authority and existing rules

Owner instruction (`README.md` §1) → the specification (`docs/SPEC.md`) → this programme's ADRs (AVADR-0001 one
renderer and GPU/CPU parity; AVADR-0002 memory budgets and caches) and contracts → phase contract → the simplest
implementation that satisfies **Done when**. A requested outcome is never silently dropped, and nothing unrequested
is added — **`README.md` §4 is not built**.

| Rule | Source |
| --- | --- |
| **No code file over 600 lines — any code file, anywhere in the repository**; new files aim under 450; programme documents and tools under 300 | spec §2.4; `CLAUDE.md` |
| Five-minute ceiling on every command; one test suite at a time | spec §2.6 |
| Phase headings, statuses, sizing, execution records, folder naming as the owner's example | spec §2.1 … §2.3 |
| `core/` never includes Widgets or FFmpeg; `ui/` never calls FFmpeg; the model changes only through Undo commands; `Project` emits `changed(Aspect)` | spec §3.2 |
| Preview and export use the same `FrameRenderer` | spec §3, AVADR-0001 |
| Every cache and queue bounded; RAII for every FFmpeg and GL object; lifetimes released; the memory walk flat | spec §3.7, AVADR-0002 |
| Every GPU path has a CPU fallback with the same result | spec §3.6 |
| No GPL component linked (libx264, libx265); the FFmpeg build has no `--enable-gpl` | spec §3.1 |
| Every user-visible word through `tr()`; a label at most 40 characters; English (R-1) | spec §2.7 |
| A project-file key or settings key is never renamed; a format change bumps `version` with a migration | spec §2.7 |
| `.clang-format` (LLVM, 120 columns); `/W4 /WX`; no raw `new`/`delete` but Qt parents | spec §2.7 |
| The public repository: no secret, personal path, email or token committed; `check-secrets.mjs` before every commit | spec §2.9 |
| Evidence over tests: ffprobe's fields, the pictures described, the log's lines, the observed values | spec §2.6 |

Programme substitutions: prefix AV; two tracks, A (AV-001 … AV-010, closed by CP-A1, AV-015) and B (AV-011 … AV-014,
closed by CP-B1, AV-016); an item phase's ordinal is its item (`README.md` §2); a corrective takes the next free letter
and names `parent_phase`; a late phase `late: true` above the checkpoints; requirement family REQ-AV; ADR prefix AVADR
(AVADR-0002 accepted by AV-002, AVADR-0001 by AV-007); commit tag `[AV-NNN]`; the plan's commit `[AV] Plan`.

## 2. Frozen outcomes

- **Item 1, the ground** (D-001); **2, the media** (D-002); **3, the canvas and the Selection** (D-003); **4, the
  crop tools** (D-004); **5, the timeline** (D-005); **6, the loop engine** (D-006); **7, the renderer** (D-007);
  **8, the preview window** (D-008); **9, the export** (D-009); **10, the project and the settings** (D-010);
  **11, the keyframes** (D-011); **12, the loop finder** (D-012); **13, the batch and the other formats** (D-013);
  **14, the installer** (D-014).
- **Not built**: `README.md` §4.
- Reserved numbers: **D-001 … D-014** (`DECISIONS.md`; spec §0, newest first).

## 3. Per-phase procedure

Read this file, the state, the progress file, the active contract and its `read_scope`. Verify the working tree and
the sizes of the files to be touched — a code file that would pass 600 lines is split first (`RISKS.md` 1). Implement
one coherent slice. Exceeding 150% of `code_budget` requires a recorded deviation or a split at the seam (the budget
counts hand-written lines of `src/`, `tests/`, `tools/`, `scripts/`, `cmake/`, `resources/`; generated files and the
icons' SVG are not counted). Three distinct root-cause attempts on one failure require an explicit decision — the
simplest alternative that meets **Done when**, written under Deviations —; never weaken a gate. Update
`docs/SPEC.md` §0 under the phase's D-number with delivered behaviour; the lore takes the rule
(`docs/lore/avloop.md`). **Each phase runs its own walk before it closes — none is deferred to a checkpoint.** **A
failure found is fixed before the next phase, never carried.**

## 4. Validation and close

1. Focused tests for the slice, then the gates the change touches (`control/02_MAJOR_CHECKPOINTS.md` §1).
2. The walk: `node control/lab/walk-phase.mjs execution/AV-NNN-walk <walk …>` — the pictures looked at and described
   in the record; nothing of the walk left.
3. `node control/phase-tools/check-loc.mjs` prints PASS; `node tools/check-secrets.mjs` prints PASS.
4. README current: the feature list true; the screenshots retaken by `tools/walks/readme-shots.json` where the UI
   changed (each PNG under 300 KB).
5. Write `execution/AV-NNN-EXECUTION.md` from `execution/_TEMPLATE-EXECUTION.md`.
6. `node control/phase-tools/close-phase.mjs AV-NNN "<one line>"` — `sync-progress.mjs`,
   `validate-phase-system.mjs`, `check-continuity.mjs`.
7. Commit explicit paths with `[AV-NNN]`, the message from a file (`git commit -F`).
8. `git fetch origin`; take in what is there (never a forced push); `git push origin main` (on failure retry after
   30 s, 2 min, 10 min, else at the next close, written in the record); `git status -sb` reads `## main...origin/main`.
9. `gh run watch` on the push's CI run (GH_TOKEN from git's credential manager, R-12): red is fixed before the next
   phase.
10. Promote the next eligible phase (`close-phase.mjs` did) and continue.

## 5. The owner's standing instructions

- Autonomous: never ask; decide, record the reading, continue (spec §2.8).
- Code files stay under 600 lines. **After every completed phase everything is pushed to `origin` `main`.**
- The README is always professional and current, with real screenshots.
- No code signing; Windows only.
