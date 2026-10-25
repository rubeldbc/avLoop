---
phase: AV-001
title: "The ground"
status: DONE
owner_item: 1
track: A
checkpoint_id: CP-A1
is_checkpoint: false
risk: Medium
permission_class: VERIFY_ENVIRONMENT_FIRST
code_budget: 600
depends_on: []
unlocks: [AV-002, AV-015]
requirements: [REQ-AV-001, REQ-AV-015, REQ-AV-019, REQ-AV-021]
read_scope:
  - phases/26-10-26-034934-AvLoop/control/03_SHAPES_THE_GROUND.md
  - phases/26-10-26-034934-AvLoop/control/04_THE_LAB_AND_THE_WALK.md
  - docs/SPEC.md
  - docs/mock/avloop-gui-mock.html
  - spec: §2.4, §2.6, §2.7, §2.9, §3.1, §3.2, §4, §5
---

# The ground

Canonical current execution state: `../AUTONOMOUS_STATE.md`

**Goal.** The repository stands and builds: the CMake project with its gates, the dependencies fetched by script, a
maximized main window with the §5 layout (top bar with only Open / Preview / Export, tool rail, canvas area with the
drop zone, side panel with four collapsible sections, timeline area, status bar), the Navy and Black themes from one
token table, every icon of §5.3 drawn by the app, the `--shot` / `--walk` lab entry points, the `avloop-cli` skeleton,
the open-source files, CI and README v1 with a real screenshot (`../README.md` §2 item 1; D-001).

**Ground truth**

- The spec §4 tree and §5 layout; the mock `docs/mock/avloop-gui-mock.html` (the owner's look, not a file to copy).
- This PC (`../AUTONOMOUS_STATE.md`, *The lab here*): Visual Studio 2022 (v143), CMake 4.4, Ninja from Visual Studio,
  Qt 6.8.3 `msvc2022_64` (aqtinstall), Node 24, FFmpeg 7.1.5 LGPL shared (BtbN), Inno Setup 6, `gh`.
- The repository `origin` holds one commit (*Initial commit*, a README); the plan's commit `[AV] Plan` follows it.

**Do**

1. Root `CMakeLists.txt`, `cmake/` (`Warnings.cmake`: `/W4 /WX /permissive- /utf-8`; `Qt.cmake`; `FindFFmpeg.cmake`;
   `Deploy.cmake`), `.clang-format` (LLVM, 120 columns), `.gitignore`, `CLAUDE.md`.
2. `scripts/fetch-deps.ps1` (pinned FFmpeg zip + SHA-256, a fallback search of the newest `n7.1` LGPL shared build),
   `build.ps1` (`-Fresh`, `-Config`; the MSVC environment, Ninja, windeployqt and FFmpeg DLLs into `build/bin`),
   `test.ps1`, `format.ps1` (`-Check`).
3. `src/core` stub library, `src/ui/theme/Theme` (token table → QSS with `@token` placeholders; `resources/themes/
   navy.qss`, `black.qss`), `src/ui/theme/IconFactory` (four states, per-DPI cache), `resources/icons/*.svg` (every
   name of §5.3), the app icon `avloop.ico` from one SVG, `src/ui/MainWindow*` (the §5.1 layout, maximized, min
   1280×720, splitters remembered), `MainMenu.cpp` (the ☰ menu's entries, those of later phases disabled),
   `ui/widgets/Toast`, `ui/panels/Section`, `WalkRunner.cpp` (`--shot`, `--walk`, `--walk-out`).
4. `src/app/main.cpp` (`avloop.exe`), `src/cli/main.cpp` (`avloop-cli version` prints the FFmpeg configuration and
   refuses `--enable-gpl`).
5. `tools/check-secrets.mjs`, `tools/icons/check-icons.mjs`; `LICENSE` (MIT), `LICENSE-THIRD-PARTY.md`,
   `CONTRIBUTING.md`, `CODE_OF_CONDUCT.md`, `SECURITY.md`, issue and PR templates; `.github/workflows/ci.yml`.
6. `tests/` with `test-theme` (tokens, QSS has no hex), `test-icons` (every listed icon renders in four states).
7. `README.md` v1 (logo, badges, pitch, real screenshot, build from source, OBS guide, roadmap, licence);
   `docs/SPEC.md` §0 D-001; `docs/lore/avloop.md`.

**Do not**

Decode video (AV-002), draw on the canvas (AV-003), build any tool's behaviour (AV-003 on). The menu's later entries
exist disabled; nothing that is not in the spec is added.

**Done when**

`scripts\build.ps1 -Fresh` builds Release with zero warnings; `ctest` passes; `check-loc`, `check-icons`,
`check-secrets`, `format.ps1 -Check` PASS; `avloop-cli version` shows no `--enable-gpl`; S1: `--shot` pictures of the
Navy and Black themes at 100/150/200% DPI, maximized, every icon visible, only Open / Preview / Export in the top bar;
second start under 2 s; pushed; CI green; README's screenshot is the real window.

**Decisions this phase must record**

D-001 (spec §0); readings R-1 (English, `tr()`), R-4 (MIT); External freshness of every tool; the build layout.

**Validation**

`scripts\build.ps1 -Fresh -Config Release`; `ctest --test-dir build --output-on-failure --timeout 300`;
`scripts\format.ps1 -Check`; `node tools/icons/check-icons.mjs`; `node tools/check-secrets.mjs`;
`node control/phase-tools/check-loc.mjs`; `node control/lab/walk-phase.mjs execution/AV-001-walk walk-shell.mjs`;
`gh run watch`.

**Seam if this splits.** AV-001a — the open-source files, CI and README v1 — after the build, the shell, the theme
and the icons.
