# 29 Shapes — the align tools and the shape layers (item 26, AV-029, late)

Canonical current execution state: `../AUTONOMOUS_STATE.md`

The owner's words of 2026-10-29 (`../README.md` §1, item 26, the later [2]): *"There will be align tool set for
multiple object and make a layer system for multiple objects"*. D-026. Readings R-94 … R-96.

## 1. The reading

"Objects" are the shapes of item 22 on the canvas (the Illustrator context of the sitting); the video itself has
Fit, Fill and Center (R-94). The **align tool set** is Illustrator's Align panel: align the selected shapes' edges
and centres to each other or to the frame, distribute them evenly. The **layer system** is the Shapes list grown
into a layers panel: order (top is drawn and combined last), visibility, lock, names, multi-selection — what Illustrator's
Layers panel gives a flat list of objects (no nested groups: not asked, R-95).

## 2. Align (`ui/options/AlignBar`, `core/ShapeAlign`)

- With two or more shapes selected, the options bar of S and P shows: Align left, centre, right, top, middle,
  bottom; Distribute horizontally, vertically (equal gaps between bounding boxes); "To:" Selection / Frame (the crop's
  rectangle, or the video's with no crop). With one shape selected and To: Frame, the align moves it to the frame.
- The maths in `core/ShapeAlign` (pure, tested): each shape's bounding box (a path's by its curves' bounds); the
  target edge or centre; the shapes translated (never resized). One Undo step, "Align shapes" / "Distribute shapes".
- Keys as Illustrator's are not taken (Shift+F7 is Qt-free but obscure): buttons only (R-96).
- Icons: six aligns, two distributes — the 55th … 62nd.

## 3. The shape layers (`ui/panels/ShapeLayers`)

- The Crop section's Shapes list becomes a `ShapeLayers` widget: one row a shape — the eye (visible), the lock, the
  kind's icon, the name (double-click renames, "Rectangle 2" by default), the role's icon; the top row the topmost.
  Click selects, Shift+click a range, Ctrl+click toggles; a drag reorders (the mask's order changes with it); the
  buttons under it: Up, Down, Duplicate, Delete, Combine… (AV-026's). A hidden shape is out of the mask and the
  dim; a locked one is not selectable on the canvas nor movable (its row still selects it to unlock).
- The file: `"name"`, `"visible"`, `"locked"` a shape (absent: default, true, false), version 3.
- The canvas: a click selects the topmost *visible, unlocked* shape; the tools, the align bar and the panel share
  `CropShape::selected` (a list, AV-026).
- Undo: reorder, hide, lock, rename, duplicate each one step.

## 4. Tests and walk

`test-shapes`: align's six targets and the two distributes on three boxes (positions exact); hidden shapes out of
the mask; the order's effect. `walk-align.json`: three shapes; Align left (the probes' x equal), centre to the frame,
Distribute horizontally (equal gaps); the layers list — rename, hide (the dim and `mask.at` change), lock (a canvas
click selects the one beneath), reorder by drag (a Cut moved above an Add changes the mask); Duplicate; Undo through
them. `walk-align-render.mjs`: a hidden shape's project rendered without it, the reordered one's mask, GPU = CPU.
