# 17 Shapes — the installer (item 14, AV-014)

Canonical current execution state: `../AUTONOMOUS_STATE.md`

Spec §1.2 item 14, §1.3, §3.1, §3.8. D-014. No code signing.

## 1. The deployment

`scripts/deploy.ps1`: a Release build → `deploy/` with `avloop.exe`, `avloop-cli.exe`, `windeployqt --release
--no-translations --no-system-d3d-compiler --no-opengl-sw` (Qt's DLLs, the platform and style plugins, `imageformats`
with `qsvg`, `iconengines`), the FFmpeg DLLs, the licences (`LICENSE`, `LICENSE-THIRD-PARTY.md`, Qt's LGPL text,
FFmpeg's LGPL text), the VC++ runtime DLLs (app-local from Visual Studio's redist folder).

## 2. The installer (`installer/AvLoop.iss`)

- Inno Setup 6; `AppId` fixed; the version from `CMakeLists.txt` (passed by `/DAppVersion=`); `PrivilegesRequired=
  lowest` with `PrivilegesRequiredOverridesAllowed=dialog` (per-user default, per-machine by choice); the licence page
  (MIT + notices); Start menu entry; an optional desktop icon; the app icon.
- The `.avloop` association: `HKA\Software\Classes\.avloop` → `AvLoop.Project`; its `DefaultIcon` the exe;
  `shell\open\command` = `"{app}\avloop.exe" "%1"`; `ChangesAssociations=yes`.
- Uninstall: the program files, the association, `%LOCALAPPDATA%\AvLoop\cache` and `\autosave`; `%APPDATA%\AvLoop`
  (settings, presets, queue) kept.
- Output: `installer/Output/AvLoop-<version>-setup.exe` (git-ignored).

## 3. Walk

`walk-install.mjs`: `ISCC` builds it; `/VERYSILENT /SUPPRESSMSGBOXES /CURRENTUSER /DIR=<temp>` installs; `avloop.exe
--shot` from there (a picture); the association's command read from the registry (`reg query`) and run with a sample
`.avloop` and `--shot` (the project open in the picture's title); `unins000.exe /VERYSILENT` uninstalls; the folder,
the association and the cache gone; `%APPDATA%\AvLoop` still there.
