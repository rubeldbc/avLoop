# 09 Shapes — the loop engine (item 6, AV-006)

Canonical current execution state: `../AUTONOMOUS_STATE.md`

Spec §8 entire. D-006. The maths are pure functions in `core/`, unit-tested without any video.

## 1. Definitions (spec §8.1)

`fps` the output rate (rational), `s` Speed, `tIn`, `tOut`. `D = round((tOut − tIn) × fps / s)`, minimum 2 (3 for
Ping-pong). `τ(i) = tIn + i × s / fps`. `S[i]` = the image at τ(i) by the FPS method. `O[k]`, `L` frames.

## 2. `LoopMap` — output frame → segment frames

`struct Pick { int a; int b; double w; }` (one frame when `w == 0`, else `(1−w)·S[a] + w·S[b]`).

- Simple: L = D; `O[k] = S[k]`.
- Crossfade: F clamped to 1 … ⌊D/2⌋; L = D − F; for 0 ≤ j < D − 2F: `S[F+j]`; for D − 2F ≤ j < D − F:
  `(1 − w)·S[F+j] + w·S[j − (D − 2F)]`, `w = c((j − (D − 2F) + 1)/(F + 1))`, `c(x) = x` or `x²(3 − 2x)`.
- Ping-pong: L = 2D − 2; `O[k] = S[k]` for k < D, `S[2D − 2 − k]` for k ≥ D.
- Ping-pong + Smooth turn: T clamped to 1 … ⌊(D − 1)/2⌋; `a = T/(D − 1)`; forward `x = k/(D − 1)`, backward
  `x = (2D − 2 − k)/(D − 1)`; `p = (D − 1) · e(x)/(1 − a)` with `e` of §8.2; a fractional `p` blends ⌊p⌋ and ⌈p⌉.

## 3. `TimeMap` — segment frame → source frames

- Nearest: the source frame whose PTS is nearest τ(i) (a tie → the earlier).
- Frame Blend: the frames before and after τ(i), weighted by time.
- Motion Interpolation: the frame of the `minterpolate` pre-pass at the output grid (`Interpolator`), from the disk
  cache.
- VFR: by PTS always (the frame index table built from the decoder's PTS list for the segment).
- Output: one or two `(pts, weight)`; combined with `LoopMap` this gives up to four source frames for one output
  frame (the renderer blends).

## 4. FPS and Speed

FPS choices: Source (`avg_frame_rate`; VFR → the nearest of the common list), 15, 23.976, 24, 25, 29.97, 30, 50,
59.94, 60, Custom (1–60, three decimals); maximum 60. In, Out, F and T sit on the output grid; when the FPS changes the
seconds stay and the frame counts are recalculated. Speed 0.10×–4.00× step 0.05; output length = (tOut − tIn)/s; with
Speed < 1 and Nearest the hint "Motion Interpolation gives smoother slow motion".

## 5. The Loop panel and the band

`LoopPanel`: Mode, Crossfade (s and frames, linked), Curve, Turn (s), Speed, FPS method; the summary `Loop 4.97 s ·
149 frames @ 30 fps` (`L / fps`); a value clamped flashes `warning` for 600 ms. The Output section's FPS row. The
timeline's crossfade band: a gradient transparent → `accent` over the F frames before Out and a dashed outline over
the F frames after In; dragging the gradient's left edge sets F (one Undo step); Smooth turn: both ends' T frames
filled quiet orange. `SetLoop` and `SetOutput` commands.

## 6. `Interpolator`

`buffer` → `minterpolate=fps=<out>:mi_mode=mci:mc_mode=aobmc:vsbmc=1` → `buffersink` over the segment's source
frames (normalized, working size), the results written to the disk cache keyed by (settings hash, i); progress and
cancel; invalidated when the range, Speed, FPS or working size change.

## 7. Tests and walk

`test-loopmap`: L for each mode over D = 2 … 400 and F/T at bounds; Crossfade's `O[0] = S[F]` and the dominant part of
`O[L−1]` = `S[F−1]`; Ping-pong's turns appear once (no two equal consecutive picks); Smooth turn monotone halves and
positions at a, ½, 1 − a. `test-timemap`: the three methods on a synthetic PTS list (CFR 30, VFR jitter, 23.976 → 30,
Speed 0.5 and 2). Walk `walk-loop.json`: modes switched, the summary read, the band dragged (F read back), a picture.
