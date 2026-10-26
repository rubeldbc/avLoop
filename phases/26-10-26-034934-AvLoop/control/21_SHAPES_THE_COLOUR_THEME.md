# 21 Shapes — the colour theme (item 18, AV-020, late)

Canonical current execution state: `../AUTONOMOUS_STATE.md`

The owner's words of 2026-10-27 (`../README.md` §1, item 18). D-018. Readings R-71 … R-73.

## 1. The effects (`src/core/ClipEffects`, straight sRGB 0 … 1)

| Effect | Values | What it does |
| --- | --- | --- |
| Replace colour | From, To (#rrggbb), Hue range 0 … 180° (30), Softness 0 … 180° (20), Amount 0 … 100 % | HSV: d = the hue's distance from From's; weight = Amount × (1 − smoothstep(range, range + softness, d)) × smoothstep(0.08, 0.25, S); hue += weight × the From → To turn (shortest way), S × (1 + weight·(S_To/S_From − 1)), V likewise. First in the order, on the clip's own colours. |
| Colourise | Colour, Amount | every pixel HSL(the colour's hue and saturation, its own lightness), mixed by Amount |
| Split toning | Shadows, Highlights, Amount | + Amount × 0.6 × (tint_S·(1 − Y)² + tint_H·Y²), each tint the colour at HSL lightness ½ less grey |
| Duotone | Dark, Light, Amount | mixed toward Dark + (Light − Dark)·Y by Amount (Y the Rec. 709 luma) |

Colourise, Split toning and Duotone come after Saturation and Vibrance, before Sepia. An effect at 0 % keeps its
colours without counting: the frame passes untouched. All are per-pixel colour functions: they ride the canvas's
colour table and the CPU pass of D-017 unchanged — no new drawing path.

## 2. The panel and the eyedropper

- Four groups after Colour: Replace colour (From with the eyedropper, To, Hue range, Softness, Amount), Colourise
  (Colour, Amount), Split toning (Shadows, Highlights, Amount), Duotone (Dark, Light, Amount).
- `ui/widgets/ColourField`: the hex (typed, taken on Enter), a swatch (the colour dialog), the eyedropper; a colour
  chosen while its effect is at 0 % sets it to 100 %. One Undo step a colour.
- `Canvas::pickColour`: the cross cursor; the next left click on the video gives the clip's straight colour there
  (the frame as decoded, the flips folded in), the click not the tool's; Esc or a click beside the video gives none.

## 3. Walk

`walk-theme.json`: the eyedropper on the counter sample's yellow bar; To typed red — the yellow bar red, the others
as they were; Esc cancels a pick; Colourise 100 %; Duotone by its colours alone; Split toning; Undo; the Preview.
`walk-theme-render.mjs`: avloop-cli frames — yellow → red exact, the red and green bars byte for byte, Colourise,
Duotone's dark at black, an AV-019 file without the theme's keys, GPU = CPU.
