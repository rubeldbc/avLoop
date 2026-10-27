# 22 Shapes — the release 1.1.0 (item 19, AV-021, late)

Canonical current execution state: `../AUTONOMOUS_STATE.md`

The owner's words of 2026-10-27 (`../README.md` §1, item 19). D-019. Readings R-74 … R-76.

## 1. The version

- `CMakeLists.txt`'s `project(AvLoop VERSION 1.1.0 …)` is the one place: `AvLoopVersion.h` gives avloop.exe's
  VERSIONINFO (FILEVERSION 1,1,0,0; ProductVersion "1.1.0"), `AppInfo::version()` (the About box, `avloop-cli
  version`), and the installer reads avloop.exe's own (`AvLoop-1.1.0-setup.exe`, Apps & features 1.1.0).
- A minor step (R-74): main added features since 1.0.1 — the wheel and the keyframe copy, the clip effects, the colour
  theme — and the project file's version 2, which 1.0.x refuses with its message. Nothing a 1.0.x user has is taken
  away: settings, presets, the batch queue and version-1 projects open as before.
- The AppId, the install folder, the association and the settings keys are unchanged: 1.1.0 installs over 1.0.x.

## 2. The Release

`v1.1.0`, named "AvLoop by Rubel 1.1.0", on the phase's commit once its CI is green, marked latest; assets
`AvLoop-1.1.0-setup.exe` and `AvLoop-1.1.0-setup.exe.sha256` (`<hash>  <name>`, as 1.0.1's). The notes, as 1.0.1's:
the download and its SHA-256; *What changed since 1.0.1* — the canvas's wheel zoom about the pointer and Alt+drag
keyframe copy; the Clip effects section; the colour theme with the eyedropper; the project file's version 2 (a file
with effects does not open in 1.0.x) —; installed over 1.0.x it keeps the settings, presets, queue and association;
unsigned (SmartScreen), per user or per machine; the OBS tip; the licences. `v1.0.1` and `v1.0.0` stay as they are.
The asset is downloaded again and its SHA-256 compared with the file's.

## 3. Walk

`walk-release.mjs` (Node): the root `CMakeLists.txt` says 1.1.0; `build\bin\avloop-cli version` says "AvLoop 1.1.0";
`scripts\deploy.ps1 -NoBuild` deploys 1.1.0; `deploy\avloop.exe`'s details (name, publisher, file and product
version) and `deploy\avloop-cli.exe version`; ISCC makes `AvLoop-1.1.0-setup.exe`, its own details 1.1.0; its SHA-256
written beside it. The owner's own installed AvLoop (if any) — its Apps & features entry read before and after —
untouched (R-75). AV-017's `walk-about.mjs` again: the About box says "Version 1.1.0".
