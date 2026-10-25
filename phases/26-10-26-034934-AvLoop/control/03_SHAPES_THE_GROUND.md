# 03 Shapes — the ground (item 1, AV-001)

Canonical current execution state: `../AUTONOMOUS_STATE.md`

Spec §2.4, §2.6, §2.7, §2.9, §3.1, §3.2, §4, §5. D-001. The mock `docs/mock/avloop-gui-mock.html` is the look.

## 1. The build

- Root `CMakeLists.txt` (≤ 80 lines): `project(AvLoop VERSION 0.1.0 LANGUAGES CXX)`, C++20, options `AVLOOP_ASAN`,
  `AVLOOP_BUILD_TESTS` (ON); `include(cmake/Warnings.cmake)`, `Qt.cmake`, `FindFFmpeg.cmake`, `Deploy.cmake`;
  `add_subdirectory(src)`, `tests`; outputs in `build/bin`.
- `cmake/Warnings.cmake`: `/W4 /WX /permissive- /utf-8 /Zc:__cplusplus /EHsc`, `NOMINMAX`, `WIN32_LEAN_AND_MEAN`,
  `UNICODE`; third-party headers as SYSTEM includes so their warnings do not fail the build.
- `cmake/FindFFmpeg.cmake`: imported targets `FFmpeg::avformat`, `avcodec`, `avutil`, `swscale`, `avfilter` from
  `third_party/ffmpeg/{include,lib}`; the DLLs listed for copying.
- `cmake/Qt.cmake`: `find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets Svg Concurrent
  Test)`; `qt_standard_project_setup()`; AUTOMOC/AUTORCC.
- `cmake/Deploy.cmake`: a post-build step copying the FFmpeg DLLs into `build/bin`; `windeployqt` is run by
  `build.ps1` once a fresh build (and by `deploy.ps1` at AV-014).
- `scripts/fetch-deps.ps1`: downloads the pinned zip (R-9), checks SHA-256, extracts to `third_party/ffmpeg`
  (`bin`, `include`, `lib`), writes `third_party/ffmpeg/VERSION.txt`; on a 404 asks the GitHub API for the newest
  `n7.1` `win64-lgpl-shared-7.1` asset and records it.
- `scripts/build.ps1`: finds Visual Studio by `vswhere`, imports `vcvars64` into the process, finds Qt (`QTDIR`,
  `Qt6_DIR`, else `<drive>:\Qt\6.8.*\msvc2022_64`), configures Ninja (`-Fresh` wipes `build/`), builds, runs
  windeployqt where `Qt6Core.dll` is missing; `-Config Release|Debug|RelWithDebInfo`, `-Sanitize`.
- `scripts/test.ps1` (ctest with the five-minute timeout), `scripts/format.ps1` (`-Check` = dry run, fail on a
  difference; Visual Studio's clang-format).

## 2. The shell (§5.1)

- `MainWindow`: `showMaximized()`, minimum 1280×720; top bar 36 px (☰, the logo, *AvLoop*, Open | the options bar's
  slot | Preview, Export — Preview and Export disabled until a video is open); tool rail 40 px (buttons 32×32, icons
  20 px, tooltips "Name (Key)"); the canvas area (until AV-003 a placeholder with the drop zone: a large dashed
  rectangle, the open icon, "Drop a video here", Open); the side panel 260 px in a splitter (four `Section`s: Crop,
  Loop, Output, Quality — collapsible, state remembered; a preset row above them, disabled until AV-010); the timeline
  area 150 px (splitter 110–260; until AV-005 the transport row and an empty ruler); the status bar 22 px (source,
  GPU, export progress, RAM/VRAM/cache).
- `MainMenu.cpp`: New, Open Project, Save, Save As, Recent, Batch Queue, Loop Finder, Theme (Navy, Black), Settings,
  Keyboard shortcuts, About, Exit — each later phase's entry disabled until it is built.
- Window state: the normal geometry and splitter positions in `QSettings` (restored after un-maximize).
- Fonts: Segoe UI 9 pt; numbers and timecodes Consolas 9 pt. HiDPI: `PassThrough`.

## 3. The theme (§5.2) and the icons (§5.3)

- `Theme`: the token table (bg, panel, panel2, border, text, text2, accent, accentHover, accentPress, accentSoft,
  warning, danger, success) for Navy and Black; `resources/themes/*.qss` written with `@token` placeholders, replaced
  at load; no hex in the QSS (a test reads them); corners 2 px; inputs 24 px; switching without restart (re-polish +
  `IconFactory` cache cleared, `themeChanged` signal).
- `IconFactory`: loads `:/icons/<name>.svg`, replaces `currentColor` with the state colour (normal `text2`, hover
  `text`, active `accent`, disabled `text2` at 40%), renders by `QSvgRenderer` at the DPR, caches per (name, state,
  size, dpr, theme); a `QIcon` with Normal/Active/Selected/Disabled modes.
- Every icon of §5.3's table as an SVG (24×24 viewBox, stroke 1.75, `currentColor`, `fill="none"`, round caps and
  joins); `resources/app/avloop.svg` → `avloop.ico` (16 … 256) by a Node tool using the built app's renderer, or by
  `avloop-cli` (no third-party converter); `avloop.rc` sets it for the exe; every window sets `windowIcon`.

## 4. The lab's entry points

- `--shot <png> [--theme] [--tool] [--project]`, `--walk <json> --walk-out <dir>`, `--mem-log <file>` parsed in
  `app/main.cpp`; `WalkRunner` executes steps: `wait`, `key`, `click`, `drag`, `wheel`, `type`, `shot`, `expect`,
  `theme`, `open`, `resize`, `quit`; unknown steps fail the walk.

## 5. The repository's public face (§2.9)

`LICENSE` (MIT, R-4), `LICENSE-THIRD-PARTY.md` (Qt LGPLv3, FFmpeg LGPL 2.1+, libvpx BSD, dav1d BSD, zimg WTFPL — with
where their sources are), `CONTRIBUTING.md`, `CODE_OF_CONDUCT.md` (Contributor Covenant 2.1, reports by GitHub's
private channel), `SECURITY.md`, `.github/ISSUE_TEMPLATE/{bug,feature}.yml`, `PULL_REQUEST_TEMPLATE.md`,
`.github/workflows/ci.yml` (windows-latest: checkout, Node, Qt by `jurplel/install-qt-action`, `fetch-deps.ps1` with
the zip cached, `build.ps1`, `ctest` with `QT_QPA_PLATFORM=offscreen` and the GPU tests skipped, `check-loc`,
`check-secrets`, `check-icons`, `format.ps1 -Check`). `README.md`: logo, badges (CI, License, Platform: Windows), the
pitch, a real screenshot, features (only what works), Roadmap, the OBS guide, download/install, build from source,
shortcuts, structure, contributing, licence, third-party notices. `CLAUDE.md`: the agent's rules.
