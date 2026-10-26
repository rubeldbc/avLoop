---
phase: AV-010
title: "The project and the settings"
status: IN_PROGRESS
owner_item: 10
track: A
checkpoint_id: CP-A1
is_checkpoint: false
risk: Medium
permission_class: VERIFY_ENVIRONMENT_FIRST
code_budget: 550
depends_on: [AV-009]
unlocks: [AV-015]
requirements: [REQ-AV-010, REQ-AV-015, REQ-AV-020]
read_scope:
  - phases/26-10-26-034934-AvLoop/control/13_SHAPES_THE_PROJECT.md
  - docs/SPEC.md
  - spec: §11.1, §11.2, §11.3, §11.4
  - after AV-003: src/core/ProjectJson.h
  - after AV-003: src/core/Commands.h
---

# The project and the settings

Canonical current execution state: `../AUTONOMOUS_STATE.md`

**Goal.** Work is kept and the app remembers: the `.avloop` file's whole UX (save, open, Save As, atomic, title `*`,
the close question, source finding, migration), autosave and crash recovery, Recent, the Settings window, presets,
logs and minidumps, the shortcut overlay (`../README.md` §2 item 10; D-010).

**Ground truth**

- Spec §11.1–11.4; `ProjectJson` version 1 (AV-003) and the Undo commands; §12.3's crash and instance rows.

**Do**

1. `core/ProjectMigrate.cpp` (older versions read; a newer file refused clearly), the source's finding (`path` →
   `relativePath` → *Locate source…*; size/mtime differ → status warning); `QSaveFile`; the title
   `AvLoop — <name>.avloop*`; Save / Don't save / Cancel; New, Open Project, Save, Save As, Recent (10 projects and
   videos) in the menu; `avloop.exe <video|project>`.
2. Autosave every 2 minutes with changes to `%LOCALAPPDATA%\AvLoop\autosave\`; "Recover unsaved project?" after a crash.
3. `core/AppSettings` (`%APPDATA%\AvLoop\AvLoop.ini`), `ui/dialogs/SettingsDialog` (General, Performance, Export
   tabs), `AboutDialog.cpp`; the theme switch without restart.
4. `core/Presets` (Loop + Output + Quality under a name, `%APPDATA%\AvLoop\presets\*.json`; the three shipped OBS
   presets; the dropdown with save/delete icons above the side panel).
5. Logs (`%LOCALAPPDATA%\AvLoop\logs\avloop-YYYYMMDD.log`, 7 days, 10 MB, time + module) and `MiniDumpWriteDump`;
   the F1 overlay of shortcuts; Ctrl+, Settings.
6. `tests/test-presets.cpp`; `test-project-json.cpp` grown (migration, `extra` kept, save → open identical).

**Do not**

Keyframes in the file beyond what the model holds (AV-011 adds them); the batch queue file (AV-013).

**Done when**

Build, ctest, gates PASS; S12 — save → open gives identical pixels (`avloop-cli frame` both, difference 0); a moved
source triggers relativePath, then Locate; Undo/Redo for every command; autosave recovery after a killed process;
the Settings tabs and the F1 overlay pictured.

**Decisions this phase must record**

D-010 (spec §0); key names frozen from here on (§2.7).

**Validation**

`scripts\build.ps1`; `ctest … -R "project|presets|commands"`; `node control/lab/walk-phase.mjs execution/AV-010-walk
walk-project.mjs walk-settings.json walk-recover.mjs`; format; check-loc; check-secrets.

**Seam if this splits.** AV-010a — presets, logs and minidumps — after the file, autosave, Recent and Settings.
