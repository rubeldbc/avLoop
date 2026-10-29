# AV-NNN — Execution Record

**Phase:** AV-NNN — <title> (owner item N | checkpoint)
**Status:** DONE | BLOCKED_EXTERNAL | DEFERRED | SPLIT
**Date:** YYYY-MM-DD
**Phase start commit:** <hash>
**ADR:** AVADR-NNNN accepted before the code it governs, indexed in `adr/README.md` and `DECISIONS.md` — or "none owed"
**Environment class:** local-disposable (this PC: the build in `build/`, the samples in `samples/`, the walk's folder)

## The owner's words

*"<the item, verbatim from README §1>"*

## What was built

1. `<path>` — what it is, what it exports, the split axis or "one concern".
2. …

## Decisions recorded

- **D-NNN** — <one line>; spec §0. (or "see AVADR-NNNN".)
- **<Local decision>** — default kept / changed to X because <evidence>.

## Specification delta

- §0: `D-NNN` — <one line>.
- §<section>: <what changed>; `CLAUDE.md` / the lore: <the standing rule written, if one was>.

## Verification

| Command | Result |
| --- | --- |
| `scripts\build.ps1 -Config Release` | built, 0 warnings (<N> targets, M s) |
| `ctest --test-dir build --output-on-failure --timeout 300` | **N passed**, 0 failed, M s |
| `scripts\format.ps1 -Check`; `node tools/icons/check-icons.mjs` | clean / N icons PASS |
| `node tools/check-secrets.mjs` | PASS (N files) |
| `node control/phase-tools/check-loc.mjs` | PASS (the largest code file: `<path>`, N lines) |
| `node control/phase-tools/validate-phase-system.mjs` | PASS: 16 phases, … |
| `node control/phase-tools/check-continuity.mjs` | WORKING: … / FINISHED: … |
| `git fetch origin`; `git push origin main`; `git status -sb` | pushed: `## main...origin/main` (`<hash>`) |
| `gh run watch <id>` | CI green (<run id>, M min) |

Gate rows, one line each, with the observed value (not "passed"), every run under five minutes.

## The walk

```
<step by step: which binary, which sample or project, what was pressed and how (QTest events on which widget), what
the log said, what ffprobe and av-lab read, what the pictures showed where, their names; no process, cache or temp
file left>
```

## The pictures

| Picture | What | What it shows — and what was seen in it |
| --- | --- | --- |
| `AV-NNN-walk/<name>.png` | the main window, Navy, 100% | <read in plain words: what is right, what is weak> |

## The regressions

| Walk of before | Result | Mended |
| --- | --- | --- |
| `<walk>` | PASSED / FAILED: <why> | <what it reads anew> — or "none" |

## External freshness

- `<dependency> — <version observed> — <YYYY-MM-DD> — <source>` or `Not applicable — <reason>`.

## Files and line counts

| File | Before | After |
| --- | --- | --- |
| `<path>` | — | **N** |

## Recovery log

None — no gate failure occurred.
(or) Attempt 1 RESOLVED — gate: …; cause: …; repair: …; result: …

## Deviations from the phase contract

None. (or) What was widened/narrowed, why it was inseparable, and what was deferred to which phase.

## Blocked legs (if any)

- `<exact command to resume>` — capability missing: <what>.

## Staged paths

- `<explicit path>` … (committed and pushed to `origin` `main`, spec §2.9)

## Next

AV-NNN+1 — <title>, promoted to IN_PROGRESS in the same turn. No avloop process, session cache or walk temp file left.
