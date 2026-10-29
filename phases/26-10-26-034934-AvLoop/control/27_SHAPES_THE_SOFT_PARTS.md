# 27 Shapes — the soft parts (item 24, AV-027, late)

Canonical current execution state: `../AUTONOMOUS_STATE.md`

The owner's words of 2026-10-29 (`../README.md` §1, item 24, [5]): *"It should be possible to select specific areas
within a shape and apply custom feathering or transparency, allowing any part of the selection to be made
transparent."* D-024. Readings R-90 … R-91.

## 1. The reading

Every shape gets its own **Feather** (0 … 200 output px, as the crop's) and **Strength** (0 … 100 %): how far its
edge softens and how strongly it applies. A part of a selection is made transparent by a Cut shape drawn over it at
the transparency wanted — Strength 40 % cuts 40 %, 60 % stays; its Feather softens the region's edge. A Select/Add
shape at 60 % adds 60 % coverage; Keep only at 60 % keeps 60 % outside its area too (r·(1 − 0.6·(1 − s))); Exclude
scales likewise (R-90). The frame's own feather stays the crop's.

## 2. The maths (`src/render/MaskBuilder`)

- A shape's coverage s(x, y): its signed distance d in output px (negative inside) through §6.1's formula
  clamp((0.5 − d) / max(f, 1), 0, 1), then × strength. Rect, Rounded and Ellipse by their analytic distance (the
  crop's `distance`, moved to the shape's centre); Polygon and Path by an **exact Euclidean distance transform**
  (Felzenszwalb–Huttenlocher, two passes, O(W·H)) of the antialiased raster: outside distance from the raster's
  inside set, inside distance from its outside set, the edge pixel's own coverage refining the half pixel (R-91).
  At feather 0 the raster's antialiasing is the coverage, as AV-024.
- The soft ops of AV-026 take s as it is. Memory: the running mask, one shape's raster and the EDT's two float rows
  — bounded by the output size, no image a shape.
- The cache key takes feather and strength.

## 3. The tools and the panel

- The options bar of S and P: Feather and Strength numbers for the selected shape(s) — typed, stepped, wheeled; a
  value typed with several selected sets all.
- The Crop section's shape rows: Feather, Strength; the list's row shows "· 20 px · 60 %" when not the defaults.
- The overlay: the dim at a Cut shape of Strength p is p × 60 % (the canvas hints the transparency; the Preview is
  exact, R-22); feather not drawn on the canvas.
- The file: `"feather"`, `"strength"` a shape (absent: 0 and 100), version 3.

## 4. Tests and walk

`test-mask`: a Cut ellipse at Strength 50 inside a Select rect — the centre 128 (±1); Feather 20 on a polygon — a
ramp monotonic over ~20 px measured along a normal; the EDT against brute force on a 64 × 64 raster (max error ≤ 0.5
px); feather 0 byte for byte AV-024's. `walk-soft.json`: a rectangle selection, a Cut ellipse within it at 40 %,
feathered 30 px — the list's row, the Preview's half-transparent window; Strength 100; Undo; two shapes selected and
one Feather typed for both. `walk-soft-render.mjs`: avloop-cli frames — alpha at the cut's centre 153 (±1), on its
feather ramp between, outside 255; GPU = CPU; AV-024's and AV-026's walks again (their shapes at the defaults).
