# AvLoop — rules for the implementation agent

The programme: `phases/26-10-26-034934-AvLoop` (start at its `AGENT_BOOTSTRAP.md`, then `AUTONOMOUS_STATE.md`,
`EXECUTOR-PREAMBLE.md`, `CONTINUE.md`). The specification: `docs/SPEC.md` (§0 takes D-001 … D-014, newest first).
The traps found so far: `docs/lore/avloop.md`.

## Hard rules

- **No code file over 600 lines, anywhere** (`.c .cpp .h .hpp .cmake CMakeLists.txt .qss .mjs .js .ps1 .py .iss .glsl
  .frag .vert .qrc .ui`). New files aim under 450; programme documents and tools under `phases/` under 300.
  `node phases/26-10-26-034934-AvLoop/control/phase-tools/check-loc.mjs` PASS before every commit.
- **Nothing secret or personal committed**: `node tools/check-secrets.mjs` PASS before every commit; stage explicit
  paths; the GitHub token reaches `gh` only as `GH_TOKEN` for one command (from `git credential fill`), never stored or
  printed.
- **Autonomous mode**: never ask the owner; decide, record the reading (`README.md` §3 of the programme,
  `DECISIONS.md`), continue. A failure is fixed before the next phase. Gates are never weakened.
- Every command under a five-minute ceiling; one test suite at a time.

## The close of a phase (EXECUTOR-PREAMBLE §4)

1. Focused tests; `scripts\build.ps1` (0 warnings); `ctest --test-dir build --output-on-failure --timeout 300 -R
   avloop`; `scripts\format.ps1 -Check`; `node tools/icons/check-icons.mjs`; `node tools/check-secrets.mjs`.
2. The phase's own walk: `node control/lab/walk-phase.mjs execution/AV-NNN-walk <walks …>` (from the programme
   folder) — the pictures looked at and described; nothing left behind.
3. README current (the feature list true; `docs/screenshots` retaken by `build\bin\avloop.exe --walk
   tools/walks/readme-shots.json --walk-out docs/screenshots` when the UI changed; each PNG under 300 KB).
4. `execution/AV-NNN-EXECUTION.md` from the template; `docs/SPEC.md` §0 under the D-number; the lore.
5. `node control/phase-tools/close-phase.mjs AV-NNN "<one line>"`.
6. `git commit -F <message file>` with explicit paths, subject `[AV-NNN] …`; `git fetch origin`; `git push origin
   main` (never force; retry 30 s, 2 min, 10 min); then watch CI with `gh run watch` and fix red before going on.

## Code

- C++20, MSVC v143, `/W4 /WX /permissive- /utf-8`; `.clang-format` (LLVM, 120 columns).
- `core/` never includes Widgets or FFmpeg; `ui/` never calls FFmpeg; the model changes only through Undo commands;
  preview and export share one `FrameRenderer` (AVADR-0001); every cache and queue bounded (AVADR-0002).
- FFmpeg objects by RAII (`src/media/FfmpegPtr.h`); no raw `new`/`delete` but Qt parents and Qt's ownership-taking
  APIs (`QIcon(QIconEngine*)`).
- Every user-visible word through `tr()`; labels at most 40 characters; colours only from the theme's tokens.
- Project-file and settings keys are never renamed; a format change bumps `version` with a migration.

## This PC

Qt 6.8.3 (`QTDIR`, else found under `<drive>:\Qt`); Visual Studio 2022's MSVC, Ninja and clang-format;
`scripts\build.ps1` imports the MSVC environment itself. PowerShell 5.1: a GUI exe launched without a pipe is not
waited for — use Node (`spawnSync`) or pipe its output. A Bash heredoc eats backslashes: write scripts with the Write
tool.
