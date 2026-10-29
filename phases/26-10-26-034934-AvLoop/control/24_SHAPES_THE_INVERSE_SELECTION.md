# 24 Shapes — the inverse selection (item 21, AV-023, late)

Canonical current execution state: `../AUTONOMOUS_STATE.md`

The owner's words of 2026-10-29 (`../README.md` §1, item 21, [2]): *"A feature should be implemented to allow for
the 'inverse selection' of a chosen area, ensuring that the unselected (inverted) portion is included in the
output."* D-021. Readings R-79 … R-81.

## 1. The reading

The crop is the chosen area: a rectangle, a rounded rectangle or an ellipse with a feather. **Inverse** turns it
into a hole: the output is the whole video's rectangle (where it stands at In, as with no crop — R-8, R-49), and the
crop's shape is transparent in it, the feather softening outward from the shape's edge into what is kept (R-79). The
crop's rectangle, ratio, radius and feather keep their meaning; its tools, handles, snapping and keys work as before —
only what is kept flips.

## 2. The model (`src/core/CropShape`)

- `bool invert = false`; the file's `"invert"` under `crop` (absent = false).
- `geometry::exportFrame`: with `invert` the video's rectangle (the no-crop rule); `needsAlpha`: true with `invert`
  (a hole is transparent); `coverage` likewise reports Partly.
- `Project::exportFrame()`, `outputSize()`, `RenderJob::of` read it through `geometry`; the Output section's size
  readout follows (the video's size, scaled by Long side or Custom as before).
- **The file's version 3** (R-80): the first key added since the Release 1.1.0. `ProjectMigrate` 2 → 3 writes the
  number; a version-2 file opens with `invert` false; 1.1.0 refuses a version-3 file with its message (as R-68). The
  phases after this one add their keys under version 3 while it is unreleased (as R-73 did under 2).

## 3. The mask (`src/render/MaskBuilder`)

- `coverage` with `invert`: 1 − the shape's coverage over the frame, the shape placed at its scene position inside the
  frame (output px = (scene − frame.topLeft) × k, k = out.w / frame.w). Feather 0 keeps the 1 px anti-aliased edge;
  feather f softens over f output px outward from the edge, into the kept part (the mirror of §6.1's inward).
- The cache key takes `invert` and the shape's offset in the frame.
- Both back ends keep using the one mask (AVADR-0001): nothing changes in GlRenderer or CpuRenderer.

## 4. The canvas and the panels

- The overlay (`CanvasOverlay`): the dim (60 % black) covers the shape instead of the outside; the outline, the
  dashed box and the handles as before; the video's rectangle gets the thin accent outline the export frame has.
- The Crop section: an "Inverse" check box after Shape ("The chosen area is cut out; the rest is kept"); the Output
  row shows the video's size with "· alpha". The crop options bars: an "Inverse" icon toggle (a new icon, the 45th:
  a square with a hole) beside Clear.
- Delete (the crop removed) clears `invert` too; a new crop drawn keeps the current `invert` (R-81). One Undo step,
  "Invert the crop".
- Probes: `crop.invert`, `panel.crop.output`.

## 5. Tests and walk

`test-mask`: an inverted ellipse — the centre 0, the corners 255; feather 20 steps up over ~20 px from the edge
outward; `test-geometry`: the frame and alpha with `invert`; `test-project-json`: version 3 written, a version-2 file
read, `invert` round trip, a version-4 file refused. `walk-inverse.json`: the counter sample, an ellipse crop,
Inverse on — the dim inside, the Output row 640 × 360 · alpha, the Preview showing the hole; feather; Undo; M then a
new rectangle keeps Inverse; Delete clears it. `walk-inverse-render.mjs`: avloop-cli frames — the hole's centre alpha
0, a corner's 255, the GPU and the CPU byte for byte; a version-2 file's frame unchanged from AV-020's.
