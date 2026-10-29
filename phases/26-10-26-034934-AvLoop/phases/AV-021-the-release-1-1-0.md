---
phase: AV-021
title: "The release 1.1.0"
status: DONE
owner_item: 19
track: B
checkpoint_id: CP-B1
is_checkpoint: false
late: true
risk: Low
permission_class: VERIFY_ENVIRONMENT_FIRST
code_budget: 150
depends_on: [AV-020]
unlocks: [AV-022]
requirements: [REQ-AV-026]
read_scope:
  - phases/26-10-26-034934-AvLoop/control/22_SHAPES_THE_RELEASE_1_1_0.md
  - docs/SPEC.md
  - CMakeLists.txt
  - installer/AvLoop.iss
  - README.md
  - spec: §1.2 item 14, §3.8
---

# The release 1.1.0

Canonical current execution state: `../AUTONOMOUS_STATE.md`

**Goal.** Version 1.1.0 and its GitHub Release `v1.1.0`: what main holds since `v1.0.1` — the wheel and the keyframe
copy, the clip effects, the colour theme — given as an installer (`../README.md` §1 item 19; D-019). A late phase.

**Ground truth**

- The owner's words (`../README.md` §1, 2026-10-27).
- R-5 and R-62: the Releases `v1.0.0` and `v1.0.1` — the installer and its SHA-256, the README's Download link.
- R-65, R-70, R-73: main ran ahead of `v1.0.1` with no Release until one carries it.

**Do**

1. `CMakeLists.txt` 1.1.0 — the one place (avloop.exe's VERSIONINFO, `AppInfo::version()`, the installer's name).
2. AV-017's About and name walks read the version from `CMakeLists.txt`, not a written `1.0.1` (R-76).
3. `walk-release.mjs`: the build, the deployment, the installer and its SHA-256 at 1.1.0; no silent install (R-75).
4. The README: v1.1.0, its Download link and file name; the Release `v1.1.0` on the phase's commit, after CI is green,
   with `control/22` §2's notes (R-74).

**Do not**

Change the installer's AppId, folder or association; install over the owner's own AvLoop; overwrite or delete the
Release `v1.0.1`; bump the project file's version.

**Done when**

Build 0 warnings; ctest; format, icons, secrets, LOC PASS; `walk-release.mjs` PASSED, AV-017's `walk-about.mjs`
PASSED again; README current; pushed with CI green; the Release `v1.1.0` with `AvLoop-1.1.0-setup.exe` and its
SHA-256, its asset's hash read back.

**Decisions this phase must record**

D-019 (spec §0); R-74 … R-76.

**Validation**

`node control/lab/walk-phase.mjs execution/AV-021-walk walk-release.mjs`; `walk-phase.mjs` of `AV-017-walk
walk-about.mjs`; check-loc; check-secrets; `validate-phase-system.mjs`.

**Seam if this splits.** AV-021a — the Release — after the version, its walk and the push.
