# AV-014 — Execution Record

**Phase:** AV-014 — The installer (owner item 14)
**Status:** DONE
**Date:** 2026-10-26
**Phase start commit:** `c68cc7e` (*[AV-013] The batch and the other formats: GIF, APNG, MP4, ProRes 4444 and a queue*)
**ADR:** none owed
**Environment class:** local-disposable (this PC: `build/`, `deploy/`, `installer/Output/` (git-ignored); the walk's
per-user install into a temporary folder, its registry keys and Start menu entry, all removed by its uninstall; the
owner's AvLoop folders copied aside and put back)

## The owner's words

*"A Windows installer: Start menu, the .avloop association, an uninstaller."*

## Environment verified first

Inno Setup 6 (`C:\Program Files (x86)\Inno Setup 6\ISCC.exe`), Qt 6.8.3's `windeployqt`, Visual Studio 2022's
`VC\Redist\MSVC\*\x64\Microsoft.VC143.CRT` — all present.

## What was built

1. `scripts/deploy.ps1` (Release build → `deploy\`: the two programs, windeployqt's Qt without translations, software
   OpenGL or network, the FFmpeg DLLs, the VC++ runtime app-local, `licenses\`) and the `deploy` target in
   `cmake/Deploy.cmake`.
2. `installer/AvLoop.iss` (Inno Setup 6: per user by default, every user by choice; the licence and the notices shown;
   Start menu, optional desktop icon; `.avloop` → `AvLoop.Project` with the app's icon; uninstall removes
   `%LOCALAPPDATA%\AvLoop` and `%LOCALAPPDATA%\cache\AvLoop`, keeps `%APPDATA%\AvLoop`; not signed).
3. The version: `resources/app/avloop.rc` gains VERSIONINFO from AvLoopVersion.h (the root CMakeLists.txt); the
   installer reads it from `deploy\avloop.exe` (`/DAppVersion=` overrides); the About box already showed it.
4. `licenses/LGPL-3.0.txt` and `licenses/GPL-3.0.txt` (the FSF's texts); `LICENSE-THIRD-PARTY.md` gains OpenH264 and
   the VC++ runtime, and says where the texts are installed.
5. Found on the way and mended: av-lab looked for the disk cache's sessions in `%LOCALAPPDATA%\AvLoop\cache`; the
   cache lives in `%LOCALAPPDATA%\cache\AvLoop\cache` — walk-phase's "no session left" now reads the real folder (every
   walk rerun and clean).

## Decisions recorded

- **D-014** — the installer as delivered; spec §0.
- **R-56** install scope (per user by default, every user by choice); **R-57** what uninstalling removes and keeps;
  **R-58** no network module, the VC++ runtime app-local, the version from the exe.

## Specification delta

- §0: `D-014`. Lore: *The installer* — four rules.

## Verification

| Command | Result |
| --- | --- |
| `scripts\build.ps1` | built, **0 warnings** (the .rc with VERSIONINFO: ProductVersion 0.1.0) |
| `ctest` | **23/23 PASS** |
| `scripts\deploy.ps1 -NoBuild` | 37 files, 141.2 MB; `deploy\avloop-cli.exe version` runs with only Windows on PATH |
| `ISCC installer\AvLoop.iss` | `AvLoop-0.1.0-setup.exe`, 44.8 MB, 31 s |
| `scripts\format.ps1 -Check` | PASS (217 files) |
| `node tools/icons/check-icons.mjs` | PASS: 43 icons |
| `node tools/check-secrets.mjs` | PASS (513 files) |
| `node control/phase-tools/check-loc.mjs` | PASS (269 code files; the largest `src/ui/MainWindow.cpp`, 389 lines) |
| AV-013's CI run | green, 8 m 13 s |

## The walk

```
node control/lab/walk-phase.mjs execution/AV-014-walk walk-install.mjs
walk-install.mjs (49 s): deploy.ps1 (37 files, 141.2 MB; the VC++ runtime, no Qt6Network, no FFmpeg tools); ISCC →
  AvLoop-0.1.0-setup.exe 44.8 MB; /VERYSILENT /CURRENTUSER /DIR=<temp>: 33 entries, the four licence files;
  HKCU .avloop → AvLoop.Project → "…\app\avloop.exe" "%1", its icon "…\avloop.exe",0; AvLoop.lnk in the Start menu;
  Apps & features: AvLoop 0.1.0; the installed avloop.exe --shot with only Windows on PATH — picture; the
  association's command run with a project: title "AvLoop — opened-by-double-click.avloop", crop ellipse — picture;
  unins000 /VERYSILENT: the folder, the association, the Start menu entry, the uninstall entry, %LOCALAPPDATA%\AvLoop
  and %LOCALAPPDATA%\cache\AvLoop gone; the marker in %APPDATA%\AvLoop kept
PHASE WALKS PASSED
```

## The pictures

| Picture | What | What it shows — and what was seen in it |
| --- | --- | --- |
| `AV-014-walk/installed.png` | the installed app | the main window maximized, the drop zone, the tool rail's and the panels' SVG icons drawn (the svg plugins deployed), "No video open", RAM 155 MB |
| `AV-014-walk/opened.png` | the association | counter-640p30.mp4 open from the project: the ellipse crop 400×280 · alpha, Loop Simple 3.00 s, Output Auto (on), the In–Out range 2–5 s on the timeline |

## The regressions

| Walk of before | Result | Mended |
| --- | --- | --- |
| AV-001 … AV-013, AV-015 (every walk, with the session check now reading the real cache folder) | PASSED; no session left | none |

Their pictures and logs were restored from git.

## External freshness

Inno Setup 6 as installed on this PC; the VC++ runtime from Visual Studio 2022's current redistributable folder.

## Files and line counts

New: `scripts/deploy.ps1` (68), `installer/AvLoop.iss` (72), `licenses/LGPL-3.0.txt`, `licenses/GPL-3.0.txt`,
`execution/AV-014-walk/walk-install.mjs` (119). Changed: `resources/app/avloop.rc` (36), `cmake/Deploy.cmake`,
`LICENSE-THIRD-PARTY.md`, `README.md`, `control/lab/av-lab.mjs`.

## Recovery log

Attempt 1 RESOLVED — gate: the deployment — windeployqt brought Qt6Network with the TUIO touch plugin, and its TLS and
network-information plugins: left out (`--no-network`, `--skip-plugin-types generic,tls,networkinformation`).
Attempt 2 RESOLVED — gate: deploy.ps1's parse — `"$bin:"` is a scoped variable: `${bin}`.

## Deviations from the phase contract

- The walk launches the installed app with `--shot` and then the association's command with a walk (the project's
  title and crop read by probes rather than seen in a picture's title bar: a grab shows the client area only).

## Blocked legs (if any)

None.

## Staged paths

- `scripts/deploy.ps1`, `installer/AvLoop.iss`, `licenses/`, `resources/app/avloop.rc`, `cmake/Deploy.cmake`,
  `LICENSE-THIRD-PARTY.md`, `README.md`, `docs/SPEC.md`, `docs/lore/avloop.md`, `phases/26-10-26-034934-AvLoop/`.

## Next

AV-016 — CP-B1 the whole walked, promoted to IN_PROGRESS in the same turn. No avloop process, session cache, lab
folder or walk temp file left; the owner's focus untouched; the owner's registry and folders as they were.
