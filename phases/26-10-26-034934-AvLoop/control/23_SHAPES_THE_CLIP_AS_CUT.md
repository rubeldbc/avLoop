# 23 Shapes — the clip as cut (item 20, AV-022, late)

Canonical current execution state: `../AUTONOMOUS_STATE.md`

The owner's words of 2026-10-29 (`../README.md` §1, item 20, [1]): *"Users should be able to export without applying
a loop at the end, or without adding a crossfade in the middle."* D-020. Readings R-77 … R-78.

## 1. The reading

The four modes of §8.2 all make a loop: Simple cuts hard at the seam, Crossfade blends the end into the start,
the Ping-pongs run back. "Without applying a loop at the end, without a crossfade" is read as **a fifth mode, None**:
the clip from In to Out, as cut — no seam treatment, no blend, no return — presented as a clip, not a loop (R-77).
Simple stays what it is (a loop whose seam is a hard cut); None is the first entry of the Mode list so it is found.

## 2. The maths (`src/core/LoopMap`, `LoopSettings`)

- `LoopSettings::Mode::None`, id `"none"`; `length` = D, `pick(k)` = {k, k, 0} — Simple's frames; D ≥ 2 as §8.1.
- `LoopPlan`: `fadeClamped` and `turnClamped` false; the plan's `length` D.
- The file: `"mode": "none"` under `loop`. A new value of an old key: 1.1.0 would read it as Crossfade. The file's
  version is bumped to 3 by the next phase (AV-023, the first to add keys); this phase writes `"none"` under
  version 2 only on main between the two phases, never in a Release (R-78).

## 3. Where None shows

- The Loop section: Mode "None (the clip as cut)"; Crossfade, Curve and Turn rows hidden as Simple hides them; the
  summary reads `Clip 4.97 s · 149 frames @ 30 fps` instead of `Loop …`.
- The timeline: no band, no turn marks (as Simple).
- The Preview: no seam mark on the slider, Around seam disabled with the tip "The clip is not a loop" (the player
  still repeats — a Media Source repeats a clip too).
- The export: the suggested name `<source>_clip_<W>x<H>.<ext>` instead of `_loop_`; the batch's name template
  likewise; everything else (formats, quality, alpha) unchanged.
- avloop-cli: `frames` and `export` take the mode from the file; `version` unchanged.
- The Loop Finder: unchanged (it finds In/Out pairs; the mode is the user's).

## 4. Tests and walk

`test-loopmap`: None's length and picks; `test-project-json`: `"none"` round trip; `test-export`: a None export has D
frames, the first the frame at In. `walk-cut.json`: the counter sample; Mode None; the summary's words; the band gone;
the Preview with no seam mark; the export's suggested name; Undo back to Crossfade. `walk-cut-render.mjs`: avloop-cli
frames of a None project — frame 0 = In's frame, frame D − 1 = Out's last frame, D frames, byte for byte Simple's.
