# 18 Shapes — the name and the About box (item 15, AV-017, late)

Canonical current execution state: `../AUTONOMOUS_STATE.md`

The owner's words of 2026-10-26, after the programme closed (`../README.md` §1, item 15). D-015. Readings R-59 … R-62.

## 1. The shown name

- `AppInfo::displayName()` = "AvLoop by Rubel": the window's title (alone, or "AvLoop by Rubel — <file>"), the top
  bar's title, the ☰ menu's "About AvLoop by Rubel", the About box's title and heading, the questions' window titles.
- avloop.exe's VERSIONINFO: ProductName "AvLoop by Rubel", FileDescription "AvLoop by Rubel - seamless looping videos
  for OBS", CompanyName and LegalCopyright the developer's name.
- The installer (`installer/AvLoop.iss`): AppName, AppVerName, the Start menu and desktop shortcuts, Apps & features
  (UninstallDisplayName), "Launch AvLoop by Rubel"; AppPublisher and AppCopyright the developer. An install over 1.0.0
  deletes 1.0.0's `AvLoop.lnk` shortcuts (`[InstallDelete]`).
- What stays "AvLoop" (R-59): `AppInfo::name()` and `organization()` — QSettings, `%APPDATA%\AvLoop`,
  `%LOCALAPPDATA%\AvLoop`, the disk cache's folder —; the install folder; the AppId; `.avloop` → `AvLoop.Project`; the
  executables' file names; `avloop-cli version`'s line; the project file's keys.

## 2. The About box (`src/ui/dialogs/AboutDialog.cpp`)

- The logo; "AvLoop by Rubel", "Version <x.y.z>", the one-line purpose, "MIT licence · github.com/rubeldbc/avLoop"
  (a link).
- **Developer & contact**: Developer — Md. Kamrul Islam Rubel; Country — Bangladesh; Email — the owner's address
  (`mailto:`); Developer profile — fb.com/rubel.social (https); WhatsApp — the owner's number (`https://wa.me/` and
  its digits). No website (the owner: "website not required"). The details are those of the owner's gLayer About box
  (R-60), the email the owner's own correction; the address and the number are written in `AboutDialog.cpp` alone.
- **Built with**: the libraries and their licences, as before. The footer: "© <year> Md. Kamrul Islam Rubel · Designed
  and built in Bangladesh". Close.
- Links open in the browser or the mail program (`setOpenExternalLinks`); their colour is the theme's accent
  (`QPalette::Link`). Proper nouns, addresses and numbers are not translated; every sentence is `tr()`.
- The walk reads it by the `about.text` probe: the box's labels as written, links with their targets.

## 3. The version and the gate

- Version 1.0.1 (R-62): a build that differs from the Release 1.0.0 is not called 1.0.0. The GitHub Release `v1.0.1`
  on the phase's commit carries `AvLoop-1.0.1-setup.exe` and its SHA-256 (as R-5 for 1.0.0); the README links it.
- `tools/check-secrets.mjs` allows the owner's address in `src/ui/dialogs/AboutDialog.cpp` only (R-61); any other
  address, or this one in any other file, still fails.

## 4. Walk

`walk-about.mjs` (Node, the app walked by a script it makes): the title, the menu entry, the About box's labels and
links by `about.text` — the contact details read from `AboutDialog.cpp`, the log naming each by its row —, its
picture in Navy and Black. `walk-name.mjs` (Node): `deploy.ps1` and ISCC make `AvLoop-1.0.1-setup.exe`; its own and
avloop.exe's VERSIONINFO read; a stand-in 1.0.0 `AvLoop.lnk` in the Start menu; a silent per-user install into a temp
folder; the shortcut "AvLoop by Rubel.lnk" there and the stand-in gone; Apps & features' DisplayName, Publisher and
DisplayVersion; the silent uninstall leaves no shortcut and no entry. The settings folders untouched.
