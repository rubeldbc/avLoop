---
phase: AV-017
title: "The name and the About box"
status: DONE
owner_item: 15
track: B
checkpoint_id: CP-B1
is_checkpoint: false
late: true
risk: Low
permission_class: VERIFY_ENVIRONMENT_FIRST
code_budget: 200
depends_on: [AV-016]
unlocks: [AV-018]
requirements: [REQ-AV-022]
read_scope:
  - phases/26-10-26-034934-AvLoop/control/18_SHAPES_THE_NAME_AND_ABOUT.md
  - docs/SPEC.md
  - src/ui/dialogs/AboutDialog.cpp
  - installer/AvLoop.iss
  - spec: §3.8, §5.1
---

# The name and the About box

Canonical current execution state: `../AUTONOMOUS_STATE.md`

**Goal.** The application is shown as "AvLoop by Rubel" — in the app, its executable's details and its installer —
and its About box gives the developer and the ways to reach them (`../README.md` §1 item 15; D-015). A late phase:
the owner's wish after CP-B1 closed.

**Ground truth**

- The owner's words (`../README.md` §1, 2026-10-26, after the programme closed).
- The owner's gLayer About box (`gLayer2/apps/web/src/editor/about/about-content.ts`, the owner's own repository):
  the developer's name, country, profile and WhatsApp. The email: the owner's correction.
- Spec §5.1 (the ☰ menu's About), §3.8 (the libraries named in About); spec §2.7 (settings keys never renamed).

**Do**

1. `AppInfo::displayName()`; the title, the top bar, the menu, the questions' titles and the About box use it;
   `name()` and `organization()` stay "AvLoop" (R-59).
2. The About box of `control/18` §2, its links in the theme's accent; the `about.text` probe.
3. avloop.exe's VERSIONINFO and `installer/AvLoop.iss` by `control/18` §1; version 1.0.1 and its Release (R-62).
4. `tools/check-secrets.mjs`: the one address in the one file (R-61).
5. The walks before that read the old name (`AV-001-walk/walk-shell.json`, `AV-010-walk/walk-settings.json`) read the
   new one.

**Do not**

Rename a settings key, a data folder, the install folder, the AppId or the association; let the secrets gate pass
any other address.

**Done when**

Build 0 warnings; ctest; format, icons, secrets, LOC PASS; `walk-about.mjs` and `walk-name.mjs` PASSED, the earlier
walks of the old name PASSED again; README's pictures retaken; pushed with CI green; the Release `v1.0.1`.

**Decisions this phase must record**

D-015 (spec §0); R-59 … R-62.

**Validation**

`node control/lab/walk-phase.mjs execution/AV-017-walk walk-about.mjs walk-name.mjs`; `walk-phase.mjs` of
`AV-001-walk walk-shell.json`, `AV-010-walk walk-settings.json walk-project.mjs` and `AV-014-walk
walk-install.mjs`; check-loc; check-secrets; `validate-phase-system.mjs`.

**Seam if this splits.** AV-017a — the installer's names — after the app's name and the About box.
