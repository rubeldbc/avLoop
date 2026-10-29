# 02 The checkpoint (CP-S1, AS-009)

Canonical current execution state: `../AUTONOMOUS_STATE.md`

Gates at every close: build 0 warnings (`/W4 /WX`); `ctest -R avloop`; `scripts\format.ps1 -Check`; `check-icons`;
`check-secrets`; `check-loc` (600 lines a code file, 300 a programme document); the phase's own walks; CI green.

CP-S1 walks: every `execution/AS-*-walk` walk; AvLoop's walks that still mean something on migrated projects (its
render, loop, export, effects, shapes, layers walks — the GUI walks of panels that moved are replaced by this
programme's); the memory walk (growth < 50 MB); ASan (no report) and Debug (`avloop-leaks`).
