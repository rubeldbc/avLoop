# 14 Shapes — the keyframes (item 11, AV-011)

Canonical current execution state: `../AUTONOMOUS_STATE.md`

Spec §6.2, §6.4, §7.1 row 5, §12.2. D-011.

## 1. The model (`core/Keyframes`)

- A keyframe: source time `t`, `x`, `y`, `scale`, `interp` (linear | ease). Flips are not keyframed: they hold (the
  transform's flips apply at all times).
- `valueAt(t)`: no keyframes → the static transform; before the first → the first; after the last → the last;
  between k and k+1 → `lerp(k, k+1, u)` with `u = (t − tk)/(tk+1 − tk)`, eased by `u²(3 − 2u)` when keyframe k is
  Ease. Keyframes outside In–Out still count (§12.2).
- Commands: AddKeyframe (or update at the same time within half a frame), MoveKeyframe (merge within one drag),
  DeleteKeyframe; the file's `transform.keyframes` as §11.1.

## 2. The tool and the Selection

K (the Keyframe tool) adds/updates a keyframe at the playhead with the current transform. Options: Add, Delete, Prev
(`[`), Next (`]`), Interpolation (Linear/Ease) of the selected keyframe. With keyframes present, a Selection change
writes into the keyframe at the playhead, creating one there if none (like After Effects).

## 3. The row

Diamonds on the keyframe row (16 px): selected solid orange, others hollow; Ease diamonds with rounded corners;
click selects; drag moves in time (snapped to the output grid); Delete removes the selected; the playhead snaps to
them within 8 px.

## 4. Rendering

`FrameRenderer` and the canvas take `Keyframes::valueAt(sourceTime)` for each source frame drawn (each pick's own
time).

## 5. Tests and walks

`test-keyframes` (values between two keyframes for Linear and Ease at u = 0.25/0.5/0.75; before/after; outside In–Out;
commands). Walks: `walk-keyframes.json` (K at two times, a Selection drag between, Ease set, a diamond dragged,
pictures of the row) and `walk-keyframes.mjs` (`avloop-cli frame` at the middle: the video's centre measured by its
bright pixels halfway for Linear, and the Ease curve's value at a quarter).
