## What this changes

<!-- One coherent change: what it does and why. Link the issue it closes. -->

## How it was checked

<!-- Tests run, a screenshot or a short clip for anything visible, the video you tried. -->

## Checklist

- [ ] `scripts\build.ps1` builds with zero warnings and `scripts\test.ps1` passes
- [ ] `scripts\format.ps1 -Check` is clean
- [ ] `node phases/26-10-26-034934-AvLoop/control/phase-tools/check-loc.mjs` prints PASS (no code file over 600 lines)
- [ ] `node tools/check-secrets.mjs` prints PASS
- [ ] New user-visible words go through `tr()`; no project or settings key renamed
