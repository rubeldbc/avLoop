# AV-021 — Execution Record

**Phase:** AV-021 — The release 1.1.0 (owner item 19, late)
**Status:** DONE
**Date:** 2026-10-27
**Phase start commit:** `81e425c` (*[AV-020] The colour theme: replace a colour (orange to red), colourise, split
toning, duotone*)
**ADR:** none owed
**Environment class:** local-disposable (this PC: `build/`, `deploy/`, `installer/Output/` — git-ignored; no install:
the owner's own AvLoop 1.0.1 is installed per user and open, R-75)

## The owner's words

*"pull everyhitng from main and create a installer"* — main pulled (`b4e9ec4` → `81e425c`, AV-018 … AV-020), and an
installer built from it, still 1.0.1 by the version, named apart: `AvLoop-1.0.1-81e425c-setup.exe` (local, not
released; the owner installed it). Then: *"bump to 1.1.0 and publish a release"*.

## What was built

1. `CMakeLists.txt` — `project(AvLoop VERSION 1.1.0 …)`: the one place; avloop.exe's VERSIONINFO, `AppInfo::version()`
   and the installer's name follow it (R-74).
2. `execution/AV-021-walk/walk-release.mjs` — the version through the build, the deployment and the installer, the
   installer's SHA-256 written beside it; the owner's own install read before and after, never touched (R-75).
3. `execution/AV-017-walk/walk-about.mjs`, `walk-name.mjs` — the version read from the root `CMakeLists.txt` (R-76).
4. `README.md` — v1.1.0, what is new since 1.0.1, the Download link and file name, version 2 files and 1.0.x.
5. The programme: item 19 (`README.md` §1–§3), `control/22_SHAPES_THE_RELEASE_1_1_0.md`, this phase, `OWNER_ITEMS`
   19, REQ-AV-026, D-019, R-74 … R-76, owner actions 41–42, risk 19, a waiting leg (the owner's own upgrade).

## Decisions recorded

- **D-019** — the release 1.1.0; spec §0.
- **R-74** 1.1.0 a minor step, the Release `v1.1.0` as R-5 and R-62, latest; **R-75** no silent install over the
  owner's own AvLoop — the installer checked by its details and SHA-256; **R-76** the walks read the version.

## Specification delta

- §0: `D-019` — the release 1.1.0. The lore: *The release 1.1.0 (late)* — three rules.

## Verification

| Command | Result |
| --- | --- |
| `scripts\build.ps1 -Config Release` | built, 0 warnings (`/W4 /WX`; the version's 62 steps, 6 s) |
| `ctest --test-dir build --output-on-failure --timeout 300 -R avloop` | **24 passed**, 0 failed, 82 s |
| `scripts\format.ps1 -Check`; `node tools/icons/check-icons.mjs` | 225 files formatted; 44 icons, avloop.ico 7 sizes |
| `node tools/check-secrets.mjs` | PASS (636 files) |
| `node control/phase-tools/check-loc.mjs` | PASS: 282 code files (the largest `src/core/ClipEffects.cpp`, 425 lines) |
| `node control/phase-tools/validate-phase-system.mjs` | PASS: 21 phases, 26 requirement ids |
| `node control/phase-tools/check-continuity.mjs` | WORKING: AV-021 is IN_PROGRESS (before the close) |
| AV-020's CI run | green, 6 m 0 s (40404255002) |

## The walk

```
node control/lab/walk-phase.mjs execution/AV-021-walk walk-release.mjs → PHASE WALKS PASSED (31 s)
walk-release.mjs: CMakeLists.txt project(AvLoop VERSION 1.1.0); the owner's own AvLoop installed here: 1.0.1 (left
  as it is); build\bin\avloop-cli version → "AvLoop 1.1.0"; deploy.ps1 -NoBuild → AvLoop 1.1.0, 37 files, 141.4 MB;
  deploy\avloop.exe: AvLoop by Rubel | Md. Kamrul Islam Rubel | 1.1.0 | 1.1.0; deploy\avloop-cli.exe version →
  "AvLoop 1.1.0"; ISCC → AvLoop-1.1.0-setup.exe 44.8 MB, its details AvLoop by Rubel | Md. Kamrul Islam Rubel |
  1.1.0 | 1.1.0; SHA-256 220a3a236ccb5633a7d004bb1f271e2785d8c3a24e76b87e85483dc7896ef252 written to
  AvLoop-1.1.0-setup.exe.sha256; Apps & features still 1.0.1 after. 10 PASS. No process, cache or temp left.
```

## The pictures

| Picture | What | What it shows — and what was seen in it |
| --- | --- | --- |
| `AV-021-walk/about-navy.png` | About, Navy, from AV-017's walk again | logo, "AvLoop by Rubel", **Version 1.1.0**, the purpose, MIT · the repository in orange; Developer & contact's five rows, three links orange; Built with (Qt 6.8.3, FFmpeg n7.1.5, libvpx, dav1d, zimg, Windows); the footer; Close |

## The regressions

| Walk of before | Result | Mended |
| --- | --- | --- |
| `AV-017-walk/walk-about.mjs` | PASSED (6 s): "the version: as written" — Version 1.1.0; 21 PASS | the version read from `CMakeLists.txt` |
| `AV-017-walk/walk-name.mjs`, `AV-014-walk/walk-install.mjs` | not run (R-75): they install and uninstall under the AppId the owner's own AvLoop holds | `walk-name.mjs` reads the version too |

Logs in `AV-021-walk/regressions/`; AV-017's pictures and log put back as its phase left them.

## External freshness

- Not applicable — no dependency changed (Qt 6.8.3, FFmpeg n7.1.5, Inno Setup 6 as at AV-017).

## Files and line counts

| File | Before | After |
| --- | --- | --- |
| `CMakeLists.txt` | 1.0.1 | **1.1.0** (line 3) |
| `execution/AV-021-walk/walk-release.mjs` | — | **73** |
| `execution/AV-017-walk/walk-about.mjs` / `walk-name.mjs` | 75 / 112 | **77 / 113** |
| `control/22_SHAPES_THE_RELEASE_1_1_0.md` | — | **33** |

## Recovery log

Attempt 1 RESOLVED — walk-release: avloop-cli.exe has no VERSIONINFO (only avloop.exe takes `avloop.rc`); its check
is its `version` line. Attempt 1 RESOLVED — the mend of AV-017's walks through a Bash `node -e` lost its regex's
backslashes (the lore's heredoc rule); written again with the Edit tool.

## Deviations from the phase contract

The install walks were not run: the owner's own AvLoop shares the AppId (R-75); their upgrade is a waiting leg.

## Blocked legs (if any)

None.

## Staged paths

The paths above, `docs/SPEC.md`, `docs/lore/avloop.md`, `README.md`, the programme's files.

## Next

None — every phase closed again. The Release `v1.1.0` on this commit once its CI is green; the asset's SHA-256 read
back.
