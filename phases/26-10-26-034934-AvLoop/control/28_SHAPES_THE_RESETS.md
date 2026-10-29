# 28 Shapes — the resets (item 25, AV-028, late)

Canonical current execution state: `../AUTONOMOUS_STATE.md`

The owner's words of 2026-10-29 (`../README.md` §1, item 25, [6] and [7]): *"Clicking on the text label next to any
control should reset its value."* and *"Each group on the right-hand side should include a right-click context menu
with a 'Reset' option, which will reset the settings for that group."* D-025. Readings R-92 … R-93.

## 1. The label click

- `ui/widgets/ResetLabel`: a QLabel that takes a left click (the hand cursor, the tip "Click to reset"), emits
  `reset()`; the same quiet look. `Section::addRow` and `OptionsRow::addNumber`/`add` make their labels ResetLabels
  and connect them to the value widget's reset: a `StepSpinBox` to its **default** (a new `defaultValue` the panels
  set where the field is made — the value of a fresh project's struct), a combo box to its default item, a
  `ColourField` to its default colour, a check box to its default state, a `TimeEdit` to its own.
- Every control with a label next to it: the Crop, Loop, Output, Quality and Clip effects sections' rows; the options
  bars (Selection's X/Y/W/H/Scale, the crops', the shapes', the zoom's); the Preview's labelled fields; the
  Settings window's rows; the Loop Finder's. A pair of fields under one label (X / Y, W / H, Crossfade s / f) both
  reset. A label of a read-only row (Output size, the summary) does nothing and keeps the arrow cursor.
- The reset goes through the control's own change path: one Undo step, the same as typing the default (R-92). A
  shape's or a keyframe's field resets that shape's or keyframe's value.
- What "reset" means a field: the project's fresh value (Crop X/Y/W/H: unchanged — a position has no default; they
  stay as they are, the label says "No default" in its tip; Radius 0, Feather 0; Loop: Crossfade 0.5 s, Curve
  Smooth, Turn 0.3 s, Speed 1, Method Nearest, Mode Crossfade; Output: R-6's; Quality: Medium's; Effects: none;
  Selection: Scale 100 %, X/Y as Reset does, Flips off) (R-93).

## 2. The group's context menu

- Each `Section` of the side panel: a right click on its header or its body opens a menu with "Reset <title>"
  (Crop, Loop, Output, Quality, Clip effects — and the Shapes subgroup "Reset shapes" inside Crop's). The reset is
  the group's fresh state as one Undo step: Crop → no crop, no shapes, Inverse off ("Clear the crop"); Loop → the
  defaults (the range stays); Output → R-6's; Quality → Medium; Clip effects → the Reset button's; Shapes → the
  shapes removed, the frame kept.
- The options bars get the same menu ("Reset <tool>") for the tool's fields.
- The menu's items are named for walks (`menu.reset.<key>`); a walk opens it by the `menu` step or a right click.

## 3. Tests and walk

`test-commands`: each group's reset as one Undo step and its Undo restoring every value. `walk-resets.json`: a
value changed in every section (Feather 20, Crossfade 1.2 s, Long side 720, Custom quality, Exposure 2), each
label clicked and its value read back as the default, each an Undo step; the right-click menu on each group — Reset,
the probe summaries empty/default; the options bar's labels for the Selection's Scale and the crop's Radius; Undo
through them all back to the changed values.
