# AV-010 — Execution Record

**Phase:** AV-010 — The project and the settings (owner item 10)
**Status:** DONE
**Date:** 2026-10-26
**Phase start commit:** `d990920` (*[AV-009] The export: VP9/VP8 alpha WebM, checked, with Target size*)
**ADR:** none owed
**Environment class:** local-disposable (this PC: `build/`, `samples/`, the walks' temp folders — the lab's settings,
autosave and logs in their own folder; lab windows behind the owner's, never activated)

## The owner's words

*"Work is kept and the app remembers."*

## What was built

1. The project file's UX (`ui/MainWindowProject.cpp`): New, Open Project, Save, Save As, Recent (10 projects, 10
   videos), Ctrl+O / drag and drop / `avloop.exe` for projects too; the title with `*`; Save / Don't save / Cancel
   before the work is replaced or the window closes; *Locate source…*; the changed-source warning.
2. `core/ProjectMigrate` (the version chain; a newer file refused), `core/ProjectSections.cpp` (Loop, Output and
   Quality to and from JSON — the file's and a preset's).
3. Autosave every N minutes to the data folder with its PID and project; deleted on a clean exit; offered back at the
   next start when its process is gone (Recover / Discard).
4. `core/AppSettings` (the frozen keys, their defaults and ranges, the folders, Recent); `ui/dialogs/SettingsDialog`
   (General, Performance, Export — saved at once), `AboutDialog`, `ShortcutOverlay` (F1).
5. `core/Presets` (three built-in OBS presets, the user's as JSON files), the preset row (`ui/panels/PresetPanel.cpp`),
   `Project::changeSettings` (a preset in one Undo step).
6. `app/Diagnostics` — the daily log (`HH:MM:SS.mmm [module] text`, 10 MB parts, 7 days) and the minidump on a crash.
7. The export reads the frozen keys (`export/showObsTips`, `export/nameTemplate`, `export/defaultFolder`).
8. The lab: `saveAs`, `autosaveNow`, `recover`, `crash` steps; `answer` takes a choice or a text; the window picture
   keeps a toast and the F1 list over the GL canvas.
9. Tests: `test-presets` (4), `test-project-json` (+ migration, save → open → save identical), `test-commands`
   (+ every command undone and redone, a preset one step).

## Decisions recorded

- **D-010** — the project and the settings as delivered, with the settings keys frozen (§2.7); spec §0.
- **R-39** the first format; **R-40** the presets' long sides; **R-41** the title of a video without a project;
  **R-42** the lab's answers and data folder; **R-43** a changed source's toast; **R-44** Settings saved at once.

## Specification delta

- §0: `D-010`.
- Lore: *The project and the settings* — four rules (a GL grab and floating widgets; one owner per probe; a quiet crash;
  a walk answers what a user would be asked).

## Verification

| Command | Result |
| --- | --- |
| `scripts\build.ps1` | built, **0 warnings** (from clean objects after the header changes) |
| `ctest -LE gpu` | **18/18 PASS** (new: avloop-presets) |
| `ctest -L gpu` | **1/1 PASS** |
| `scripts\format.ps1 -Check` | PASS: 197 files |
| `node tools/icons/check-icons.mjs` | PASS: 43 icons |
| `node tools/check-secrets.mjs` | PASS |
| `node control/phase-tools/check-loc.mjs` | PASS (the largest `src/ui/MainWindowProject.cpp`, 388 lines) |
| AV-009's CI run | green, 7 m 15 s |

## The walk

```
node control/lab/walk-phase.mjs execution/AV-010-walk walk-project.mjs walk-settings.json walk-recover.mjs
walk-project.mjs (S12, 16 s), in a temporary folder:
  save-open: a crossfade loop of a copied clip, an ellipse — "AvLoop — clip.mp4*"; Save As p1.avloop → "AvLoop —
    p1.avloop", clean; Feather 14 → "…p1.avloop*"; Ctrl+S clean; Ctrl+Z dirty; Ctrl+Y clean; Recent has p1; a change,
    close → "Save the changes to p1.avloop?", Cancel stays; New (Don't save) → "AvLoop", no video; p1 opened: Feather
    14, ellipse, clean; Save As p2 — 39 steps
  S12: avloop-cli frame 10 of p1 and of p2: 921,600 bytes each, max difference 0; p2 has every key of p1 (view aside)
  moved: the folder renamed → p1 opens its clip by relativePath (b\src\clip.mp4), Feather 14
  locate: the clip moved away → not found ("was not found"), the log "[project] the video was not found by its path
    or beside the project: …\a\src\clip.mp4 — Locate source…"; answered with the new place → opened, Feather 14
walk-settings.json (9 s): Ctrl+, → General (picture), Theme Black (picture) and back without a restart, autosave 5 min
  applied; Performance: RAM cache 1024 MB applied (picture); Export: the name "counter-640p30_loop_640x360.webm" →
  template "{source}-{w}" → "counter-640p30-640.webm" (picture); Close; F1 → the list (picture), Esc closes it; About
  (picture); the preset "OBS Lower Third 16:9 1280" applied → Long side 1280, Ctrl+Z → Crop size, Ctrl+Y → back;
  High → no preset matches; saved as "My overlay" (picture), deleted after "Delete the preset “My overlay”?"
walk-recover.mjs (12 s), the app's data in a temporary folder:
  a change autosaved, then a crash: exit 0xC0000005, avloop-YYYYMMDD-HHMMSS.dmp 603,844 bytes, the log "HH:MM:SS.mmm
  [app] AvLoop 0.1.0 started / HH:MM:SS.mmm [lab] error: a crash on purpose", the autosave left
  next start: "Recover unsaved project? (autosaved at …)" — Discard: nothing restored, the autosave gone
  a change autosaved, then taskkill /F: the autosave left; next start: Recover — ellipse, Feather 25, "AvLoop —
  counter-640p30.mp4*"; a clean exit leaves no autosave; no AvLoop left running
PHASE WALKS PASSED
```

## The pictures

| Picture | What | What it shows — and what was seen in it |
| --- | --- | --- |
| `AV-010-walk/settings-general.png` | General | Theme Navy, "Open the last project at start", "Autosave every 2 min"; Close |
| `AV-010-walk/settings-black.png` | the Black theme | the whole window re-themed at once (black panels, the orange accent) |
| `AV-010-walk/settings-performance.png` (and `docs/screenshots/settings.png`) | Performance | Hardware decoding and GPU rendering Auto with the note when they apply; RAM cache 1024 MB; the disk cache folder (default) and 20 GB with their note; Clear cache |
| `AV-010-walk/settings-export.png` | Export | the default folder (the last used), the template "{source}-{w}" with its note, the OBS tips on |
| `AV-010-walk/shortcuts.png` | F1 | the list over the canvas in two columns: the menu's and window's actions (Open, Save, Save As, Undo, Redo, Settings, F1, Preview, Export), the tools, the transport's keys |
| `AV-010-walk/about.png` | About | the logo, AvLoop 0.1.0, MIT, Qt 6.8.3 and the FFmpeg build with their licences, libvpx, dav1d, zimg, the Windows libraries |
| `AV-010-walk/presets.png` | the preset row | "My overlay" chosen, ✓ and ✕ |

## The regressions

| Walk of before | Result | Mended |
| --- | --- | --- |
| AV-001 … AV-009, every walk | PASSED | none |

Their pictures and logs were restored from git.

## External freshness

Not applicable — no dependency changed (DbgHelp is part of Windows).

## Files and line counts

New: `src/core/{AppSettings,Presets,ProjectMigrate}.{h,cpp}`, `src/core/ProjectSections.cpp`,
`src/app/Diagnostics.{h,cpp}`, `src/ui/MainWindowProject.cpp` (388), `src/ui/dialogs/{SettingsDialog,AboutDialog,
ShortcutOverlay}.{h,cpp}`, `src/ui/panels/PresetPanel.cpp`, `tests/test-presets.cpp`. Changed: `ProjectJson.*`,
`Project.*`, `MainWindow.*`, `MainWindowActions.cpp`, `MainWindowExport.cpp`, `MainWindowPreview.cpp`, `MainMenu.cpp`,
`SidePanel.*`, `FormatProfiles.*`, `FrameRenderer.cpp`, `MediaProbe.*`, `WalkSteps.cpp`, `app/main.cpp`, `app.qss`,
the tests and CMake files.

## Recovery log

Attempt 1 RESOLVED — gate: the build — `QAbstractButton` incomplete; the question's buttons kept as `QPushButton`.
Attempt 2 RESOLVED — gate: test-commands — `ProjectData` needs `ProjectJson.h`.
Attempt 3 RESOLVED — gate: walk-settings (the title) — the window's older `window.title` probe replaced the project's;
removed.
Attempt 4 RESOLVED — gate: the pictures (F1's list missing) — the window picture renders floating widgets after the GL
frames.
Attempt 5 RESOLVED — gate: the pictures (Loop Finder in the F1 list) — actions not built yet are left out.

## Deviations from the phase contract

- **Code budget**: 550 planned; ~1,600 new lines in `src` and `tests` + ~400 changed. The named seam (AV-010a: presets,
  logs and minidumps) would move ~450. Recorded instead of split.
- **"Open the last project at start"** is honoured outside lab runs only (a lab run's settings are fresh each time).

## Blocked legs (if any)

None.

## Staged paths

- `src/`, `tests/`, `resources/themes/app.qss`, `README.md`, `docs/SPEC.md`, `docs/lore/avloop.md`,
  `docs/screenshots/settings.png`, `phases/26-10-26-034934-AvLoop/`.

## Next

AV-015 — CP-A1, the checkpoint (it unlocks after AV-010), promoted to IN_PROGRESS in the same turn. No avloop process,
session cache or walk temp file left; the owner's focus untouched.
