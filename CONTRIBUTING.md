# Contributing to AvLoop

Thank you for helping. AvLoop is a Windows desktop app (C++20, Qt 6.8, FFmpeg 7.1) that turns a cut and cropped part
of any video into a seamless looping WebM for OBS. Issues and pull requests are welcome.

## Build from source

You need Windows 10/11 x64 and:

- **Visual Studio 2022** with *Desktop development with C++* (MSVC v143; its CMake, Ninja and clang-format are used).
- **CMake 3.28+** (Visual Studio's is enough) and **Node.js 20+** (the gate tools).
- **Qt 6.8** for `msvc2022_64`. The simplest way:

  ```powershell
  python -m pip install aqtinstall
  python -m aqt install-qt windows desktop 6.8.3 win64_msvc2022_64 -O C:\Qt
  $env:QTDIR = 'C:\Qt\6.8.3\msvc2022_64'
  ```

Then, from the repository root:

```powershell
scripts\fetch-deps.ps1          # FFmpeg 7.1 LGPL shared build into third_party\ffmpeg
scripts\build.ps1 -Config Release
scripts\test.ps1                # ctest -R avloop
build\bin\avloop.exe
```

`scripts\build.ps1 -Fresh` starts from an empty build folder; `-Config Debug` builds into `build-debug`;
`-Sanitize` builds with AddressSanitizer into `build-asan`.

## Rules every change follows

- **No code file over 600 lines**, anywhere in the repository (`.c .cpp .h .hpp .cmake CMakeLists.txt .qss .mjs .js
  .ps1 .py .iss .glsl .frag .vert .qrc .ui`); aim for new files under 450. Split a growing file at its seam first.
  `node phases/26-10-26-034934-AvLoop/control/phase-tools/check-loc.mjs` must print PASS.
- **Code style**: `.clang-format` (LLVM-based, 120 columns): `scripts\format.ps1` formats, `scripts\format.ps1 -Check`
  verifies. Warnings are errors (`/W4 /WX`).
- **Memory**: every cache and queue has a fixed limit; FFmpeg objects live in `std::unique_ptr` with a deleter
  (`src/media/FfmpegPtr.h`); no raw `new`/`delete` except objects handed to a Qt parent.
- **Modules**: `core/` uses Qt Core only; `ui/` never calls FFmpeg; the project model changes only through Undo
  commands; preview and export share one `FrameRenderer`.
- **Words**: every user-visible string goes through `tr()`; labels at most 40 characters; English.
- **Files**: project-file and settings keys are never renamed — a format change bumps `version` with a migration.
- **Icons**: the app draws its own (`resources/icons/*.svg`, 24×24, `currentColor`); `node tools/icons/check-icons.mjs`.
- **Nothing secret**: `node tools/check-secrets.mjs` must print PASS (no tokens, emails or personal paths).
- **No GPL**: never link libx264/libx265 or a GPL FFmpeg build.

## Commits and pull requests

- One coherent change a pull request; describe what it does and how you checked it (tests, a screenshot for UI).
- Commit messages: a short subject line in the imperative (`Fix the crop's snapping at 150% zoom`), a body that says
  why. The maintainers' own phase work uses `[AV-NNN] …` subjects; you do not need to.
- CI (`.github/workflows/ci.yml`) builds, runs the unit tests and the gates on `windows-latest`; it must be green.

## Reporting bugs

Use the *Bug report* issue form. Include the AvLoop version, Windows version, GPU, the video's details (`avloop-cli
probe <video>` prints them) and the log from `%LOCALAPPDATA%\AvLoop\logs`. Security problems: see `SECURITY.md`.
