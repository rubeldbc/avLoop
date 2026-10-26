---
phase: AV-014
title: "The installer"
status: DONE
owner_item: 14
track: B
checkpoint_id: CP-B1
is_checkpoint: false
risk: Medium
permission_class: VERIFY_ENVIRONMENT_FIRST
code_budget: 300
depends_on: [AV-013]
unlocks: [AV-016]
requirements: [REQ-AV-014]
read_scope:
  - phases/26-10-26-034934-AvLoop/control/17_SHAPES_THE_INSTALLER.md
  - docs/SPEC.md
  - spec: §1.2, §3.1, §3.8
  - after AV-001: cmake/Deploy.cmake
---

# The installer

Canonical current execution state: `../AUTONOMOUS_STATE.md`

**Goal.** A Windows installer made by Inno Setup 6 from a `windeployqt` deployment: Start menu entry, the `.avloop`
file association, an uninstaller that leaves nothing behind but settings, the LGPL and third-party notices; no code
signing (`../README.md` §2 item 14; D-014).

**Ground truth**

- Spec §1.2 item 14, §3.1 (Inno Setup 6 + `windeployqt`), §3.8 (the licences carried), §1.3 (no signing).

**Do**

1. `scripts/deploy.ps1` (Release build → `windeployqt` → FFmpeg DLLs → `deploy/`), `cmake/Deploy.cmake`'s target.
2. `installer/AvLoop.iss` (per-user or per-machine by the user's choice; Start menu; `.avloop` → `avloop.exe "%1"`
   with the app icon; the licences shown and installed; uninstall removes the program files, the association and
   `%LOCALAPPDATA%\AvLoop\cache`, keeps `%APPDATA%\AvLoop`).
3. The version from the root `CMakeLists.txt` into the installer and `AboutDialog`.

**Do not**

Sign anything; add an updater, telemetry or a licence key (§1.3).

**Done when**

Build, gates PASS; S15 — `/VERYSILENT` install into a temp folder, the app launches (`--shot`), a `.avloop`
double-click opens it (the association's command read from the registry and run), uninstall leaves nothing but the
settings.

**Decisions this phase must record**

D-014 (spec §0); install scope.

**Validation**

`scripts\deploy.ps1`; `ISCC installer\AvLoop.iss`; `node control/lab/walk-phase.mjs execution/AV-014-walk
walk-install.mjs`; check-loc; check-secrets.

**Seam if this splits.** AV-014a — the file association — after the deployment and the installer's install and
uninstall.
