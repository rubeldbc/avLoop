<!-- generated-by: control/phase-tools/sync-progress.mjs — do not edit by hand -->
# 00 — Phase Index

Canonical current execution state: `../AUTONOMOUS_STATE.md`

Phases: **9** · done: **8** · state: **ACTIVE** · generated: 2026-10-29T22:22:59.982Z

Tracks: A The editor — the sequence, the composite, the shell, the timeline, the effect controls, the masks, the export, the project

"Item" is the owner's numbered item the phase implements (`../README.md` §1).

| Phase | Item | Track | CP | Status | Risk | Class | Title | Depends on |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| AS-001 | 1 | A | CP-A1 | DONE | High | verify-env | The sequence | — |
| AS-002 | 2 | A | CP-A1 | DONE | High | verify-env | The composite | AS-001 |
| AS-003 | 3 | A | CP-A1 | DONE | High | verify-env | The editor's shell | AS-002 |
| AS-004 | 4 | A | CP-A1 | DONE | High | verify-env | The timeline editor | AS-003 |
| AS-005 | 5 | A | CP-A1 | DONE | Medium | verify-env | The Effect Controls | AS-004 |
| AS-006 | 6 | A | CP-A1 | DONE | High | verify-env | The masks on the monitor | AS-005 |
| AS-007 | 7 | A | CP-A1 | DONE | Medium | verify-env | The export | AS-006 |
| AS-008 | 8 | A | CP-A1 | DONE | Medium | verify-env | The project | AS-007 |
| AS-009 | 0 | A | CP-A1 | IN_PROGRESS | High | verify-env | **CP-S1 the studio walked** | AS-001, AS-002, AS-003, AS-004, AS-005, AS-006, AS-007, AS-008 |
