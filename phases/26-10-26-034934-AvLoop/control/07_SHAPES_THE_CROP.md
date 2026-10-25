# 07 Shapes — the crop tools (item 4, AV-004)

Canonical current execution state: `../AUTONOMOUS_STATE.md`

Spec §3.3, §6.1. D-004. Reading R-8.

## 1. The model

- `CropShape`: `kind` (Rect, Rounded, Ellipse), rect (x, y, w, h in scene px), `radius` (0 … half the short side),
  `feather` (0–200 output px), `ratio` (id + custom W:H); "no crop" = an empty rect → the whole video rectangle is
  used by Preview and Export.
- `AspectRatio`: Free, Original (the video's display ratio), 1:1, 4:3, 3:4, 16:9, 9:16, 21:9, 3:2, 2:3, 4:5, Custom;
  `apply(rect, ratio)` keeps the centre and the area as close as possible (w' = √(A·r), h' = √(A/r)).
- Output size (`Geometry::outputSize`): the crop's w×h (or Custom/Long side at AV-009); long side > 1920 → scaled
  down keeping the ratio; both rounded to the nearest even, minimum 16.
- `needsAlpha(project)`: not (Rect, feather 0, fully inside the video) — Alpha Auto's rule.
- `SetCrop` command (merge within one drag).

## 2. The tool (`CropTool.cpp`, `CropHandles.cpp`)

- No crop: drag draws (marquee) — Shift locks the ratio (1:1 when Free; a circle for Ellipse), Alt draws from the
  centre; under 16 px → cancelled. With a crop: drag inside moves; the 8 handles resize (Shift ratio, Alt centre);
  drag outside draws a new crop replacing the old (one Undo step).
- Keys: arrows 1 px, Shift+arrows 10 px; Esc cancels a drawing; Delete removes the crop; double-click = the whole
  video rect. M/U/E switch the kind keeping the rectangle.
- Snapping: the moving edges (and centre) to the video's edges and centre within 8 screen px; Ctrl held → off; the
  snap lines drawn while they hold.

## 3. The overlay

60% black outside the crop; a 1.5 px orange (`accent`) border following the shape; for Ellipse and Rounded a dashed
bounding box; 8×8 white handles with orange borders; while dragging: the rule-of-thirds guides and a label
`640 × 640 → 640 × 640` (crop size → output size) near the cursor.

## 4. The mask (`render/MaskBuilder`)

- Built at the output size from the crop's kind, radius and feather (in output px), cached by those inputs; an 8-bit
  coverage image. Signed distance d from the shape's edge (inside negative): Rect (box SDF), Rounded (rounded-box SDF
  with the radius scaled to output px), Ellipse (the ellipse SDF approximation by normalized gradient); coverage =
  clamp((−d − f/2) / max(f, 1) + 0.5, 0, 1) shaped so that feather 0 still gives a 1 px anti-aliased edge and feather
  f softens inward from the edge.
- Both renderers use this one mask (AVADR-0001): the GPU uploads it as a texture sampled at pixel centres.

## 5. The panel and options

`CropOptions` (Ratio, X, Y, W, H, Swap W/H, Clear; Radius for Rounded; Feather for Rounded/Ellipse — and Rect),
`SidePanel` with its four `Section`s and `CropPanel` (shape, ratio, size, radius, feather, the output size readout).
Status: "Transparent area in output" when a part of the crop is outside the video; "Output is fully transparent"
when the crop is entirely outside.

## 6. Tests and walk

`test-aspect` (every ratio, centre and area kept), `test-mask` (Ellipse corners 0 and centre 255; Feather 20 steps
down over ~20 px monotonically; feather 0 has a ≤ 1 px ramp; Rounded radius; PNGs written to the walk folder when
`AVLOOP_TEST_OUT` is set). Walk `walk-crop.json`: each shape drawn, every ratio chosen, Shift/Alt, arrows, snapping on
and off, Delete, double-click; pictures of each shape.
