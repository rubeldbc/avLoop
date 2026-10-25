# 15 Shapes — the loop finder (item 12, AV-012)

Canonical current execution state: `../AUTONOMOUS_STATE.md`

Spec §11.5. D-012.

## 1. The finder (`export/LoopFinder`)

- Input: the search range (default the current In–Out ± 5 s, or the whole file), minimum and maximum length (default
  2–10 s), the crop region and transform (fingerprints are of what will be exported).
- Fingerprints: for each source frame on the output grid in the range, the crop region rendered small and reduced to
  64×64 greyscale (4 KB a frame), by its own decoder; progress; Cancel.
- Score of a pair (a, b), b − a within the lengths: `mad(a, b)` (the mean absolute difference of the fingerprints) +
  `motion(a, b)` = `mad(a − (a−1), b − (b−1))` (the frame-to-frame difference images compared); lower is better.
- All pairs scored by `QtConcurrent::blockingMapped` over rows a; the best 5 by score with no two within 1 s (both
  ends); score shown 0–100 = `100 × (1 − raw / rawMax)` where rawMax is the 95th percentile of the raw scores (stable
  against outliers).

## 2. The window (`ui/export/LoopFinderWindow`)

Menu → Loop Finder (Ctrl+L): range, min/max length, Find; progress + Cancel; the list (In, Out, length, score,
thumbnails of both ends from the fingerprints' source frames at 120×68); click = SetRange (one Undo step);
double-click = set and open Preview. Closing releases the fingerprints.

## 3. The sample and tests

`make-samples.mjs` adds `loop4s.mp4`: a 20 s video whose content repeats every 4 s exactly (a generated pattern with
motion), so the best pairs are (t, t + 4 s) etc. `test-loopfinder`: synthetic fingerprints with a period → ranked
first within ±2 frames; the 1 s separation; Cancel. Walks: `walk-finder.mjs` (the CLI's or the window's result on
`loop4s.mp4`: best length 4.000 s ± 2 frames), `walk-finder.json` (the window pictured, a click sets In/Out).
