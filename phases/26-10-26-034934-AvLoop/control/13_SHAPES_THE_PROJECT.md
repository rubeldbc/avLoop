# 13 Shapes — the project and the settings (item 10, AV-010)

Canonical current execution state: `../AUTONOMOUS_STATE.md`

Spec §11.1–§11.4, §12.3. D-010. Reading R-13 (the format first written by AV-003).

## 1. The file

- §11.1's JSON; all times seconds, geometry scene px; unknown keys kept in `extra` and written back.
- Finding the source: `path` → `relativePath` (relative to the project file) → *Locate source…* (a file dialog);
  size/mtime differ → a status warning.
- `version`: `ProjectMigrate.cpp` reads older versions; a newer file → "This project was saved by a newer AvLoop
  (version N); update AvLoop to open it."
- Save atomic by `QSaveFile`; the title `AvLoop — <name>.avloop*`; closing with changes: Save / Don't save / Cancel.
- Menu: New, Open Project, Save (Ctrl+S), Save As (Ctrl+Shift+S), Recent (the last 10 projects and videos);
  `avloop.exe <video|project>`; drag and drop of `.avloop`.

## 2. Autosave and recovery

Every 2 minutes (Settings: minutes) when there are changes, to `%LOCALAPPDATA%\AvLoop\autosave\<session>.avloop` with
a marker of the session's PID; a clean exit deletes it; at start, an autosave whose PID is not alive → "Recover unsaved
project?" (Recover / Discard).

## 3. Settings (`core/AppSettings`, `ui/dialogs/SettingsDialog`)

`QSettings` INI at `%APPDATA%\AvLoop\AvLoop.ini`. Tabs: General (Theme, open last project at start, autosave
minutes), Performance (Hardware decoding Auto/Off, GPU rendering Auto/Off, RAM cache MB 256–8192, disk cache folder
and GB, Clear cache), Export (default folder, name template `{source}_loop_{w}x{h}`, show OBS tips). Keys are frozen
once written (§2.7). `AboutDialog`: version, licences, the libraries of §3.8.

## 4. Presets (`core/Presets`)

Loop + Output + Quality under a name in `%APPDATA%\AvLoop\presets\*.json`; shipped: "OBS Overlay 1:1 512", "OBS Lower
Third 16:9 1280", "OBS Vertical 9:16 1080" (built in, not files to delete); the dropdown + save/delete icons above the
side panel; applying a preset is one Undo step.

## 5. Logs and crashes

`%LOCALAPPDATA%\AvLoop\logs\avloop-YYYYMMDD.log`: a Qt message handler writing `HH:MM:SS.mmm [module] text`; 7 days
kept; a file over 10 MB rolls to `-2`; on an unhandled exception `MiniDumpWriteDump` into the logs folder; the
session cache deleted at the next start.

## 6. The shortcut overlay

F1 shows a small overlay listing §11.4's table (from the actions' own shortcuts, so it is never stale); Esc or F1
closes it. Ctrl+, opens Settings; Ctrl+L Loop Finder (AV-012).

## 7. Tests and walks

`test-project-json` (round trip, `extra`, migration from a version 0 fixture, newer refused), `test-presets`.
Walks: `walk-project.mjs` (save → open → `avloop-cli frame` identical; the source moved → relativePath; moved again →
Locate needed, said by the log), `walk-settings.json` (the tabs, the theme switch, F1 — pictures), `walk-recover.mjs`
(start, change, wait for autosave, kill the process, start again → the recovery question, accepted, the project
back).
