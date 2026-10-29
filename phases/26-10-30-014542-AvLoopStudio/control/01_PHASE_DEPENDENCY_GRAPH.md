# 01 Phase dependency graph

Canonical current execution state: `../AUTONOMOUS_STATE.md`

```
AS-001 ──► AS-002 ──► AS-003 ──► AS-004 ──► AS-005 ──► AS-006 ──► AS-007 ──► AS-008 ──► AS-009 (CP-S1)
```

| Phase | Depends on | Why |
| --- | --- | --- |
| AS-001 | — | the model first: every later phase edits or draws it |
| AS-002 | AS-001 | the composite draws the sequence |
| AS-003 | AS-002 | the monitor shows the composite; the bin feeds the sequence |
| AS-004 | AS-003 | the timeline lives in the shell |
| AS-005 | AS-004 | the Effect Controls act on the clip the timeline selects |
| AS-006 | AS-005 | the masks are rows of the Effect Controls and shapes on the monitor |
| AS-007 | AS-006 | the export writes what the editor makes |
| AS-008 | AS-007 | the project keeps all of it |
| AS-009 | AS-001 … AS-008 | CP-S1 walks the whole |
