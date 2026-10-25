# AV-001 — Execution Record

**Phase:** AV-001 — The ground (owner item 1)
**Status:** DONE
**Date:** 2026-10-26
**Phase start commit:** `bc5de3e` (*[AV] Plan*)
**ADR:** none owed (AVADR-0002 is AV-002's, AVADR-0001 AV-007's)
**Environment class:** local-disposable (this PC: the build in `build/`, the walk's folder; lab settings in a temp folder)

## The owner's words

*"Compact GUI. Orange with black or navy blue theme. The app draws its own icons."* — *"'Full screen' means a
maximized window, not true full screen."* — *"No code file over 600 lines."* — *"Auto-push … Public repository; anyone
can contribute. The README is always professional and up to date, with app screenshots."*

## What was built

1. `CMakeLists.txt`, `cmake/Warnings.cmake` (`/W4 /WX /permissive- /utf-8 /Zc:preprocessor`, external headers at W0,
   ASan option), `cmake/Qt.cmake` (Qt 6.8 components, the Qt prefix for tests), `cmake/FindFFmpeg.cmake` (imported
   targets from `third_party/ffmpeg`), `cmake/Deploy.cmake` (FFmpeg DLLs beside binaries; the tests' environment).
2. `scripts/fetch-deps.ps1` (pin + SHA-256 + newest-7.1 fallback, `VERSION.txt`), `build.ps1` (vswhere → vcvars64,
   Ninja, Qt finder, windeployqt once, `Invoke-Native` for PS 5.1), `test.ps1`, `format.ps1` (`-Check`).
3. `src/core/AppInfo` + `Version.h.in`; `src/ui/theme/Theme` (the token table, `expand`, the stylesheet, palette,
   switch), `IconFactory` (a themed `QIconEngine`: four states, per-DPR render and cache; the list of 43 icons; the app
   icon); `resources/themes/app.qss` + `navy.qss` + `black.qss`; `resources/icons/*.svg` (43); `resources/app/avloop.svg`,
   `avloop.ico`, `avloop.rc`; `resources/resources.qrc`.
4. `src/ui/MainWindow` (+ `MainMenu.cpp`): top bar, rail (tool keys as window actions), canvas stack with
   `ui/canvas/DropZone`, `ui/panels/SidePanel` + `Section`, the timeline area with `ui/timeline/Transport`, the status
   bar; layout saved and restored; probes for walks. `ui/widgets/Toast`.
5. `src/ui/WalkRunner` + `WalkSteps.cpp`: the lab's GUI driver — click, dclick, drag, wheel, key, type, shot, expect,
   waitFor, menu, theme, resize, maximize; probes and mappers; extension steps for later phases.
6. `src/app/main.cpp` (`--shot`, `--walk`, `--walk-out`, `--theme`, `--tool`, `--scale`; lab settings in a temp dir),
   `src/cli/main.cpp` (`avloop-cli version`, the licence gate), `tools/icons/iconbake.cpp` (SVG → PNG-entry `.ico`).
7. `tests/test-theme.cpp` (9 checks), `tests/test-icons.cpp` (6 checks); `tools/check-secrets.mjs`,
   `tools/icons/check-icons.mjs`; `tools/walks/readme-shots.json`.
8. `LICENSE` (MIT), `LICENSE-THIRD-PARTY.md`, `CONTRIBUTING.md`, `CODE_OF_CONDUCT.md` (Contributor Covenant 2.1),
   `SECURITY.md`, `.github/ISSUE_TEMPLATE/{bug,feature}.yml`, `PULL_REQUEST_TEMPLATE.md`, `.github/workflows/ci.yml`,
   `.clang-format`, `.gitignore`, `.gitattributes`, `CLAUDE.md`, `docs/lore/avloop.md`, `README.md` v1,
   `docs/screenshots/main-window.png`.

## Decisions recorded

- **D-001** — the ground as delivered; spec §0.
- **R-1** (English, `tr()`), **R-4** (MIT) taken as read.
- **Undo and Redo stand in the ☰ menu** (disabled until AV-003): the owner's mock shows them there and the icons `undo`,
  `redo` exist for them; §5.1's list does not name them — a reading, open for a veto.
- **`resources/themes/app.qss` added** beside `navy.qss` and `black.qss`: one shared template, each theme file holding
  only its own rules, so no rule is written twice and both stay token-only (§4 lists the two theme files).
- **`ui/canvas/DropZone`** and **`ui/WalkSteps.cpp`** are files the §4 tree does not name: the drop zone is the canvas
  area's content until AV-003's `Canvas`; the walk's steps split from `WalkRunner.cpp` to keep both under 300 lines.
- **Open, Preview, Export and the transport are disabled** until AV-002 makes opening a video possible — an honest
  shell rather than buttons that do nothing.
- **The token `accentLine`** (accent at 35%) and **`onAccent`** (#111) join the table for the checked tool's outline and
  the text on the orange button, so the QSS needs no literal colour.
- **Tool keys are window `QAction`s**, not the buttons' own shortcuts (which animate a click first; `docs/lore`).
- **The lab's settings live in a temporary folder** during `--shot`/`--walk`, never in the owner's `AvLoop.ini`.

## Specification delta

- §0: `D-001` — the ground (the build, the shell, the themes, the icons, the lab's entry points).
- `CLAUDE.md` written (the hard rules, the close, the code rules, this PC); `docs/lore/avloop.md` started (eight traps).

## Verification

| Command | Result |
| --- | --- |
| `scripts\build.ps1 -Fresh -Config Release` | built with MSVC 14.44.35207 + Ninja 1.12.1: 68 steps, **0 warnings** (`/W4 /WX`) |
| `ctest --test-dir build --output-on-failure --timeout 300 -R avloop` | **2 passed** (avloop-theme 9 checks, avloop-icons 6), 0 failed, 0.24 s |
| `scripts\format.ps1 -Check` | PASS: 27 files formatted (clang-format 19.1.5) |
| `node tools/icons/check-icons.mjs` | PASS: 43 icons, 43 listed, avloop.ico with 7 sizes |
| `node tools/check-secrets.mjs` | PASS: 169 files, nothing secret or personal |
| `node control/phase-tools/check-loc.mjs` | PASS: 53 code files, none over 600 lines (the largest: `src/ui/MainWindow.cpp`, 301 lines) |
| `build\bin\avloop-cli.exe version` (licence gate) | `licence: LGPL version 3 or later`; `--enable-version3`, `--disable-libx264 --disable-libx265`, no `--enable-gpl`; PASS |
| `node control/phase-tools/validate-phase-system.mjs` | PASS (run by close-phase.mjs) |
| `git push origin main`; `gh run watch` | see the commit `[AV-001]` and its CI run (recorded at the next close if red) |

## The walk

```
node control/lab/walk-phase.mjs execution/AV-001-walk walk-shell.mjs walk-shell.json
walk-shell.mjs (Node, 10 s):
  avloop-cli version → "PASS: LGPL build", licence "LGPL version 3 or later"
  avloop.exe --shot twice → window shown after 189 ms, then 184 ms (second start < 2 s); the process 1.1 s incl. the
  800 ms settle before the grab
  --shot --theme navy|black --scale 1|1.5|2 → six pictures; the canvas pixel at 45%,25% reads rgb(14,23,38) (Navy bg
  #0E1726) and rgb(11,11,12) (Black bg #0B0B0C)
walk-shell.json (avloop.exe --walk, real QTest key clicks and synthetic mouse events on the widgets, 7 s, 52 steps):
  maximized true; top-bar buttons "Open,Preview,Export"; title "AvLoop"; 43 icons, none missing; 9 rail tools; the drop
  zone visible; status "No video open"
  keys M U E H Z I O K V → crop-rect, crop-round, crop-ellipse, hand, zoom, in, out, keyframe, select (each read back)
  a click on tool.crop-ellipse → crop-ellipse
  ☰ → Theme → Black (the menu's own popup pressed) → theme "black"; → Navy → "navy"
  the Crop section's header clicked → closed (picture), clicked → open
  resize 1280×720 → "1280x720" (the minimum), maximize → maximized
No avloop process, session cache or temp file left (walk-phase.mjs: PHASE WALKS PASSED).
```

## The pictures

| Picture | What | What it shows — and what was seen in it |
| --- | --- | --- |
| `AV-001-walk/s1-navy-100.png` | main window, Navy, 100% | the mock's layout: dark navy canvas with the dashed drop zone centred, the rail's nine icons in four groups with the Selection tool orange, the side panel's preset row and four open sections, the transport row with orange time; only Open (disabled), Preview, Export in the top bar |
| `AV-001-walk/s1-black-100.png` | main window, Black, 100% | the same in near-black panels; the orange accents unchanged; text contrast holds |
| `AV-001-walk/s1-navy-150.png`, `s1-navy-200.png`, `s1-black-150.png`, `s1-black-200.png` | 150% and 200% | icons, text and the dashed zone sharp (rendered per DPR, no scaled bitmaps); the rail separators visible at 200%; at this 1080p monitor 200% fits a 960×505 logical window, so the panels crowd — expected at that scale on 1080p |
| `AV-001-walk/main-navy.png`, `main-black.png` | the theme switched by the menu in one run | colours change in place without a restart |
| `AV-001-walk/rail-ellipse-active.png` | the rail with Crop: Ellipse active | the ellipse icon orange on the accentSoft fill with its 35% outline; the others text2 |
| `AV-001-walk/side-crop-closed.png` | the Crop section closed | the chevron turned right, the body hidden, the other three sections moved up |
| `AV-001-walk/min-1280x720.png` | the window at its minimum | everything fits; the side panel keeps 260 px |
| `docs/screenshots/main-window.png` | README | the Navy window at 1600 px wide, 46 KB |

Weak: the disabled Open video… button in the drop zone reads as a dim block until AV-002 enables it.

## The regressions

| Walk of before | Result | Mended |
| --- | --- | --- |
| — | none: the first phase | — |

## External freshness

- Visual Studio 2022 Build Tools 17.14 (MSVC 14.44.35207) — chosen by vswhere `-latest` — 2026-10-26 — this PC.
- CMake 4.4.0; Ninja 1.12.1 (Visual Studio's); clang-format 19.1.5 (Visual Studio's LLVM) — 2026-10-26 — this PC.
- Qt 6.8.3 `win64_msvc2022_64` — aqtinstall 3.3.0 — 2026-10-26 — the last open-source 6.8 patch (R-10).
- FFmpeg `n7.1.5-12-g1fdbca85aa` LGPL shared (BtbN autobuild-2026-07-31-14-10), SHA-256 `0f376f96…c5d0` — 2026-10-26.
- Node 24.12.0; Python 3.14.0; `gh` 2.103.2; Inno Setup 6.7.3 — 2026-10-26 — winget / this PC.
- CI: `actions/checkout@v4`, `actions/setup-node@v4` (Node 20), `jurplel/install-qt-action@v4`, `actions/cache@v4`.

## Files and line counts

| File | Before | After |
| --- | --- | --- |
| `src/ui/MainWindow.cpp` | — | **301** |
| `src/ui/WalkSteps.cpp` | — | **272** |
| `resources/themes/app.qss` | — | **270** |
| `src/ui/theme/IconFactory.cpp` | — | **213** |
| `src/ui/WalkRunner.cpp` | — | **177** |
| `src/ui/theme/Theme.cpp` | — | **156** |
| `src/app/main.cpp` | — | **105** |
| every other new file | — | under 100 |

## Recovery log

Attempt 1 RESOLVED — gate: build; cause: `vcvars64.bat`'s stderr ("vswhere.exe is not recognized") became a
terminating error in PowerShell 5.1; repair: `>nul 2>&1` and `Invoke-Native`; result: built.
Attempt 2 RESOLVED — gate: ctest; causes: a token in a QSS comment, an over-strict blank-icon threshold; repair: the
comment reworded, the threshold 100; result: 2 passed.
Attempt 3 RESOLVED — gate: the walk; cause: tool keys lagged 100 ms (a button's shortcut animates a click); repair: window
actions; result: PASSED. (Three distinct causes on three gates — not three attempts on one failure.)

## Deviations from the phase contract

- **Code budget**: 600 planned; about 2,610 hand-written C++ lines and 960 lines of CMake, scripts, QSS, workflows and
  tools came. The contract asks in one phase for the build, the shell, both themes, the icon system with 43 icons, the
  lab's GUI driver (`--walk`, used by every later phase) and the CLI; the split named by the contract (AV-001a: the
  open-source files, CI, README) would move documents, not code. Recorded instead of split; nothing outside the
  contract's **Do** was built.

## Blocked legs (if any)

None.

## Staged paths

- `.clang-format`, `.gitattributes`, `.gitignore`, `.github/`, `CLAUDE.md`, `CMakeLists.txt`, `CODE_OF_CONDUCT.md`,
  `CONTRIBUTING.md`, `LICENSE`, `LICENSE-THIRD-PARTY.md`, `README.md`, `SECURITY.md`, `cmake/`, `docs/SPEC.md`,
  `docs/lore/`, `docs/screenshots/`, `resources/`, `scripts/`, `src/`, `tests/`, `tools/`,
  `phases/26-10-26-034934-AvLoop/` (the record, the walk's folder, the state files, the lab's walk-phase.mjs).

## Next

AV-002 — The media, promoted to IN_PROGRESS in the same turn. No avloop process, session cache or walk temp file left.
