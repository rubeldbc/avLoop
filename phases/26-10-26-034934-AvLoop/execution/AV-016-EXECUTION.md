# AV-016 — Execution Record

**Phase:** AV-016 — CP-B1, the whole walked (checkpoint after track B; items 1–14)
**Status:** DONE
**Date:** 2026-10-26
**Phase start commit:** `c6750cf` (*[AV-014] The installer: Inno Setup 6, the .avloop association, a clean uninstall*)
**ADR:** none owed
**Environment class:** local-disposable (this PC: `build/` fresh, `build-asan/`, `build-debug/`, `deploy/`,
`installer/Output/`, a per-user install in `%TEMP%\avloop-cpb1\app` for the walks — uninstalled after —, the walks'
temp folders; lab windows behind the owner's, never activated) and the GitHub Release `v1.0.0`

## What was done

1. Version 1.0.0. One fresh Release build (`scripts\build.ps1 -Fresh`, 111 s, 0 warnings); its ctest; an
   AddressSanitizer build and its ctest; a Debug build and its ctest with the leak gate.
2. `scripts\deploy.ps1`, the installer built and installed silently per user into a temporary folder; **every phase's
   walk run against the installed app** (`AVLOOP_BIN_DIR`, a lab change of this phase); the GPU-off set; the 10-minute
   memory walk on the installed app; the README's pictures taken from the installed app; then the installed copy
   uninstalled and S15 walked afresh (build, install, launch, association, uninstall).
3. The correction it needed (below), the Release `v1.0.0` with the installer and its SHA-256, the final README.

## The gates (control/02 §1)

| Gate | Observed |
| --- | --- |
| Line limit | PASS — 269 code files, none over 600 (the largest `src/ui/MainWindow.cpp`, 389) |
| Build | fresh Release 111 s, **0 warnings**; ASan 91 s and Debug 89 s, 0 warnings |
| Unit | Release **23/23** (110.7 s, rerun on the final build); ASan **23/23** (236 s, no AddressSanitizer report); Debug **23/23** incl. `avloop-leaks` (no debug-heap block left) |
| Form | `format.ps1 -Check` PASS (217 files); `check-icons` PASS (43 icons, avloop.ico 7 sizes) |
| Secrets | `check-secrets` PASS (564 files) |
| Licence | the installed `avloop-cli version`: "AvLoop 1.0.0 · LGPL version 3 or later · PASS: LGPL build (no --enable-gpl)" |
| Memory | §3.7.8 walk on the installed app PASS (below); Debug leak gate PASS |
| ASan | all unit tests under `/fsanitize=address`: no report |
| GPU parity | AV-007's walk on the installed app: 9 projects GPU against CPU, mean ≤ 0.0700, max 1 (gate ≤ 1.0 / ≤ 4) |
| CI | AV-014's run green (5 m 17 s); this commit's run watched below |
| README | final: v1.0.0, the Download link to the Release, pictures from the installed app (14–289 KB) |
| Phase system | `validate-phase-system` PASS (16 phases, 21 requirement ids), `check-continuity` PASS |

## The situations, observed on the installed build

| S | Observed |
| --- | --- |
| S1 | second start: window shown in 463 ms (first 474 ms); walk-shell 30 checks (top bar Open/Preview/Export only, the themes); walk-dpi: Navy and Black at 100/150/200 % |
| S2 | 52 import checks: every sample opens upright, deinterlaced, HDR tone-mapped, alpha kept; the Bengali path; broken files refused with a message; HW = SW frames |
| S3 | 40 cold seeks through the installed avloop-cli exact (1080p H.264, D3D11VA, mean 210 ms); 200 + 200 random seeks exact in `test-decoder-seek`; 10 s at 30 and 60 fps: 304 and 607 shown, 0 dropped; a minute of loop playback: 1,804 shown, 0 dropped |
| S4 | the Selection walk: 45 checks |
| S5 | the crop walk: 115 checks; exported ellipse corner alpha 0, centre 255 |
| S6 | the timeline walk 86 checks; the loop walk 40 (the crossfade band) |
| S7 | frame count = L in every mode; seam / median: Simple 1.05, Crossfade 1.09, Ping-pong 0.84, Ping-pong smooth 0.05, Crossfade with alpha 1.30 |
| S8 | walk-fps 62 checks: 15/24/30/60/12 fps × the three methods, `r_frame_rate` right, CFR; Speed 0.5× and 2× |
| S9 | Live frame 10 = avloop-cli frame 10 (mean 0.0000); Encoded against Live mean 1.065; the Encoded file's alpha corner 0 / centre 255; the Preview's memory back on close |
| S10 | VP9 and VP8: codec, W×H, `alpha_mode=1` with alpha and absent without, video only, one keyframe first, the alpha plane by libvpx; Target size −1.4 % (VP9, 3 passes) and −0.5 % (VP8); no `.part` after Cancel |
| S11 | every export read twice through libvpx: 2L frames, no error (OBS itself not driven, as at CP-A1) |
| S12 | save → open: identical render; relativePath and Locate; Undo/Redo of every command (`test-commands`); crash → minidump and autosave; Discard and Recover |
| S13 | the memory walk below; GPU parity above; with the GPU off: S7/S8/S9/S10/S11's walks PASS, exports made on the CPU |
| S14 | keyframes: Linear halfway 600/360, a quarter 400/360, Ease a quarter 325/360, holds before the first; the Finder on `loop4s.mp4`: the best 4.000 s, score 100, the next four a second apart; formats: GIF/APNG/MP4 (h264_mf)/ProRes correct in ffprobe; a 3-entry batch, a failure moving on, Pending after a crash |
| S15 | `AvLoop-1.0.0-setup.exe` 44.8 MB; `/VERYSILENT /CURRENTUSER` install; the installed app draws with only Windows on PATH; the association's command from the registry opens a project; the uninstall leaves only `%APPDATA%\AvLoop` |

## The memory walk (§3.7.8) — the installed app

```
PASS playback, Private Bytes: the 10th minute against the 2nd: peaks 607 MB → 598 MB (-9.5 MB, limit 50)
PASS playback, VRAM: peaks 230 MB → 230 MB (-0.2 MB, limit 32)
PASS exports, Private Bytes: the 3rd against the 2nd: peaks 1161 MB → 1159 MB (-1.1 MB, limit 50)
PASS exports, VRAM: peaks 372 MB → 372 MB (+0.0 MB, limit 32)
PASS Previews, Private Bytes: the 3rd–5th against the 2nd: peaks 1013 MB → 1020 MB (+7.3 MB, limit 50)
PASS Previews, VRAM: peaks 370 MB → 372 MB (+1.8 MB, limit 32)
PASS at rest, Private Bytes: floors 693 MB → 626 MB (-67.9 MB, limit 50)
PASS at rest, VRAM: floors 309 MB → 309 MB (+0.0 MB, limit 32)
     3260 samples (200 ms) over 11.0 minutes; the highest 1161 MB, VRAM 376 MB; at rest 626 MB, VRAM 309 MB
```

The log: `AV-016-walk/memory-cp-b1.csv`; every phase's fresh logs: `AV-016-walk/phase-logs/`; S15's pictures:
`AV-016-walk/installed.png`, `opened.png`.

## Findings — and the correction done before closing

| Found | By | Done |
| --- | --- | --- |
| The memory walk's first run on the installed app failed "exports, VRAM: 309 → 372 MB (+62.8)": the samples a second apart missed the 2nd export's brief GPU peak (its floor after, 309 MB, was the 1st's and the 3rd's) | the memory walk | `--mem-log-ms` (the app samples faster on request); the walk samples at 200 ms; rerun: 372 → 372 MB. The limits unchanged |

## The pictures

| Picture | What | What it shows — and what was seen in it |
| --- | --- | --- |
| `AV-016-walk/installed.png` | the installed 1.0.0 | the maximized window, the drop zone, every SVG icon drawn, "No video open" |
| `AV-016-walk/opened.png` | the association | the project opened from its command: the counter with its ellipse crop 400×280 · alpha, In 2 s – Out 5 s |
| `docs/screenshots/*.png` | the README | taken from the installed app: main window, timeline, Preview, ellipse crop, export toast, batch queue, Settings |

## The Release

`gh release create v1.0.0` on this phase's commit: `AvLoop-1.0.0-setup.exe` (46,931,456 bytes) and
`AvLoop-1.0.0-setup.exe.sha256` (SHA-256 `86560A470666A0713877EBDF2BBCDC9FD8C4E9967007EA02BEB46EA492A9ABFC`);
`gh release view v1.0.0` after it (see the close).

## Decisions recorded

- **R-5** taken: the Release `v1.0.0` with the installer. Spec §0 *CP-B1*; lore *The checkpoint (CP-B1)* — two rules.
- The programme's Completion record in `AUTONOMOUS_STATE.md`.

## Deviations from the phase contract

- **S11's OBS half**: OBS is not on this PC (no obs-websocket); the libvpx half passes — as at CP-A1.
- **S3's 200 random seeks** are the unit test's; the installed app's 40 cold seeks through avloop-cli.
- **Code**: a lab option (`--mem-log-ms`) and the lab's `AVLOOP_BIN_DIR`; no feature.

## Blocked legs (if any)

None. No `BLOCKED_EXTERNAL` in the programme.

## Staged paths

- `CMakeLists.txt`, `src/app/main.cpp`, `README.md`, `docs/SPEC.md`, `docs/lore/avloop.md`, `docs/screenshots/`,
  `phases/26-10-26-034934-AvLoop/`.

## Next

None — the programme is COMPLETE. No install, avloop process, session cache, lab folder or walk temp file left; the
owner's focus untouched; the owner's registry and folders as they were.
