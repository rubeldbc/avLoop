# AV-015 — Execution Record

**Phase:** AV-015 — CP-A1, the loop maker walked (checkpoint after track A, items 1–10)
**Status:** DONE
**Date:** 2026-10-26
**Phase start commit:** `6b3424e` (*[AV-010] The project and the settings*)
**ADR:** none owed
**Environment class:** local-disposable (this PC: `build/` fresh, `build-asan/`, `build-debug/`, `samples/`, the walks'
temp folders; lab windows behind the owner's, never activated)

## What was done

1. One fresh Release build (`scripts\build.ps1 -Fresh`), the whole ctest on it, an AddressSanitizer build and its
   ctest, a Debug build and its ctest with the new leak gate (`tests/test-leaks.cpp`).
2. Every phase's walks (AV-001 … AV-010) on that build; the checkpoint's own: the §3.7.8 memory walk (10 minutes of
   playback, 3 exports, 5 Previews), the rendering set again with the GPU off, the README's pictures retaken.
3. Corrective work found by it and done before closing (see *Findings*).

## The gates (control/02 §1)

| Gate | Observed |
| --- | --- |
| Line limit | PASS — no code file over 600 lines (the largest `src/ui/MainWindowProject.cpp`, 388) |
| Build | fresh Release: built in 98 s, **0 warnings**; ASan and Debug builds 0 warnings |
| Unit | Release `ctest`: **20/20** (114.6 s); ASan: **20/20** (memory growth reported, not judged — R-46); Debug: **20/20** incl. `avloop-leaks` |
| Form | `format.ps1 -Check` PASS (198 files); `check-icons` PASS (43 icons) |
| Secrets | `check-secrets` PASS |
| Licence | `avloop-cli version`: "PASS: LGPL build (no --enable-gpl)", libvpx, dav1d, zimg; no libx264/x265 |
| Memory | §3.7.8 walk PASS (below); Debug leak gate PASS — no debug-heap block left by 20 project, 20 preset, 8 decode, 8 encode and 3 export cycles (R-45) |
| ASan | all unit tests under `/fsanitize=address`: no AddressSanitizer report |
| GPU parity | AV-007's walk: 9 projects, GPU against CPU — mean ≤ 0.0191, max ≤ 1 (gate: ≤ 1.0 / ≤ 4) |
| CI | AV-010's run green (5 m 04 s); this commit's run watched below |
| README | pictures retaken by `tools/walks/readme-shots.json` (main window, timeline, crop, Preview, export, Settings — 14–289 KB each); the feature list checked |
| Phase system | `validate-phase-system` PASS, `check-continuity` PASS |

## The situations, observed on this build

| S | Observed |
| --- | --- |
| S1 | second start: window shown in 462 ms (first 491 ms); Navy and Black at 100 / 150 / 200 % (1904×993, 1920×1080, 2560×1440) pictured; only Open / Preview / Export in the top bar; 43 icons |
| S2 | 51 import checks: every sample opens (upright rotation, square SAR, deinterlaced, HDR tone-mapped, alpha kept — first/middle/last frames); the Bengali path opens; `broken.mp4` and `audio-only.m4a` refused with a message; HW decode on and off: the same frame (mean 0.000, max 0) |
| S3 | 200 + 200 random seeks exact (`test-decoder-seek`, SW 640p and D3D11VA 1080p); 40 cold seeks through avloop-cli exact (mean 209 ms); 10 s at 30 and 60 fps: 304 and 607 frames shown, 0 dropped; a minute of loop playback: 1,804 shown, 0 dropped |
| S4 | the Selection walk: 45 checks (move, scale, Fit/Fill/Center/Flip, the options bar, the upscale warning, the crop still) |
| S5 | the crop walk: 115 checks (three shapes, every ratio, Shift/Alt, snapping, arrows); exported ellipse: corner alpha 0, centre 255; Feather pictured in the Preview |
| S6 | the timeline walk: 86 checks (In/Out by tool, key and drag; labels); the loop walk: 40 (the crossfade band changes F) |
| S7 | every mode's frame count = L; seam / median: Simple 1.05, Crossfade 1.09, Ping-pong 0.84, Ping-pong smooth 0.05, Crossfade with alpha 1.30; no frame repeated |
| S8 | 15 / 24 / 30 / 60 / 12 fps × Nearest / Blend / Motion: D frames, `r_frame_rate` right, CFR (1/fps ± 0.67 ms); Speed 0.5× 120 frames, 2× 30 |
| S9 | Live frame 10 = avloop-cli frame 10 (mean 0, max 0); Encoded against Live mean 1.07 (compression); the Encoded file's alpha corner 0 / centre 255; six opens: Private Bytes 667 → 647 MB, VRAM 117 → 106 MB |
| S10 | VP9 and VP8: codec, W×H, `alpha_mode=1` with alpha (absent without), video only, one keyframe first, alpha plane by libvpx; Target size −1.4 % (VP9) and −0.5 % (VP8); a held destination written as `held (1).webm`; no `.part` after Cancel (UI walk) |
| S11 | every export read twice in a row through libvpx: 2L frames, no error (OBS itself not driven) |
| S12 | save → open → save: avloop-cli frame 10 of both identical (921,600 bytes, max 0); a moved folder opens by relativePath; a moved clip: not found, the log asks for Locate source…, located → opened; every command undone and redone (`test-commands`); crash → minidump (599,074 bytes) and the autosave; Discard and Recover |
| S13 | the memory walk and the GPU parity (above); with the GPU off (`AVLOOP_GPU_RENDERING=off`): S9's walk and check, S7/S8/S10/S11's walks all PASS, the six exports made on the CPU |

## The memory walk (§3.7.8)

```
node execution/AV-015-walk/walk-memory.mjs start        (detached: 11 minutes)
node control/lab/walk-phase.mjs execution/AV-015-walk walk-memory.mjs
PASS the run: playback, 3 exports, 5 Previews, idle: every step passed
PASS playback, Private Bytes: the 10th minute against the 2nd: peaks 606 MB → 596 MB (-9.8 MB, limit 50)
PASS playback, VRAM: the 10th minute against the 2nd: peaks 158 MB → 158 MB (-0.2 MB, limit 32)
PASS exports, Private Bytes: the 3rd against the 2nd: peaks 1156 MB → 1154 MB (-1.8 MB, limit 50)
PASS exports, VRAM: the 3rd against the 2nd: peaks 372 MB → 372 MB (+0.0 MB, limit 32)
PASS Previews, Private Bytes: the 3rd–5th against the 2nd: peaks 1007 MB → 1018 MB (+10.9 MB, limit 50)
PASS Previews, VRAM: the 3rd–5th against the 2nd: peaks 372 MB → 372 MB (+0.0 MB, limit 32)
PASS at rest, Private Bytes: the end against the pause after the 1st export: floors 691 MB → 624 MB (-67.0 MB, limit 50)
PASS at rest, VRAM: the end against the pause after the 1st export: floors 309 MB → 307 MB (-1.8 MB, limit 32)
     662 samples over 11.0 minutes; playback 594–596 MB; the highest 1156 MB, VRAM 372 MB; at rest 624 MB, VRAM 307 MB
```

The log is kept as `AV-015-walk/memory-cp-a1.csv`; the fresh logs of every phase's walk as `AV-015-walk/phase-logs/`.

## Findings — and the corrective work done before closing (AV-015a)

| Found | By | Done |
| --- | --- | --- |
| The render service destroyed the GUI thread's `QOffscreenSurface` on the render thread (Qt asserts in Debug) | the Debug build of `test-leaks` | `GlRenderer::shutdown()` on the render thread; the surface goes on the GUI thread |
| A finished export, estimate or Preview left its textures, mask, target and PBOs in the shared GPU context (~70 MB VRAM at rest) | the memory walk | `GlRenderer::trim()` / `RenderService::trim()` after each job |
| walk-phase passed a failed walk whose check quoted "WALK PASSED" | the memory walk's first check | the verdict matched at a line's start, with the exit code |
| AV-002's memory-decode walk measured its rounds against the first — the warm-up (rounds 2–4 equal, 514 / 507 / 512 MB) | this build (54.2 MB against round 1) | mended: four rounds, round 1 the warm-up, rounds cut by timed labels; same limits |
| Under ASan, test-memory's growth thresholds failed for ASan's quarantine | the ASan ctest | reported, not judged, under ASan; judged in Release (R-46) |
| GPU-off runs needed the settings of a lab run | S13 | `AVLOOP_GPU_RENDERING=off` (app and CLI) |

The first memory run (before `trim()`) is not counted: its windows were also cut by a fixed offset; the walk now reads
each phase by its timed label (`-- export 1 @612345 ms`, the `--mem-log` clock).

## Decisions recorded

- The checkpoint's findings and corrections: spec §0 *CP-A1*; lore *The checkpoint (CP-A1)* — six rules.
- **R-45** the Debug leak gate by heap growth; **R-46** ASan judges memory errors, Release the growth.

## Deviations from the phase contract

- **Code budget**: 200 planned; ~400 lines (test-leaks, the walks, `trim`/`shutdown`, the label clock, the env
  override). The corrective work is folded into this checkpoint (AV-015a's seam was the ASan and leak legs).
- **S11's OBS half**: OBS is not driven (no obs-websocket here); the libvpx half passes.
- **S3's 200 random seeks** are the unit test's (`test-decoder-seek`), not a GUI walk's.

## Blocked legs (if any)

None.

## Staged paths

- `src/`, `tests/`, `tools/walks/readme-shots.json`, `README.md`, `docs/SPEC.md`, `docs/lore/avloop.md`,
  `docs/screenshots/`, `phases/26-10-26-034934-AvLoop/`.

## Next

AV-011 — Keyframes, promoted to IN_PROGRESS in the same turn. No avloop process, session cache or walk temp file left
(`build/cp-a1-memory/` is under `build/`, never committed); the owner's focus untouched.
