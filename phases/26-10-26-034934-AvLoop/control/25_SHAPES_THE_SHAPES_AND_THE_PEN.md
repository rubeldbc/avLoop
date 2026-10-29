# 25 Shapes — the shapes and the pen (item 22, AV-024 and AV-025, late)

Canonical current execution state: `../AUTONOMOUS_STATE.md`

The owner's words of 2026-10-29 (`../README.md` §1, item 22, [3] and the later [1]): *"Users should be able to
create custom shapes, which can then be used to define a selection or an inverse selection. Additionally, standard
shapes—such as rectangles and ellipses—can also be created and used for selection or inverse selection."* and
*"there will be cutoma shape creation like illustrator will be available with full pen tool"*. D-022. Readings
R-82 … R-87. Two phases: AV-024 the shapes, AV-025 the pen.

## 1. The reading

The crop stays the **frame**: the one rectangle that becomes the output (R-8). Inside it any number of **shapes**
say what is kept: a shape that *selects* keeps its inside, a shape that *cuts* makes its inside transparent (the
inverse selection of item 21, by shape). With no shapes the mask is the frame's own, byte for byte as before (R-82).
Shapes live in scene px like the crop; they are not keyframed (the crop is not, §1.3). A standard shape is a
rectangle, a rounded rectangle or an ellipse; a custom shape is a polygon of clicked points, a freehand outline, or
— AV-025 — a path of Bézier anchors drawn by the Pen as Illustrator's.

## 2. The model (`src/core/MaskShape`, `CropShape::shapes`)

```
struct MaskShape {
  Kind kind;            // Rect, Rounded, Ellipse, Polygon, Path (AV-025)
  Role role;            // Select, Cut                  (AV-026 widens it to Add, Cut, KeepOnly, Exclude)
  QRectF rect; double radius;        // the standard kinds
  QList<PathPoint> points;           // Polygon: anchors; Path: anchor + in/out handles, smooth flag
  QString name;  bool visible = true; bool locked = false;    // AV-029 shows these
  double feather = 0; double strength = 100;                  // AV-027
};
```

`CropShape` gains `QList<MaskShape> shapes` (at most 64, AVADR-0002) and `int selected` (the shape the tools act on;
not in Undo, not in the file). The file: `"shapes": [ { "kind", "role", "x","y","w","h", "radius", "points": [[x,y],
…] } ]` under `crop`, version 3 (R-83). `needsAlpha`: true with any Cut shape or a Select shape not covering the frame.
Undo: `changeCrop` carries the shapes — one step a drawing, a move, a point drag (merged within the drag), a delete.

## 3. The mask (`src/render/MaskBuilder`)

- Each shape rasterized at the output size as a QPainterPath (rect, rounded rect, ellipse, polygon, cubic path)
  with antialiasing — the 1 px edge of §6.1 — into an 8-bit coverage; one running mask, never one image a shape:
  running = Select: max(running, s); Cut: running × (1 − s). Start: 0 when any shape selects, else 1 (cuts alone
  cut from the whole frame). The frame's own coverage (kind, radius, feather, invert) multiplies the result (R-84).
- The cache key is a hash of the shapes with the frame's inputs; a build is one pass per shape.
- GlRenderer and CpuRenderer unchanged (AVADR-0001: the one mask).

## 4. The tools

- **S — the Shape tool** (one rail button, the options bar choosing Kind: Rectangle, Rounded, Ellipse, Polygon,
  Freehand; and Role: Select, Cut). Rectangle, Rounded, Ellipse: a drag draws (Shift square/circle, Alt from the
  centre; under 16 px cancelled); Polygon: a click an anchor, a click on the first anchor or Enter closes (three or
  more), Esc cancels; Freehand: a drag draws, the release closes, the points simplified to 1 scene px
  (Douglas–Peucker). With shapes: a click selects the topmost under the pointer; a drag inside moves it; a selected
  standard shape has the crop's eight handles; a polygon's anchors are square handles, dragged one at a time;
  Delete removes the selected shape; arrows nudge; Esc deselects. The crop's snapping and Ctrl apply (R-85).
- **P — the Pen tool** (AV-025): a click a corner anchor; a click-and-drag a smooth anchor with its two handles
  (mirrored); a click on the first anchor closes the path, Enter closes it by a straight segment, Esc cancels. On a
  path: an anchor dragged moves it; a handle dragged turns its pair (Alt breaks the pair — a corner with two
  handles); Alt+click on an anchor converts smooth ↔ corner; a click on a segment adds an anchor there; Delete removes
  the selected anchor (two left removes the shape); Ctrl held is the direct selection (move anchors without adding).
  A polygon or a freehand shape edited by the Pen becomes a Path (R-86). "Full pen tool" as Illustrator's: anchors,
  handles, add, delete, convert, close — no pressure, no width profiles (not asked).
- The crop tools (M, U, E), the Selection and the others unchanged; a shape's handles are drawn only by S and P.

## 5. The canvas and the panel

- The overlay: the dim covers what the mask cuts (the hard QPainterPath booleans of the shapes — Select united, Cut
  subtracted — intersected with the frame's shape; strength and feather are the Preview's, R-22); every shape's
  outline in the accent (a cut shape dashed); the selected shape's handles; a path's handles as thin lines with
  round ends.
- The Crop section: a "Shapes" subhead; a compact list, one row a shape (its kind's icon, "Rectangle 2", the role),
  the selected one highlighted; under it the selected shape's rows — Role, X / Y, W / H (standard kinds), Radius
  (Rounded), Delete. AV-029 grows this list into the layers panel.
- Icons: shape (the rail), pen (the rail), polygon, freehand, cut (the role) — the 46th … 50th (R-87).
- Probes: `shapes.count`, `shapes.selected`, `shapes.summary` (kind:role:x,y,w,h a shape), `mask.at` (the mask's
  value at an output px, from the render walk).

## 6. Tests and walks

`test-mask`: a Select ellipse inside a Rect frame (the centre 255, the corner 0); a Cut rect (its inside 0); two
Select shapes united; no shapes byte for byte AV-004's mask; a polygon's inside and outside; a cubic path's. `test-
project-json`: the shapes' round trip; a version-2 file without. AV-024 `walk-shapes.json`: S; a rectangle, an
ellipse, a polygon of five clicks, a freehand drag — each selected, moved, resized, one Undo step; Role Cut; Delete;
the list's rows; the Preview. `walk-shapes-render.mjs`: avloop-cli frames — the mask's values at known px, GPU = CPU.
AV-025 `walk-pen.json`: a path of corner and smooth anchors closed on its first; an anchor converted; one added on a
segment, one deleted; a handle broken with Alt; Esc; the Preview. `walk-pen-render.mjs`: the path's inside and
outside, a smooth curve's edge between two anchors neither inside nor outside its chord.
