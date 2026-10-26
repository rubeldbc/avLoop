# AV-017 — Execution Record

**Phase:** AV-017 — The name and the About box (owner item 15, late)
**Status:** DONE
**Date:** 2026-10-26
**Phase start commit:** `3bce220` (*[AV-016] CP-B1: the whole walked on the installed 1.0.0; the programme complete*)
**ADR:** none owed
**Environment class:** local-disposable (this PC — a second one: the repository at `D:\Code5\avLoop`, Qt 6.8.3 at
`C:\Qt`, Inno Setup 6 per user under `%LOCALAPPDATA%\Programs`, an RTX 5070 —: `build/`, `deploy/`,
`installer/Output/` (git-ignored); the walks' per-user installs into temporary folders, removed by their uninstalls;
the owner's AvLoop folders copied aside and put back)

## The owner's words

*"The applicaiton name will be AvLoop by Rubel. There will be a about section where my information will be available.
You can get the information from glayer.app which is in my repo"*; then *"Email: [the owner's address], website not
required."* Told that the phone number and the email become public with the push, the owner's next word: *"continue"*.

## What was built

1. `src/core/AppInfo.{h,cpp}` — `displayName()` "AvLoop by Rubel"; `name()`/`organization()` stay "AvLoop" (R-59).
2. `src/ui/MainWindow.cpp`, `MainWindowProject.cpp`, `MainWindowExport.cpp`, `MainMenu.cpp` — the title, the top bar,
   "About AvLoop by Rubel", the questions' titles by `displayName()`; the `about.text` probe.
3. `src/ui/dialogs/AboutDialog.cpp` — the name, the version, the purpose, the licence and the repository link;
   *Developer & contact* (developer, country, email, profile, WhatsApp; links open outside); *Built with*; the footer.
   `src/ui/theme/Theme.cpp` — `QPalette::Link` the accent.
4. `resources/app/avloop.rc` — ProductName, FileDescription, CompanyName, LegalCopyright. `CMakeLists.txt` — 1.0.1.
5. `installer/AvLoop.iss` — AppName, AppVerName, AppPublisher, AppCopyright, the group, the shortcuts, Apps & features,
   "Launch"; `[InstallDelete]` of 1.0.0's `AvLoop.lnk`. Folder, AppId and association unchanged.
6. `tools/check-secrets.mjs` — the owner's address allowed in `AboutDialog.cpp` alone (R-61).
7. `src/ui/WalkSteps.cpp`, `WalkRunner.h` — the `menu` step looks again for its menu for two seconds (lore).
8. Walks: `AV-017-walk/walk-about.mjs`, `walk-name.mjs`; the old name read anew by `AV-001-walk/walk-shell.json` (and
   `waitFor` maximized), `AV-010-walk/walk-settings.json`, `walk-project.mjs`, `AV-014-walk/walk-install.mjs` (the
   new shortcut; ISCC found per user too).
9. The programme: item 15 (`README.md` §1–§3), `control/18_SHAPES_THE_NAME_AND_ABOUT.md`, this phase, `OWNER_ITEMS`
   15, REQ-AV-022, D-015, R-59 … R-62, owner actions 33–34, risk 17.

## Decisions recorded

- **D-015** — the name and the About box, v1.0.1; spec §0.
- **R-59** the identity stays "AvLoop"; **R-60** About's details from the owner's gLayer About, the email the owner's
  correction, no website; **R-61** the gate's one address in one file, documents name it by its row; **R-62** 1.0.1
  and its Release, as R-5.

## Specification delta

- §0: `D-015` — the name and the About box, v1.0.1. The lore: *The name and the About box (late)* — four rules.

## Verification

| Command | Result |
| --- | --- |
| `scripts\build.ps1 -Config Release` | built, 0 warnings (incremental, the last 79 steps) |
| `ctest --test-dir build --output-on-failure --timeout 300 -R avloop` | **23 passed**, 0 failed, 76 s |
| `scripts\format.ps1 -Check`; `node tools/icons/check-icons.mjs` | 217 files formatted; 43 icons, avloop.ico 7 sizes |
| `node tools/check-secrets.mjs` | PASS (576 files) — 28 findings first, every one the address copied out of `AboutDialog.cpp` |
| `node control/phase-tools/check-loc.mjs` | PASS: 270 code files (the largest `src/ui/MainWindowProject.cpp`, 399 lines) |
| `node control/phase-tools/validate-phase-system.mjs` | PASS: 17 phases, 22 requirement ids |
| README pictures | `avloop.exe --walk tools/walks/readme-shots.json`: 7 retaken, 17–196 KB |
| AV-016's CI run | green, 8 m 16 s (40306748307) |

## The walk

```
node control/lab/walk-phase.mjs execution/AV-017-walk walk-about.mjs walk-name.mjs → PHASE WALKS PASSED
walk-about.mjs (6 s): the contacts read from AboutDialog.cpp (6 constants); wa.me = the shown number's digits;
  build/bin/avloop.exe --walk <made script>: title "AvLoop by Rubel"; ☰ → "About AvLoop by Rubel"; about.text holds
  the name, "Version 1.0.1", the repository link, the developer, Bangladesh, the mailto: link, the profile link, the
  wa.me link, the footer; no "glayer", no "Website"; empty once closed; Black theme, About again; 21 PASS.
walk-name.mjs (36 s): deploy.ps1 -NoBuild → 1.0.1, 37 files, 141.2 MB; avloop.exe: AvLoop by Rubel | Md. Kamrul
  Islam Rubel | AvLoop by Rubel - seamless looping videos for OBS | Copyright (C) 2026 … MIT License | 1.0.1;
  ISCC → AvLoop-1.0.1-setup.exe 44.8 MB, its own details the name and the publisher; a stand-in AvLoop.lnk;
  /VERYSILENT /CURRENTUSER into a temp folder: "AvLoop by Rubel.lnk" there, AvLoop.lnk gone; Apps & features
  "AvLoop by Rubel | Md. Kamrul Islam Rubel | 1.0.1"; .avloop → AvLoop.Project; the installed app (Windows alone on
  PATH) titled "AvLoop by Rubel"; uninstalled: folder, shortcuts, entry gone. No process, cache or temp left.
```

## The pictures

| Picture | What | What it shows — and what was seen in it |
| --- | --- | --- |
| `AV-017-walk/about-navy.png` | About, Navy | logo, "AvLoop by Rubel", Version 1.0.1, the purpose, MIT · the repository in orange; the five contact rows aligned, three links orange; Built with; the footer; Close. Background 14,23,38 |
| `AV-017-walk/about-black.png` | About, Black | the same box, background 11,11,12, links in the same accent |
| `AV-017-walk/topbar.png` | the top bar | ☰, the logo, "AvLoop by Rubel", Open, the Selection's options, Preview, Export |
| `AV-017-walk/installed.png` | the installed 1.0.1 | the maximized window titled by the name, the drop zone, the side panel |
| `docs/screenshots/*.png` | the README | main window, timeline, Preview, ellipse crop, export toast, batch, Settings — the top bar reads "AvLoop by Rubel" |

## The regressions

| Walk of before | Result | Mended |
| --- | --- | --- |
| `AV-001-walk/walk-shell.json` | FAILED twice in eight runs before the mend (window not yet maximized; "Theme" not found 250 ms after ☰), then PASSED | the title anew; `waitFor` maximized (3 s); the `menu` step looks again for 2 s |
| `AV-010-walk/walk-settings.json`, `walk-project.mjs` | PASSED | the title and the About entry anew |
| `AV-014-walk/walk-install.mjs` | PASSED (37 s) | the shortcut "AvLoop by Rubel.lnk"; ISCC per user too |

Logs in `AV-017-walk/regressions/`; the old folders' pictures and logs left as their phases wrote them.

## External freshness

- Not applicable — no dependency changed.

## Files and line counts

| File | Before | After |
| --- | --- | --- |
| `src/ui/dialogs/AboutDialog.cpp` | 53 | **110** |
| `src/ui/MainWindowProject.cpp` | 388 | **399** |
| `src/ui/WalkSteps.cpp` | 370 | **378** |
| `src/core/AppInfo.cpp` / `.h` | 19 / 14 | **23 / 16** |
| `installer/AvLoop.iss` | 72 | **81** |
| `tools/check-secrets.mjs` | 45 | **48** |
| `execution/AV-017-walk/walk-about.mjs` / `walk-name.mjs` | — | **75 / 112** |

## Recovery log

Attempt 1 RESOLVED — gate: check-secrets; cause: the address copied into the plan, the shapes, a walk's JSON and the
app's walk log; repair: documents name it by its row, the About walk reads it from the source and keeps the app's log
in temp; result: PASS. Attempt 1 RESOLVED — walk-name: Inno pads setup.exe's strings; trimmed.

## Deviations from the phase contract

The walk tool's `menu` step and the shell walk's maximize check were mended (regressions seen on this PC).

## Blocked legs (if any)

None.

## Staged paths

The paths above, `docs/SPEC.md`, `docs/lore/avloop.md`, `README.md`, `docs/screenshots/`, the programme's files.

## Next

None — every phase closed again. The Release `v1.0.1` on this commit; CI watched after the push.
