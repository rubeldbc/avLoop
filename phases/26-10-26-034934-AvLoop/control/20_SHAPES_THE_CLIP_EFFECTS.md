# 20 Shapes — the clip effects (item 17, AV-019, late)

Canonical current execution state: `../AUTONOMOUS_STATE.md`

The owner's words of 2026-10-27 (`../README.md` §1, item 17). D-017. Readings R-66 … R-70.

## 1. The effects (`src/core/ClipEffects`)

| Group | Value | Range | What it does (straight sRGB 0…1 unless said) |
| --- | --- | --- | --- |
| Light | Exposure | −4 … 4 EV | linear light × 2^EV |
| | Contrast | −100 … 100 | (v − ½)·k + ½; k = 1 + 2c (c ≥ 0), 1 + c (c < 0) |
| | Highlights, Shadows | −100 … 100 | v + ¼·a·6.75·v²(1−v), v + ¼·a·6.75·v(1−v)² |
| | Whites, Blacks | −100 … 100 | v + ¼·a·v², v + ¼·a·(1−v)² |
| Colour | Temperature, Tint | −100 … 100 | linear gains: R 1 + ¼t, B 1 − ¼t; G 1 − ⅕m |
| | Saturation, Vibrance | 0 … 200 %, −100 … 100 | luma + (v − luma)·s·(1 + vib·(1 − spread)) |
| | Hue | −180 … 180° | the W3C hue-rotate matrix |
| Creative | Sepia | 0 … 100 % | mixed toward the W3C sepia matrix |
| | Invert, Posterize | on/off, 2 … 32 | 1 − v; round(v·(n−1))/(n−1) |
| | Vignette, Vignette size | −100 … 100, 0 … 100 | weight smoothstep(0.7·size, +0.65, r) at the clip's (u, v); darker (a > 0) or toward white |
| | Opacity | 0 … 100 % | alpha × it |
| Detail | Sharpen | 0 … 100 | unsharp mask, σ 1 source px, amount × 2 |
| | Blur | 0 … 100 px | Gaussian of σ = radius / 2 (three box passes each way), edges held |

On off keeps the values and applies none. Neutral (off, or every value its default): the frame passes untouched.

## 2. Where they are made

- Export, Preview (Live and Encoded), batch, avloop-cli: `render/EffectsPass` on each source frame at the working
  size (blur and sharpen scaled to it), before the GPU or the CPU back end draws — the two get the same frame. The
  GPU's texture pool is keyed by the frame's pts and the look (`ClipEffects::key`), so a shared context never draws
  one look's texture for another (R-67).
- Canvas: its fragment shader — the colour from a 64³ table of `ClipEffects::colour` (rebuilt when the colour values
  change), posterize, the vignette and the opacity as written above, blur and sharpen as 7 × 7-tap Gaussians. A close
  look; the Preview shows the exact output. Without OpenGL 3.3, the QPainter fallback draws the pass's frame.
- Not made with them: the timeline's thumbnails and the Loop Finder (the source as it is, R-69).

## 3. The project

`"effects"` (every key above, `on`), version 2; a version-1 file opens without effects; 1.0.1 refuses a version-2
file with its message (R-68). One Undo step a field (typing or the wheel merges), On, Invert and Reset one each; a new
video starts with none; presets keep Loop, Output and Quality only. Alpha Auto: on when Opacity < 100 % (R-69).

## 4. The panel (`src/ui/panels/EffectsPanel.cpp`)

The fifth section, "Clip effects", after Quality, open by default: On and Reset; Light, Colour, Creative, Detail by
their names (R-70). Probes `effects.summary` (the values not none, by key) and `effects.on`.

## 5. Walk

`walk-effects.json`: none set; a look typed field by field (seven Undo steps) — the canvas, the section; Invert,
Posterize, Blur, Opacity over the checker; Undo; Off; the Preview; Reset and Undo. Five pictures and two of the
section. `walk-effects-render.mjs`: avloop-cli frames of the counter sample — none (a version-1 file), Invert, grey,
Opacity 50 %, Off byte for byte, Blur at a bar's edge, a whole look on the GPU and the CPU.
