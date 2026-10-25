# AGENT_BOOTSTRAP

You are the implementation agent for **AvLoop — the seamless loop maker for OBS**, programme `AVLOOP`. Read
`AUTONOMOUS_STATE.md` first, then `EXECUTOR-PREAMBLE.md`, `PROGRESS.json` and `CONTINUE.md`.

## Launch boundary

**STARTED 2026-10-26** on the owner's specification (the word to code, spec §2.8) and the owner's "start". Build
what `README.md` §2 says and nothing more; the readings are §3; what is not built is §4 — **no audio, no output above
1080p, no rotation tool, colour correction, text or layers, no second crop, no crop keyframes, no macOS or Linux, no
updater, licence key, telemetry or code signing**. The order: AV-001 → AV-002 → … → AV-010 → AV-015 (CP-A1) → AV-011 →
AV-012 → AV-013 → AV-014 → AV-016 (CP-B1) (`control/01`). Each phase is committed `[AV-NNN]` and **pushed to
`origin` `main` at its close**, its CI watched green. **Autonomous mode** (spec §2.8): the owner is never asked; every
assumption is a reading (`README.md` §3, `DECISIONS.md`), every owner row closed by the agent (`OWNER-ACTIONS.md`).

## Location and reading budget

- Repository: the working copy of `https://github.com/rubeldbc/avLoop` (public), branch `main`, remote `origin`.
  Programme: `phases/26-10-26-034934-AvLoop`. Planning baseline: `d98849f` (*Initial commit*, a README).
- Specification: `docs/SPEC.md` — §0 takes **D-001 … D-014**, newest first, delivered behaviour only; the rest is
  the owner's text. The mock: `docs/mock/avloop-gui-mock.html` (the look; not code to copy).
- The lore: `docs/lore/avloop.md` — *Things that bite*, the standing rules for traps found (from AV-001).
- `CLAUDE.md` at the root: the agent's rules (the 600-line rule, the close procedure, the walks).
- Tools on this PC (`AUTONOMOUS_STATE.md`, *The lab here*): PowerShell 5.1 and Git Bash; Visual Studio 2022 v143;
  CMake 4.4; Ninja (Visual Studio's); Qt 6.8.3 by aqtinstall; Node 24; Python 3.14; `gh` 2.103; Inno Setup 6;
  FFmpeg 7.1.5 LGPL shared fetched by `scripts\fetch-deps.ps1`; a GeForce RTX 2060 SUPER, 12 logical cores, 32 GB.
  **A Bash heredoc eats backslashes: a script is written with the Write tool.**

## Rules that bind every phase

- **Per-phase procedure** (`EXECUTOR-PREAMBLE.md` §3): read the state, the preamble, the progress file, the active
  contract and its `read_scope`; check the working tree and the sizes of the files to be touched; a file a change
  would take past 600 is split at its seam first; one coherent slice; over 150% of `code_budget` needs a recorded
  deviation or a split; three distinct root-cause attempts on one failure need an explicit decision; never weaken a
  gate.
- **Close** (`EXECUTOR-PREAMBLE.md` §4): focused tests, the gates the change touches, the phase's own walk (never
  deferred to a checkpoint), `check-loc` PASS, `check-secrets` PASS; the execution record; `close-phase.mjs`; commit
  `[AV-NNN]` with `git commit -F`, explicit paths; `git fetch`, push; `gh run watch` green; README current.
- **Evidence**: what was observed — the build's line, the tests' counts, ffprobe's fields, the pictures with their
  look described, the log's lines; never only "passed".
- **The memory's rule** (spec §3.7): every cache and queue bounded; RAII; lifetimes released; the memory walk flat.
- **The public repository** (spec §2.9): no secret, personal path, email or token; large binaries ignored; test videos
  made by `make-samples.mjs`, never committed.
- **A walk leaves nothing behind**: no avloop process, no session cache, no temp file; pictures and logs stay in
  `execution/AV-NNN-walk/`, each described.
