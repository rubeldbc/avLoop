# 26 Shapes — the shape combinations (item 23, AV-026, late)

Canonical current execution state: `../AUTONOMOUS_STATE.md`

The owner's words of 2026-10-29 (`../README.md` §1, item 23, [4]): *"Similar to Adobe Illustrator, it should be
possible to create new shapes by combining or permuting multiple existing shapes; these new shapes can then be used
for selection."* D-023. Readings R-88 … R-89.

## 1. The reading

Illustrator's Pathfinder: **Unite, Minus front, Intersect, Exclude**. Two ways, both given (R-88):

- **Live**: a shape's Role becomes one of Add (unite with what is below it), Cut (minus), Keep only (intersect with
  what is below), Exclude (what is in one, not both). The shapes combine in their order, bottom to top, each op on
  the running result; the roles Select and Cut of AV-024 are Add and Cut (the file's `"select"` read as `"add"`).
- **Combine**: the selected shapes (Shift+click, or Ctrl+A for all) baked into one Path shape by the chosen op —
  Combine ▸ Unite / Minus front / Intersect / Exclude — one Undo step, "Combine shapes"; the result takes the
  bottom shape's role and place, the others are gone; its anchors are the Pen's to edit.

## 2. The maths

- Live, on the running coverage r and the shape's s (0 … 1): Add max(r, s); Cut r·(1 − s); Keep only r·s; Exclude
  r + s − 2·r·s. Hard shapes (AV-024, no feather) give the same bytes as AV-024's max and ×(1 − s) — the walks of
  AV-024 and AV-025 pass again unchanged.
- Combine: `QPainterPath::united`, `subtracted`, `intersected`; Exclude = united − intersected. A standard shape
  enters as its path (a rounded rect's corners stay curves); the result is one closed path (or several subpaths —
  one Path shape with subpaths, drawn and masked by the non-zero rule; R-89). A cubic path is combined by Qt's
  path booleans on its curves; the anchors of the result are Qt's — a smooth join stays smooth where Qt keeps it.

## 3. The tools and the panel

- The Shape and Pen tools: Shift+click adds to the selection, Ctrl+A selects every shape, a drag on empty canvas
  with S draws a marquee that selects what it touches (`CropShape::selected` becomes a list — not in Undo, not in
  the file); Delete removes all selected; a move moves them together.
- The options bar of S and P: Role (the four), and Combine ▸ (a menu button; enabled with two or more selected).
- The Crop section's list: the role shown as its icon (add, cut, keep, exclude — the 51st … 54th icons); Role row
  the four; a "Combine…" button under the list with the same menu.
- The overlay: the dim by the same four ops on QPainterPaths.

## 4. Tests and walk

`test-mask`: Keep only (the intersection's inside 255, the rest 0), Exclude (the overlap 0, the two lunes 255), the
order mattering (a Cut below an Add); `test-shapes` (new): Combine's four results on two overlapping rectangles —
the path's bounding box and `contains` at five points. `walk-combine.json`: two overlapping ellipses — each role in
turn with the dim read by the mask probe; Combine Unite → one shape in the list, its outline the union; Undo back to
two; Minus front; Intersect; Exclude; the Preview. `walk-combine-render.mjs`: avloop-cli frames of the four combined
projects, their mask values at the lunes and the overlap, GPU = CPU.
