# AvLoop — Software Specification

Oct 26, 2026 · @rubel

## 0. Delivered behaviour

Each phase writes what it delivered under its D-number here, newest first (spec §4; the programme `phases/26-10-26-034934-AvLoop`). Only delivered behaviour; the sections after this one are the owner's specification as written.

### D-027 — The timeline layers I: the layers and their composite (AV-030, late, 2026-10-30)

- **Video layers** (R-97 … R-103): the project's video stays the main one (track 1) with everything it had — its
  range, loop, transform and keyframes, effects, crop and shapes; up to 8 other videos sit on tracks 2 … 9 above it
  (`"layers"`, version 3): each its file, display size and length, its track, its Start on the main video's time, its
  own In/Out, its place and scale in scene space (as the Selection's transform, flips included), Opacity, shown,
  locked. A layer shows while Start ≤ t < Start + (Out − In), its own time In + (t − Start). A file without
  `"layers"` has none.
- **The composite** (AVADR-0001 kept): either back end draws the main video's frame as before — masked, with alpha —
  and the `LayerPass` lays each shown layer present at the frame's time (the loop's dominant source frame) over it in
  track order, bilinear by its own place, at its opacity, inside the crop's mask (acc = mask·L + acc·(1 − αL)); with
  Alpha Off the background goes under last. The same CPU pass after either back end: GPU = CPU (max diff 0 in the
  walk). One software decoder a layer, opened on first use (at most 8). The Preview, every export, the batch and
  avloop-cli carry the layers; hidden, absent or none: the main video's bytes.
- **The UI**: ☰ → Add as layer… (Ctrl+Shift+O) — a probed video on the lowest free track (no overlap on a track) from
  the playhead, at its own size, its top-left at the scene's (R-18), selected; one Undo step. The **Layers** section
  (after Crop): its rows top first ("name · V3", a check box to show or hide), and for the selected X, Y, Scale,
  Opacity, Start, Track, Locked, Delete — one Undo step each; a right click "Reset Layers" removes them all. The
  canvas draws each shown layer's frame at the playhead under the crop's dim (decoded when the playhead rests, at most
  960 px, the last frame kept while playing) with its outline — the selected one solid and named. Probes
  `layers.count`, `layers.selected`, `layers.summary`, `layers.previewDecoders`; the walk's `"addLayer"` step.
- Not done here (R-98, R-103): a layer has no keyframes, effects or shapes of its own; the Selection tool, the
  thumbnails and the Loop Finder work on the main video. The tracks on the timeline are AV-031's.

### D-026 — The align tools and the shape layers (AV-029, late, 2026-10-30)

- **Align** on the S bar (R-94, R-96): "Align to" Selection (the selected shapes' joined box) or Frame (the crop's, else
  the video's); Align left, centre, right, top, middle, bottom — two or more selected, or one to the frame — and
  Distribute horizontally, vertically — three or more: the first and last (by left or top) stay, or go to the frame's
  edges, those between placed with equal gaps. Shapes move by their bounding boxes, never resize; a locked shape stays;
  one Undo step a press. Buttons only (no keys). The 54th … 61st icons.
- **The shape layers** (R-95): the Crop section's list, top first, a layers panel — each row's check box shows or
  hides the shape (a hidden shape is out of the mask and the dim), a lock icon for a locked one; rows dragged reorder
  the shapes (the mask's order with them); for the one selected: Name, Visible, Locked, Up, Down, Duplicate ("<name>
  copy" above it, 10 px across). A locked shape is not picked, moved or Ctrl+A-selected on the canvas; its row still
  selects it. Each change one Undo step. No groups or sublayers. Probe `shapes.flags` ("vu" shown/unlocked a shape).
- The file: a shape's `"name"`, `"visible"`, `"locked"` (written since AV-024), version 3.

### D-025 — The resets (AV-028, late, 2026-10-29)

- **A label's click** resets the control beside it to its default (R-92): every row of the side panel's sections
  (`Section::addRow` makes its label a `ResetLabel`) and every labelled field of the options bars (`OptionsRow`) — the
  hand cursor and "Click to reset" where there is a default. A default is kept on the control where it is made
  (`reset::setDefault`) and the reset goes through the control's own change path — a number's `valueChanged`, a list's
  `activated`, a box's click, a colour's `colourChosen` — so it is one Undo step, as if typed. A pair of fields under
  one label resets those that have defaults. No default, no reset: a position (Crop X/Y/W/H, the Selection's X/Y/W/H,
  a shape's place), a read-only row (Output, the summary), the Output's background colour (a hex field).
- The defaults are a fresh project's (R-93): Crop — Shape Rectangle, Inverse off, Ratio Free, Radius 0, Feather 0;
  Shapes — Role Select, Feather 0, Strength 100 %, Combine Unite; Loop — R-28's (Crossfade, 0.5 s, Smooth, 0.3 s, 1×,
  Nearest); Output — R-6's; Quality — Medium's (CRF 31, Speed 2, no max bitrate, one pass, no target); Clip effects —
  none, On; the bars — the crop's Ratio, Radius, Feather; the Selection's Scale 100 %; the Shape bar's Draw Rectangle,
  Role Select, Feather 0, Strength 100 %.
- **A right click on a section** (its header or a free part of its body) offers "Reset Crop" (no crop, no shapes, not
  inverted), "Reset shapes" (inside Crop: the shapes removed, the frame kept), "Reset Loop" (the range stays), "Reset
  Output", "Reset Quality", "Reset Clip effects" — one Undo step each; an options bar offers "Reset <tool>" (a crop
  bar: its ratio, radius, feather and inverse; the Selection: its Reset button's; the Shape bar: the next shape's kind
  and role and the selected shapes' role, feather, strength). Menu items named `menu.reset.<key>`; the walk's
  `"context"` step opens a right-click menu and picks an item by its text. Labels named `<control>.label`.

### D-024 — The soft parts (AV-027, late, 2026-10-29)

- Every shape's **Feather** (0 … 200 output px, `"feather"`) and **Strength** (0 … 100 %, `"strength"`): its coverage
  is §6.1's ramp — clamp((½ − d) / max(f, 1)) of its signed distance d, softening inward — times the strength, then
  joined by its role (R-90). A Cut at 40 % leaves 60 % of what it covers; Keep only and Exclude scale the same way.
- The distance: the standard kinds' analytic one (the crop's, at the shape's own place); a polygon's, a freehand
  outline's, a Pen path's and a combined shape's by an exact Euclidean distance transform (Felzenszwalb–Huttenlocher)
  of its anti-aliased raster — two float grids of the output size while it runs (R-91). An edge on the frame's own edge
  has nothing outside it to measure: no ramp there. Feather 0, Strength 100: AV-024's bytes.
- The S bar and the Crop section: Feather and Strength of every selected shape (a value typed for several sets all; one
  Undo step, typing merging); the list's row adds " · 30 px · 40 %". The canvas dims a Cut below full strength by it
  (a hint — the feather is the Preview's, R-22). Probe `shapes.soft` ("f/s" a shape).

### D-023 — The shape combinations (AV-026, late, 2026-10-29)

- Live (R-88): a shape's Role is one of **Select** (`"add"`), **Cut** (`"cut"`), **Keep only** (`"keep"`: r·s — only
  where it and the shapes below meet) and **Exclude** (`"exclude"`: r + s − 2rs — where one is, not both), joined
  bottom to top; Select and Cut as AV-024 wrote them, byte for byte. The role lists (the S and P bars, the Crop section)
  give the four with their icons (`role-add`, `shape-cut`, `role-keep`, `role-exclude`; the 51st … 53rd icons). The
  canvas's dim follows the same four on the outlines.
- Baked: **Combine** — Unite, Minus front (the lowest less the rest), Intersect, Exclude (pairwise united less
  intersected) — by the S bar's Combine ▸ menu or the Crop section's Combine op and button, for two or more selected:
  one Path shape at the lowest one's place with its role and a name "Path N", one Undo step, the others gone; its
  anchors the Pen's to edit. Qt's path booleans return lines, not curves: each all-corner outline is simplified to
  ½ scene px (R-89). A result of several outlines (two lunes, a ring) is one shape: an anchor's eighth value `1` begins
  another outline; such a shape fills odd-even (holes clear), a single outline by winding.
- The selection: Shift+click adds, Ctrl+A selects every visible, unlocked shape, a Ctrl+drag on the canvas beside them
  a marquee selecting what it touches; the list's Shift/Ctrl rows.

### D-022 — The shapes (AV-024, late, 2026-10-29)

- **Shapes** inside the crop's frame (`"shapes"` under `crop`, version 3; at most 64): a rectangle, a rounded
  rectangle or an ellipse by their rectangle, a polygon or a freehand outline by their anchors, all in scene px, never
  keyframed. Each has a role — **Select** (`"add"`: keeps its inside) or **Cut** (`"cut"`: makes it clear) — and a name
  ("Rectangle 1"), and carries `visible`, `locked`, `feather`, `strength` for the phases after (R-82, R-83).
- The mask (R-84): each visible shape drawn anti-aliased at the output size, joined bottom to top into one running
  coverage — Select max(r, s), Cut r·(1 − s) — starting at nothing when a shape selects, at the whole frame when they
  only cut; the crop's own coverage (its shape, feather, inverse) multiplies it. No shapes: the crop's mask byte for
  byte. Shapes without a crop act on the whole video. Alpha Auto is on with a visible shape. Both back ends use it.
- **The Shape tool (S)**, after the crop tools on the rail (the 46th … 49th icons: the tool, polygon, freehand, cut):
  its bar's Draw (Rectangle, Rounded, Ellipse, Polygon, Freehand) and Role for the next shape; a drag draws a standard
  one (Shift square or circle, Alt from the centre; under 16 px cancelled); clicks draw a polygon, closed by a click on
  its first anchor, Enter or a double-click, Esc cancelling; a freehand drag is simplified to 1 scene px. A click
  selects the topmost visible, unlocked shape (Shift adds); a drag moves the selected; one standard shape has the
  crop's eight handles, a polygon its anchors (one dragged alone); Delete, arrows (Shift 10 px), Esc deselects. The bar
  types X, Y, W, H, Radius of one selected standard shape and deletes. No snapping for shapes (R-85). One Undo step an
  edit ("Change the crop"); the selection is not in Undo or the file.
- The canvas dims what the shapes and the crop do not keep (their outlines' path booleans; feather and strength are
  the Preview's, R-22) and outlines each shape — a selecting one solid, the others dashed. The Crop section lists the
  shapes top first ("Polygon 1 · Select"), its rows selecting them; Role and Delete act on the selected.
- Probes `shapes.count`, `shapes.selected`, `shapes.summary`, `panel.shapes.rows`, `shape.mode`, `shape.points`,
  `shape.freehand`. The walk's drag takes `"via"` points (a freehand outline).
- **The Pen (P; AV-025)**, after S on the rail (the 50th icon), as Illustrator's: a click adds a corner anchor, a
  click-and-drag a smooth one with mirrored handles; a click on the first anchor or Enter closes the path (a Path shape,
  `"kind": "path"`, its anchors `[x, y, in x, in y, out x, out y, smooth]`; one Undo step), Esc drops it. On the one
  selected path or polygon (a polygon edited becomes a path, R-86): a handle dragged turns a smooth anchor's other
  handle opposite at its own length, Alt breaks the pair; an anchor dragged moves with its handles; Alt+click converts
  smooth ↔ corner (a corner's new handles along its neighbours' line, a third of the way to each); a click on a
  segment adds an anchor by de Casteljau, the outline unchanged; Delete removes the selected anchor (the path when two
  are left); Ctrl+click selects the shape under it without drawing. The bar: Role and how the Pen is used. The mask
  draws a path's cubic segments; the canvas shows its anchors, handles and the next segment to the pointer. No
  pressure, width or strokes (not asked). Probes `pen.drawing`, `pen.anchor`, `pen.anchors` ("c,s,b" a kind an anchor).

### D-021 — The inverse selection (AV-023, late, 2026-10-29)

- The crop's **Inverse** (`"invert"` under `crop`): the export's frame is the whole video where it stands at In (the
  no-crop rule, R-8, R-49) and the crop's shape is a transparent hole at its place in it; the feather softens outward
  from the shape's edge, into what is kept (a 1 px anti-aliased edge at 0) (R-79). The crop's rectangle, ratio, radius,
  feather, tools, handles, snapping and keys are unchanged; only what is kept flips.
- Alpha Auto is on with an inverted crop; the status bar says "Transparent area in output" while the hole touches the
  video; the Crop section's and the Output section's size read the video's (scaled by Long side or Custom as before).
- The canvas dims the hole instead of the outside and outlines the export's frame thinly. The Crop section has an
  "Inverse" check box after Shape; each crop bar an Inverse toggle (the 45th icon, "invert"). One Undo step a change.
  Delete or Clear removes the crop and the inverse; a new crop drawn, or another crop tool, keeps it (R-81).
- The mask (`MaskBuilder::build(crop, frame, size)`): not inverted, byte for byte AV-004's; inverted, 0 inside, 255
  outside, clamp((d + ½) / max(feather, 1)) — cached by the crop, the frame and the size. Both back ends use it.
- **The project file's version 3** (R-80): `ProjectMigrate` 2 → 3; a version-2 file opens with `invert` false; AvLoop
  1.1.0 refuses a version-3 file with its "saved by a newer AvLoop" message. Items 20 … 27 add their keys under 3.

### D-020 — The clip as cut (AV-022, late, 2026-10-29)

- A fifth loop Mode, **None** (`"none"` under `loop`): the clip from In to Out as cut — no crossfade, no return, no
  seam treatment. L = D; output frame k is segment frame k, byte for byte Simple's (R-77). It is the first entry of the
  Mode list, "None (the clip as cut)"; Crossfade, Curve and Turn are hidden as for Simple; the summary reads
  `Clip 2.00 s · 60 frames @ 30 fps`. The timeline shows no band.
- The Preview: no seam mark at the slider's ends; Around seam disabled, its tip "The clip is not a loop" (the player
  still repeats, as a Media Source does). The export's and the batch's suggested name says `_clip_` where the template
  says `_loop_` (or ends in `_loop`); any other template is as typed. Formats, quality, alpha, avloop-cli unchanged;
  the Loop Finder unchanged.
- The file stays at version 2 on main until AV-023 makes it 3 (R-78): 1.1.0 would read `"none"` as Crossfade, so no
  Release carries this phase alone.

### D-019 — The release 1.1.0 (AV-021, late, 2026-10-27)

- Version 1.1.0, from the root `CMakeLists.txt` alone: avloop.exe's VERSIONINFO (1.1.0.0), the About box's "Version
  1.1.0", `avloop-cli version`, the installer `AvLoop-1.1.0-setup.exe` and Apps & features. A minor step: items
  16 … 18 since 1.0.1 and the project file's version 2, which 1.0.x refuses with its message (R-74).
- The GitHub Release `v1.1.0` ("AvLoop by Rubel 1.1.0", latest) with `AvLoop-1.1.0-setup.exe` and
  `AvLoop-1.1.0-setup.exe.sha256`; its notes say what changed since 1.0.1; the README's Download link goes to it.
  `v1.0.1` and `v1.0.0` stay. The AppId, folder, association and settings keys are unchanged: 1.1.0 installs over
  1.0.x and keeps its settings, presets and queue.

### D-018 — The colour theme (AV-020, late, 2026-10-27)

- Four effects join the Clip effects section, after Colour (R-71): **Replace colour** — From (typed, chosen, or picked
  from the clip by the eyedropper), To, Hue range (0 … 180°, 30), Softness (0 … 180°, 20), Amount: the hues near
  From's turned the From → To way, their saturation and value scaled by To's over From's, the others and greys
  untouched; **Colourise** — Colour, Amount: every pixel that hue, its lightness kept; **Split toning** — Shadows,
  Highlights, Amount; **Duotone** — Dark, Light, Amount. The maths: `control/21_SHAPES_THE_COLOUR_THEME.md` §1.
- Order: Replace first (on the clip's own colours, so a picked colour is found), Colourise, Split toning and Duotone
  after Saturation and Vibrance. They are colour functions: the canvas's table and the CPU pass carry them as D-017's.
- Each colour a hex field and a swatch (the colour dialog); a colour chosen while its effect is at 0 % sets it to
  100 %; an effect at 0 % keeps its colours and changes no byte. The eyedropper (the 44th icon): the next click on the
  video gives the clip's colour there, before effects; Esc or a click beside it gives none (R-72).
- The file stays at version 2 — `"replaceFrom"`, `"replaceTo"` (#rrggbb), `"replaceRange"`, `"replaceSoftness"`,
  `"replace"`, `"colouriseColour"`, `"colourise"`, `"splitShadows"`, `"splitHighlights"`, `"split"`,
  `"duotoneDark"`, `"duotoneLight"`, `"duotone"` (R-73).

### D-017 — The clip effects (AV-019, late, 2026-10-27)

- The side panel's fifth section, "Clip effects" (after Quality, open by default): On and Reset; Light — Exposure
  (−4 … 4 EV), Contrast, Highlights, Shadows, Whites, Blacks (−100 … 100); Colour — Temperature, Tint, Vibrance
  (−100 … 100), Saturation (0 … 200 %), Hue (−180 … 180°); Creative — Sepia (0 … 100 %), Invert, Posterize (Off,
  2 … 32 levels), Vignette (−100 … 100) and Vignette size (0 … 100), Opacity (0 … 100 %); Detail — Sharpen (0 … 100),
  Blur (0 … 100 px). The maths: `control/20_SHAPES_THE_CLIP_EFFECTS.md` §1 (R-66). §1.3's "colour correction" is
  built by this.
- Order: blur, sharpen (premultiplied), then each pixel straight — exposure and white balance in linear light,
  contrast, the tones, hue, saturation and vibrance, sepia, invert, posterize, the vignette (at the clip's own
  coordinates), the opacity. Not keyframed; On off keeps the values and applies none; neutral leaves every byte.
- The Preview (Live and Encoded), every export, the batch and avloop-cli: `EffectsPass` on each source frame at the
  working size before the GPU or the CPU back end — they agree by construction; the GPU's texture pool is keyed by the
  look too (R-67). The canvas: the same effects by its shader (a 64³ colour table, Gaussian taps), a close look; the
  QPainter fallback draws the pass's frame. The thumbnails and the Loop Finder read the source as it is (R-69).
- The project: `"effects"` (every value's key, `on`), version 2; a version-1 file opens without effects; 1.0.1
  refuses a version-2 file with its message (R-68). One Undo step a field ("Change the clip effects"), typing or the
  wheel in one field merging; a new video starts with none; presets do not carry them. Alpha Auto is on when Opacity
  is under 100 % (R-69).
- Mended on the way: quitting with the Preview open crashed on the way out (the Preview outlived the render service
  it used; the main window now closes it first); AV-010's recovery walk read the window title of before AV-017.
- Probes `effects.summary`, `effects.on`. No new version (R-70).

### D-016 — The wheel and the keyframe copy (AV-018, late, 2026-10-27)

- The canvas's wheel, with Ctrl or without, zooms about the pointer: the scene point under it stays under it. One
  notch (120 eighths of a degree) is one step of §6.3's list (10 … 800 %); a finer wheel or a touchpad adds up to a
  notch, a turn the other way starting the count again. Shift+wheel and a sideways wheel pan across, as before;
  panning up and down is by a middle-drag or the Hand — the wheel no longer pans up/down (D-003's "the wheel pans
  up/down" and §6's "wheel = vertical pan" give way, R-63). The timeline's wheel is unchanged.
- On the timeline's keyframe row an Alt+drag of a diamond copies the keyframe — position, scale, interpolation — to
  the output grid frame it is dropped on; the original stays; refused where another keyframe already is. One Undo step,
  "Copy a keyframe" (MoveKeyframe's id, the copy's moves merged into it). Alt+click alone copies nothing. The copy
  cursor shows with Alt over a diamond (R-64).
- Mended: a keyframe drag refused over another keyframe keeps the one it drags (it used to pick the other one up).
- The walk tool's check that a walk left no AvLoop running counts only processes it did not find running before
  (the owner's own AvLoop may be open). No new version (R-65).

### D-015 — The name and the About box, v1.0.1 (AV-017, late, 2026-10-26)

- The name shown is "AvLoop by Rubel" (`AppInfo::displayName()`): the window's title ("AvLoop by Rubel — <file>"),
  the top bar, the ☰ menu's "About AvLoop by Rubel", the About box, the questions' window titles; avloop.exe's
  VERSIONINFO (ProductName, FileDescription; CompanyName and LegalCopyright Md. Kamrul Islam Rubel); the installer's
  AppName, publisher, copyright, Start menu and desktop shortcuts and Apps & features entry. An install over 1.0.0
  deletes 1.0.0's `AvLoop.lnk` shortcuts.
- What keeps "AvLoop": `AppInfo::name()` and `organization()` (QSettings, `%APPDATA%\AvLoop`, `%LOCALAPPDATA%\AvLoop`,
  the disk cache), the install folder, the AppId, `.avloop` → `AvLoop.Project`, the executables' names (R-59).
- The About box: the logo, the name, "Version 1.0.1", the purpose, the MIT licence and the repository (a link);
  *Developer & contact* — the developer, Bangladesh, the email (`mailto:`), the developer profile and WhatsApp
  (`wa.me`) as links in the theme's accent, opening in the browser or mail program; *Built with*, as before; the footer
  "© <year> Md. Kamrul Islam Rubel · Designed and built in Bangladesh" (R-60). The walks read it by the `about.text`
  probe.
- `tools/check-secrets.mjs` allows the owner's address in `src/ui/dialogs/AboutDialog.cpp` alone (R-61).
- Version 1.0.1; the GitHub Release `v1.0.1` carries `AvLoop-1.0.1-setup.exe` and its SHA-256 (R-62).
- The walk's `menu` step looks again for its menu or submenu for two seconds before failing (a busy desktop opens it
  late); the shell walk waits up to 3 s for the window to be maximized.

### CP-B1 — The whole walked, v1.0.0 (AV-016, 2026-10-26)

- Version 1.0.0 (the root CMakeLists.txt; the About box, avloop.exe's VERSIONINFO, the installer).
- Items 1–14 checked on one fresh Release build installed by its own installer: every unit test, the same under
  AddressSanitizer and in Debug with the leak gate; every phase's walk run against the installed app
  (`AVLOOP_BIN_DIR`), the GPU-off set, the 10-minute memory walk, the install walk (S15), the README's pictures taken
  from the installed app.
- Correction it delivered: the memory log can sample faster (`avloop.exe --mem-log <file> --mem-log-ms N`, at least
  100 ms; a second by default), and the memory walk compares export peaks at 200 ms — a sample a second missed an
  export's brief GPU peak.
- The GitHub Release `v1.0.0` carries `AvLoop-1.0.0-setup.exe` and its SHA-256 (reading R-5).

### D-014 — The installer (AV-014, 2026-10-26)

- `scripts\deploy.ps1` (or `cmake --build build --target deploy`): a Release build → `deploy\` with avloop.exe,
  avloop-cli.exe, windeployqt's Qt (no translations, no software OpenGL, no network: the TUIO touch, TLS and
  network-information plugins left out), the FFmpeg DLLs, the VC++ runtime app-local, and `licenses\` (LICENSE,
  LICENSE-THIRD-PARTY.md, LGPL-3.0, GPL-3.0); 37 files, 141 MB.
- `installer\AvLoop.iss` (Inno Setup 6, not signed): `ISCC installer\AvLoop.iss` → `installer\Output\AvLoop-<version>-
  setup.exe` (45 MB, 31 s). Per user by default, every user by choice; the licence and the notices shown; Start menu
  entry, optional desktop icon; `.avloop` → `AvLoop.Project` (`"avloop.exe" "%1"`, the app's icon); uninstalling
  removes the program, the association, `%LOCALAPPDATA%\AvLoop` and `%LOCALAPPDATA%\cache\AvLoop`, keeps
  `%APPDATA%\AvLoop`. Readings R-56, R-57, R-58.
- The version: the root CMakeLists.txt → AvLoopVersion.h → the About box and avloop.exe's VERSIONINFO, which the
  installer reads (`/DAppVersion=` overrides).
- Walked: a silent per-user install into a temporary folder; the installed app draws with only Windows on PATH; the
  association's command from the registry opens a project; the silent uninstall leaves only the settings.

### D-013 — The batch and the other formats (AV-013, 2026-10-26)

- Output → Format offers WebM VP9, WebM VP8, GIF, APNG, MP4 H.264 and MOV ProRes 4444; one this FFmpeg cannot write is
  disabled with the reason. GIF: `palettegen` (stats diff, 255 colours and a transparent one) over every frame in a
  first pass, then `paletteuse` (Bayer, scale 3) with alpha cut at 128, `-loop 0`, at most 50 fps (the Output
  section says so); APNG in RGBA, `-plays 0`; MP4 by `h264_mf` (Media Foundation; OpenH264 if it cannot open),
  yuv420p BT.709, one GOP, no B-frames, the preset's bitrate, the Background colour behind, `+faststart`; ProRes by
  `prores_ks` profile 4444 in yuva444p10le. Quality applies to WebM, and to MP4 by its bitrate; the Quality section
  says that ProRes's profile sets its quality and that GIF and APNG have none. The check after writing reads each codec (GIF's rate is not read back; alpha by
  the decoded frame, WebM by its alpha_mode too). Readings R-53, R-54.
- The batch queue (`export/ExportQueue`): entries = a project snapshot + the file; Start runs them one at a time on the
  window's render and export threads (Export waits meanwhile; the queue does not start while Export runs); a failure
  keeps its reason and the next one runs; Pause stops the entry under way, which is Pending again; `queue.json`
  beside the settings, locked, saved at every change; after a restart, unfinished entries are Pending. Ctrl+Shift+E
  (Menu → Add to Batch Queue, the window's Add current) adds the project as it is, written where Export would suggest
  (the last export's name, else the template in the export folder; the format's extension), never over a file already
  there. Menu → Batch Queue opens the window: name, format, size, state, progress;
  drag to reorder; Add current, Remove, Clear done, Pause, Start. Readings R-51, R-52, R-55.
- `avloop-cli export … --format webm-vp9|webm-vp8|gif|apng|mp4-h264|mov-prores`; its PASS line names the encoder.
- The bundled FFmpeg 7.1 (LGPL) has `h264_mf`, `libopenh264`, `gif`, `apng`, `prores_ks` and the palette
  filters: every format is available on this build.
- Measured (400×280, 3 s at 30 fps, an ellipse): GIF 1.24 MB in 1.1 s (two passes), APNG 2.9 MB in 1.7 s, MP4 0.41 MB
  in 0.5 s (h264_mf), ProRes 9.0 MB in 1.6 s; a batch of three (ProRes, VP9, GIF at 640×360) in 18 s.

### D-012 — The Loop Finder (AV-012, 2026-10-26)

- Menu → Loop Finder and Ctrl+L (with a video open) open its window: Search In–Out ± 5 s or the whole file, Length
  2–10 s (0.5–60), Find with a progress bar and Cancel, then the best five — "In 00:00:00.01 · Out 00:00:04.01 ·
  4.00 s · score 100" with the loop's first and last frames side by side. A click sets In and Out (one Undo step); a
  double-click also opens the Preview. A new video closes the window.
- `export/LoopFinder`: every frame of the range on the output grid, decoded on a software decoder of its own (at most
  640 px), reduced to a 64×64 greyscale fingerprint of the export's frame — the crop, the transform and keyframes at
  that time (4 KB a frame); every pair (a, b) with b − a within the lengths scored by the mean absolute difference of
  frames a and b plus that of their motions, rows in parallel (QtConcurrent) between progress reports; the best five
  with none within a second of a better one at both ends; 0–100 by the 95th percentile, ties to the shorter loop
  (reading R-50). The fingerprints live only in the search: closing the window (or a new search) lets them go.
- `avloop-cli find <video|project> [--from] [--to] [--min] [--max]`; `tools/media/make-samples.mjs` makes `loop4s.mp4`
  (20 s repeating exactly every 4 s, lossless H.264).
- Measured: on `loop4s.mp4` the best loop is 4.000 s (raw 0, score 100) over the whole file and over a project's crop;
  the search of 20 s at 640×360 takes 0.3 s; four searches in a row hold the memory (peaks 262–269 MB).

### D-011 — The keyframes (AV-011, 2026-10-26)

- `core/Keyframes`: a keyframe is a source time, a centre, a scale and Linear or Ease; at most one an output frame.
  The value at t: none — the one transform; before the first — the first; after the last — the last; between two —
  their mix at u, eased by u²(3 − 2u) when the earlier one is Ease. Keyframes outside In–Out count (§12.2). Flips are
  not keyframed: they hold (reading R-47).
- K adds a keyframe at the playhead with the transform shown, or updates the one there. With keyframes the Selection
  (and the options bar's X, Y, Scale) writes into the keyframe at the playhead, making one there if none; a flip changes
  the one transform. AddKeyframe, MoveKeyframe (one step a drag), DeleteKeyframe; deleting the last leaves the video
  where it was shown. The file keeps `transform.keyframes` as §11.1 shows.
- The Keyframe tool's options: Add, Delete, ◀ / ▶ (as `[` / `]`), Interpolation (Linear / Ease) and "Keyframe 2 of 3
  at 00:00:04.00". The timeline's keyframe row: diamonds — the one at the playhead solid orange (it is the one acted on,
  R-48), the others hollow, Ease ones with rounded corners; a click takes the playhead to it, a drag moves it on the
  output grid (refused onto another), Delete removes it; the playhead snaps to them within 8 px.
- The canvas shows the video where it stands at the playhead; the FrameRenderer gives every source frame drawn the
  transform at its own time (Preview, export, avloop-cli). Without a crop the frame is the video's rectangle at In;
  Alpha Auto and the working size look at every keyframe (R-49).
- Measured: a white clip moved 200 → 1000 px between 1 s and 3 s renders at 600.00 halfway (Linear), 325.00 a quarter of
  the way (Ease), 200 and 1000 before and after — on the GPU and the CPU alike.

### CP-A1 — Track A walked whole (AV-015, 2026-10-26)

- Items 1–10 checked on one fresh Release build: every unit test, the same under AddressSanitizer and in Debug (no
  debug-heap block left by repeated cycles — reading R-45), every phase's walk, the 10-minute memory walk (§3.7.8), the
  rendering walks again with the GPU off (`AVLOOP_GPU_RENDERING=off`), the README's pictures retaken.
- Corrections it delivered: a finished export, size estimate or Preview frees its GPU textures, mask, target and
  buffers (`RenderService::trim()`; the shared context stays); the render service ends its GL objects and context on
  the render thread and the offscreen surface on the GUI thread that made it.

### D-010 — The project and the settings (AV-010, 2026-10-26)

- The ☰ menu's New, Open Project…, Save (Ctrl+S), Save As… (Ctrl+Shift+S) and Recent (the last 10 projects, then the
  last 10 videos, Clear the list) work; Ctrl+O, drag and drop and `avloop.exe <file>` open a video or an `.avloop`.
  Saving is atomic (`QSaveFile`). The title is `AvLoop — <name>.avloop` (a video not saved as a project: its file name —
  reading R-41) with `*` while there are unsaved changes (the Undo stack away from its saved point, or a recovered
  autosave). Opening, New, Recent and closing the window ask Save / Don't save / Cancel when there are changes.
- Finding the source: its path, then its path relative to the project, then *Locate source…* (a file dialog; the log
  says it was needed); a video whose size or date changed since the save opens with a warning toast (R-43).
- `core/ProjectMigrate`: older versions brought up step by step; a newer file is refused ("This project was saved by a
  newer AvLoop (version N); update AvLoop to open it."); version 1 is the first format (R-39). Unknown keys are kept.
- Autosave every N minutes (2 by default) while there are changes, to `%LOCALAPPDATA%\AvLoop\autosave\session-<pid>
  .avloop` (with the PID and the project's own path); a clean exit deletes it; at the next start an autosave whose
  process is gone is offered: "Recover unsaved project? (autosaved at HH:MM)" — Recover (the work back, still unsaved,
  saving to its own file) or Discard.
- Settings (Ctrl+,; `%APPDATA%\AvLoop\AvLoop.ini`), a window of three tabs whose changes are saved at once (R-44):
  General (Theme — applied at once —, open the last project at start, autosave minutes), Performance (Hardware decoding
  and GPU rendering Auto/Off — from the next video —, RAM cache 256–8192 MB — at once —, the disk cache's folder and GB
  — from the next start —, Clear cache), Export (default folder, name template `{source}_loop_{w}x{h}`, the OBS tips).
  The keys, frozen from here on (§2.7): `ui/theme`, `general/openLastProject`, `general/autosaveMinutes`,
  `performance/hardwareDecoding`, `performance/gpuRendering`, `performance/ramCacheMB`, `performance/diskCacheFolder`,
  `performance/diskCacheGB`, `export/defaultFolder`, `export/nameTemplate`, `export/showObsTips`, `export/lastFolder`,
  `recent/projects`, `recent/videos` (and the window's `ui/…` and `preview/geometry`).
- Presets (`core/Presets`): Loop + Output + Quality under a name in `%APPDATA%\AvLoop\presets\<name>.json`; built in:
  "OBS Overlay 1:1 512", "OBS Lower Third 16:9 1280", "OBS Vertical 9:16 1080" (long sides 512 / 1280 / 1080 —
  R-40). The row above the side panel: choosing one applies it as one Undo step and the box shows the preset the
  settings match; ✓ saves under a name, ✕ deletes a user preset.
- Logs: `%LOCALAPPDATA%\AvLoop\logs\avloop-YYYYMMDD.log`, lines `HH:MM:SS.mmm [module] text`, a part over 10 MB goes on
  in `-2`, 7 days kept; an unhandled exception writes `avloop-YYYYMMDD-HHMMSS.dmp` there (`MiniDumpWriteDump`).
- About AvLoop: the version, the licence, the libraries and their licences (§3.8). F1 shows the shortcuts — the
  actions' own, the tools', the transport's — over the window; F1, Esc or a click closes it.
- Measured: save → open → save writes the same bytes and `avloop-cli frame` of both is identical (max difference 0); a
  moved folder opens by relativePath; a crash leaves a minidump and the autosave, killed or crashed the next start
  recovers it.

### D-009 — The export (AV-009, 2026-10-26)

- Export and Ctrl+E open the Save dialog (each time — reading R-34), filled with the project's last export, else
  `<source>_loop_<W>x<H>.webm` in the last folder. The export runs in the background from a snapshot of the project:
  its frames made on the render service's thread (the shared GPU context, two a turn so the Preview's work goes
  between them, at most six ahead), encoded on an export thread made at the first export or estimate, written to
  `<name>.part`, then `MoveFileEx` with REPLACE_EXISTING over `<name>`; a file still locked (OBS playing it) gets five
  more tries a second apart, then the loop is saved as `name (1).webm` with a warning toast (R-36).
- Cancel, an error or a full disk delete the `.part`; a full disk says how much the export needs and how much is free;
  an allocation failure stops the export with a message; a GPU that fails leaves the CPU to make the rest.
- The status bar: `Exporting 86/149 · 58% · 41 fps · ETA 0:02` (with "pass 1/2" for two passes), a thin bar and Cancel.
  Export stands disabled with the reason ("Open a video to export it"; a loop under 2 frames), as Preview does.
  Closing the window during an export asks "Export in progress — cancel and quit?"; a crop wholly outside the video
  asks before exporting.
- The written file is reopened and checked (`export/OutputVerify`): frame count = L, rate, W×H, codec, alpha (the
  stream's `alpha_mode` and the first frame through libvpx), the first frame a keyframe, no audio; a mismatch is an
  error toast and a log line. Success: `Exported 640×360 · 2.50 s · 0.28 MB` with Show in folder; the first export of a
  session adds the OBS tip (Media Source → Loop on · hardware decoding off) with Don't show again.
- The Output section: Format (WebM VP9, WebM VP8; the others at AV-013), Size (Crop size, Long side 1920 / 1280 /
  1080 / 720 / 512 / 256, Custom W×H with a ratio lock — even, ≥ 16, the long side ≤ 1920 with a note when that limit
  took it down), Alpha (Auto — only with transparent parts —, On, Off), Background (#rrggbb, behind the transparent
  parts when there is no alpha), FPS. The Crop section's Output row shows the result. `.avloop`: `output.size`, `w`,
  `h`, `lock`, `alpha`, `background`; `export.lastPath` (R-38).
- The Quality section: Low / Medium / High show their CRF and Speed; typing either makes it Custom from those values;
  Custom adds Max bitrate (VP8's bitrate), Two-pass and Target size (MB, R-37): two passes at
  target × 8 / duration × 0.9, one more at the corrected bitrate when the size lands more than 5 % off. The estimate
  (`~0.29 MB`): three parts of ten frames spread over the loop, encoded alone and extrapolated, 1 s after a change
  (R-35).
- `avloop-cli export <project.avloop> -o out.webm [--cpu] [--repeat N]`: the same job from a project file.
- Measured: every loop mode's frame count = L and its seam within 1.3 × the median neighbour difference; 12–60 fps by
  all three methods CFR with the right `r_frame_rate`; each file read twice in a row gives 2L frames; Target size
  within 2 %; three exports in a row hold the memory flat (Private Bytes peaks 712 / 722 / 723 MB).

### D-008 — The Preview window (AV-008, 2026-10-26)

- The Preview button or F5 opens one non-modal window (again: to the front) that shows only the exported part as it
  will be exported — the FrameRenderer's frames, the same as `avloop-cli frame` byte for byte. Its size and place are
  remembered; it starts at the output's size with its controls, at most 80% of the screen.
- Top: Live | Encoded, Background (Checker, Black, White, Green #00FF00, Navy, Image…), Zoom (Fit, 50%, 100% —
  pixel-exact —, 200%), and what it is doing ("Caching…", "Encoding n/L", "Out of date", "Encoded · 0.28 MB").
  Bottom: Play/Pause (Space), the slider over frames 0 … L − 1 (←/→ a frame) with the seam at its ends, Crossfade's
  blended frames as an orange strip, the Ping-pongs' turn points and a thin bar of the frames already made; the time
  "1.43 / 2.50 s"; Around seam (the last 2 s and the first 2 s, again and again). The info row: "640×360 · 30 fps ·
  75 frames · 2.50 s · VP9 alpha", with the encoded size once there is one.
- Live: frames made on the render service's thread into the Preview's own cache (a third of the frame cache's budget,
  at most 512 MB — reading R-31); playback always loops and waits for a frame not yet made ("Caching…") instead of
  dropping it. Any change in the main window reaches it 250 ms later.
- Encoded: the loop encoded with the current Output and Quality into a temporary WebM in the session's folder (progress
  shown), then that file played back through libvpx — alpha and compression as OBS shows them; a change marks it "Out
  of date" (it is not encoded again by itself); the file goes when the window closes.
- The encoder (`media/Encoder`, `EncoderOptions`, `Muxer`; shared with the export, reading R-14): premultiplied
  frames made straight, `yuva420p` (`yuv420p` without alpha), libvpx-vp9 (crf, `b:v 0`, deadline good, cpu-used,
  row-mt, tile columns log2(W/256), auto-alt-ref, lag-in-frames 25) or libvpx for VP8 (crf within the preset's bitrate,
  auto-alt-ref 0); BT.709 for VP9, BT.601 for VP8 (reading R-32), limited range, tagged; pts = k in 1/fps, the first
  frame a keyframe, the loop one GOP, no audio; the WebM marked `alpha_mode`. The Quality presets Low / Medium / High
  / Custom (§10.3) and the format (VP9 / VP8) are in the project (`quality`, `output.format`); their panels are
  AV-009's.
- The render service (one thread and one GPU context for the app's life — reading R-33) makes the Preview's frames;
  closing the Preview releases its player, its caches and its file; six opens and closes in a row hold Private Bytes
  within 3 MB.
- The editor's playback keeps the loop's first six frames, so each lap starts without waiting for a seek to In.

### D-007 — The renderer (AV-007, 2026-10-26)

- One `render::FrameRenderer` makes output frame k from a `RenderJob` snapshot (source, range, transform, crop,
  loop, output rate; the output size, the loop plan and the working size worked out once): LoopMap → TimeMap → the
  normalized source frames (two decoders, one for each stream of a blend; a variable rate by its packets' times;
  Motion's frames from the pre-pass) → one pass over the output → premultiplied RGBA8. AVADR-0001 is accepted.
- The pass, on the GPU and on the CPU alike: each output pixel centre maps to the video's texture coordinates by one
  affine (flips folded in), bilinear with texel centres at +½ and transparent outside, up to four layers summed by
  weight (the loop's blend × the FPS method's), times the crop's mask (built once on the CPU), over the background
  when the output has no alpha (Alpha Auto until AV-009), rounded once. Blending in premultiplied sRGB (reading R-7).
- The GPU back end: its own OpenGL 3.3 core context on an offscreen surface, an RGBA8 FBO, textures from a pool
  (the last two frames' layers, never over 512 MB or a quarter of the adapter's memory), the readback through two
  PBOs taking turns. Without OpenGL 3.3, with "GPU rendering: Off" (`performance/gpuRendering`) or after a GPU
  failure that one retry does not mend, the CPU back end renders (rows in parallel) and says why.
- GPU and CPU agree within 2/255 on the unit cases and within a mean of 1.0, a maximum of 4 on whole projects
  (measured: mean ≤ 0.075, max 1).
- Opening a video releases everything of the last one and starts no thread (spec §3.7.5): one D3D11 device serves every
  hardware decoder, filter graphs run on one thread, the decode and thumbnail workers run on threads that live with
  the window, the thumbnail decoder is single-threaded. Hardware- and software-decoded frames are identical again
  (NV12 made planar before the conversion).
- `avloop-cli frame <project.avloop> --at N -o x.png [--raw x.rgba] [--cpu]` renders output frame N;
  `avloop-cli frames <project.avloop> [--count N] [--cpu] [--mem-log x.csv]` renders N frames round the loop.

### D-006 — The loop engine (AV-006, 2026-10-26)

- The loop's maths are pure functions in `core`: `LoopMap` gives D = round((Out − In) × fps / Speed) (at least 2, 3
  for the Ping-pongs), L for each mode (Simple D, Crossfade D − F, Ping-pong and Ping-pong + Smooth turn 2D − 2) and
  for output frame k the one or two segment frames with their weight — §8.2's formulas, F clamped to 1 … ⌊D/2⌋, T to
  1 … ⌊(D − 1)/2⌋, the Smooth curve x²(3 − 2x), the turn's trapezoid velocity. `TimeMap` gives τ(i) and the source
  frames of segment frame i: Nearest (a tie to the earlier), Frame Blend (by time), Motion (the pre-pass's frame); a
  variable rate always by its frame times.
- `.avloop` keeps `loop` (mode, fade, curve, turn, speed, fpsMethod) and `output.fps` ("source" or "30000/1001").
  The default loop is Crossfade, F 0.5 s, Smooth, T 0.3 s, Speed 1, Nearest (reading R-28).
- The Loop section: Mode, Crossfade in seconds and frames (linked), Curve, Turn, Speed (0.10–4.00×, steps of 0.05),
  FPS method; "Motion Interpolation gives smoother slow motion" under Speed < 1 with Nearest; the summary "Loop 4.50 s
  · 135 frames @ 30 fps"; a value the loop clamps flashes yellow for 600 ms. Rows a mode does not use are hidden.
- The Output section's FPS: Source (shown with its value; a variable rate takes the nearest common one; at most 60),
  15, 23.976, 24, 25, 29.97, 30, 50, 59.94, 60, Custom (1–60, three decimals). A new rate keeps In and Out in seconds,
  moved to the nearest frame of its grid, in the same Undo step (reading R-29); F and T are kept in seconds.
- The timeline's crossfade band: a gradient over the F frames before Out and a dashed outline over the F after In;
  dragging the gradient's left edge sets F (one Undo step). Ping-pong + Smooth turn fills both ends' T frames quietly.
- The editor's playback runs at Speed.
- `media/Interpolator`: the Motion Interpolation pre-pass — the segment's normalized frames through `minterpolate`
  (mi_mode=mci, mc_mode=aobmc, vsbmc=1) at the output rate, Speed by stretched timestamps, alpha on a branch of its
  own; frames into the disk cache under a key of every input; progress and cancel. It runs when the Preview or the
  export needs its frames (AV-008, AV-009 — reading R-30).

### D-005 — The timeline (AV-005, 2026-10-26)

- Time is kept on the output frame grid (frame n at n / fps from the source's start); until AV-006's setting the
  output rate is the source's, at most 60 (reading R-6). `HH:MM:SS.FF` shows the whole seconds of a frame's time and
  its frame within that second; a time field takes `12.4` (seconds), `00:00:12.12` or `372f`.
- The range covers the frames from In up to Out (§8.1: D = (Out − In) × fps): I takes the frame under the playhead as
  the first, O as the last; In at or after Out moves Out to the file's end, Out at or before In moves In to the start;
  at least two output frames; Alt+I / Alt+O clear (reading R-24). The In and Out tools set them where the timeline is
  clicked; their options show the time (typed and Enter sets it) and, for Out, the length. `.avloop` keeps
  `range`: in, out (none read = the whole file). Every change is one Undo step; a handle's drag is one.
- The timeline: the ruler (timecode or frame numbers, double-click to switch; the step between labels from 1 frame up
  to 10 minutes and more, always wide enough for its label), the thumbnail strip (a software decoder of its own,
  keyframe seeks, at most 400 samples of 160 × 90, the visible ones first; `panel2` until made), the In–Out overlay
  (`accentSoft` inside, 50% black outside, the 3 px orange bar to drag the range, bracket handles), the empty
  crossfade band and keyframe row, the playhead over all. A click or drag on the ruler or the strip scrubs: the
  keyframe preview while moving, the exact frame after 150 ms at rest or on release (reading R-27); the playhead snaps
  to In and Out within 8 px; Ctrl+wheel zooms about the pointer from the whole file to 12 px a frame; the wheel and
  Shift+wheel scroll;  fits the file, Shift+ the range.
- The transport row: Go to In, Prev, Play/Pause, Next, Go to Out, Loop; the time (type to jump), the total; the In,
  Out and length readout. Keys: Space, ←/→ (1 frame), Shift+←/→ (10), Home/End (In / the range's last frame),
  Ctrl+Home/End, L, I, O, Alt+I, Alt+O. With a crop tool and a crop, the arrows on the canvas move the crop.
- Playback shows the source: decoding runs ahead into a ring of six frames; a 4 ms clock takes them in order and
  never waits; loop on cycles In → Out; a frame more than two frames late with a newer one due is dropped and counted
  in the status bar's "Dropped: N" (reading R-25). During playback the timeline and the time field follow at most 30
  times a second (reading R-26). 1080p H.264 plays at 30 and 60 fps with none dropped; memory stays flat over a
  minute of loop playback.

### D-004 — The crop tools (AV-004, 2026-10-26)

- One crop at a time (reading R-8): a Rect (M), Rounded (U) or Ellipse (E) in scene px with its ratio, Radius and
  Feather, kept in the project and the `.avloop` file (`crop`: kind, x, y, w, h, ratio, radius, feather). With no crop
  the export is the whole video.
- On the canvas: with no crop a drag draws one; with a crop a drag inside moves it, the eight handles resize it, a
  drag outside draws a new one in its place — each one Undo step. Shift keeps the ratio (1:1 when Free, a circle for
  the Ellipse), Alt draws and resizes from the centre, Ctrl turns snapping off; snapping brings a drawing's corners, a
  moved crop's edges or centre and a resized crop's moving edges to the video's edges and centre within 8 screen px
  (reading R-23). Arrows move it 1 px, Shift+arrows 10 px (presses in a row are one step); Esc cancels a drawing;
  Delete removes the crop; a double-click makes it the whole video; a drawing under 16 px is cancelled. Switching crop
  tools changes the kind and keeps the rectangle.
- The overlay: 60% black outside the crop, a 1.5 px orange border (rounded or elliptic), a dashed bounding box for the
  Rounded rect and the Ellipse, eight 8×8 white handles with orange borders, and while dragging the rule-of-thirds
  guides, the snap lines and the label `W × H → output W × H`. The canvas shows the crop by this outline; the
  Feather's softness is seen in the Preview and the export (reading R-22).
- Ratios: Free, Original, 1:1, 4:3, 3:4, 16:9, 9:16, 21:9, 3:2, 2:3, 4:5 and Custom… (two W:H fields; it starts from
  the crop's own proportions in lowest terms — reading R-21). A new ratio keeps the crop's centre and area; W or H
  typed with a ratio set moves the other side, the top-left staying. Rect: Swap W/H (the ratio swapped too) and Clear.
- The options bar: Ratio, X, Y, W, H and Swap W/H, Clear (Rect) / Radius (Rounded) / Feather (Rounded, Ellipse). The
  side panel's Crop section: Shape, Ratio, X / Y, W / H, Radius, Feather and the output's size with "· alpha" when the
  export needs alpha (a Rounded rect or Ellipse, a Feather, a crop past the video, or a source with alpha). The status
  bar says "Transparent area in output" for a crop partly outside the video, "Output is fully transparent" for one
  wholly outside. Ctrl+0 fits the crop.
- The mask (`render/MaskBuilder`): built on the CPU at the output size from the shape's signed distance — a 1 px
  anti-aliased edge at Feather 0, Feather 0–200 output px softening inward, Radius up to half the short side — and
  kept until the shape, the size, the radius or the feather changes.

### D-003 — The canvas and the Selection (AV-003, 2026-10-26)

- The canvas is an OpenGL 3.3 core widget over an unbounded scene in pixels: the video drawn by the transform shader
  (`render/Shaders`, shared with the renderer) as a mipmapped premultiplied texture, a checkerboard (16 px, two dark
  greys) where it is transparent, the scene around it `bg`; tool overlays painted by QPainter. Without OpenGL 3.3 the
  same picture is painted by QPainter.
- On load the video stands at 100% with its top-left at (0,0) (reading R-18) and the view fits it; the fit holds
  through resizes until the user zooms or pans.
- Zoom (Z): a click zooms in a step about the point (10, 25, 33, 50, 67, 100, 150, 200, 300, 400, 800%), Alt+click
  out, a drag zooms to the rectangle; options Fit, 100%, 200% and the zoom %. In every tool: Ctrl+wheel zooms about the
  cursor, the wheel pans up/down, Shift+wheel across, a middle-drag pans; Ctrl+0 Fit, Ctrl+1 100%, Ctrl+= / Ctrl+- zoom.
  Hand (H): a drag pans. The view never enters Undo.
- Selection (V): a drag inside the video moves it; a corner handle scales it keeping its ratio about the opposite
  corner, Alt about the centre (5–1000%). The video's border is a blue-white dashed line with four white handles,
  shown while the Selection is active (reading R-20).
- The options bar shows the active tool's row. Selection: X, Y (the centre), Scale % (step 0.1), W, H (locked: they
  set the scale; unlocked: shown only — R-2), Lock, Fit, Fill, Center (on the crop; without one, the video's own frame
  at 100% — R-19), Flip H, Flip V, Reset; a yellow warning beside Scale when the output is upscaled ("Upscaled: output
  may look soft"). Every number is a StepSpinBox: the wheel and ↑/↓ step, Shift ten times, Enter commits.
- Undo/Redo (Ctrl+Z; Ctrl+Y or Ctrl+Shift+Z; the ☰ menu): 200 steps of values; one drag is one step; typing or the
  wheel in one field within 500 ms is one step; a new video clears it.
- `.avloop` version 1 is read and written (source, transform, view; unknown keys kept at every level; the source found
  by its path, then its path relative to the project file; a newer version refused); `avloop.exe --project <file>`
  and a dropped `.avloop` open one (the file's menus and dialogs: AV-010).

### D-002 — The media (AV-002, 2026-10-26)

- A file opens by its content (probesize 50 MB, analyzeduration 10 s), never by its extension: the Open button and
  Ctrl+O (filter "All media (*.*)" and the known extensions), a drop on the window or the drop zone, and
  `avloop.exe <file>`. Probing runs on a worker; a file that cannot open says why in a toast — "This file has no video
  AvLoop can decode (container: …)", "Cannot open <name>: <reason>", and for an animated WebP "FFmpeg reads still
  WebP images only. Convert it to GIF, APNG or a video first." (reading R-15); the open video stays.
- The default video stream is decoded; every other stream is discarded at the demuxer (audio never decoded).
- The decoder (its own thread) gives the frame on screen at a time: a seek goes BACKWARD to the keyframe and decodes
  to the target PTS (a container that lands after the target is sought again 1, 2, 4, 8 s earlier, then from the
  start; MPEG-TS/PS rewind by byte; after a seek decoding waits for a keyframe packet); moves forward within 2 s
  decode on. Corrupt frames are skipped and counted. 200 random seeks give the exact frame (software and D3D11VA).
- D3D11VA for H.264, HEVC, VP9, AV1, MPEG-2, VC-1 (not for sources with alpha); a failure reopens the codec in
  software and finds the position again, said once a file. VP8/VP9 with alpha decode through libvpx.
- Frames are normalized: rotation and flips upright (display matrix as ffmpeg's autorotate reads it), square pixels
  (SAR applied, even sizes), bwdif for interlaced frames, zscale + tonemap=hable to SDR BT.709 for PQ/HLG, RGBA
  premultiplied, at the working size — the display size with the long side capped at 1920 until a crop sets it
  (reading R-16).
- The frame cache: an LRU of 1536 MB (256–8192) with a disk spill in `%LOCALAPPDATA%\AvLoop\cache\<pid>-<ms>\` (20 GB,
  oldest first; deleted on exit; stale sessions deleted at start; a full disk stops the spill); below 1 GB of free RAM
  the cache is halved with a toast.
- The status bar: `<file> · 1920×1080 · 29.97 fps · H.264 · VFR → CFR` (`HDR → SDR`, `interlaced`, `alpha` when so),
  `GPU: <adapter> · HW decode on|off`, `RAM … MB · VRAM … MB · Cache …/… GB` each second; `--mem-log <file>` writes the
  same each second.
- `avloop-cli probe <video>` (JSON) and `avloop-cli frame <video> --at N -o x.png [--hw on|off] [--size WxH]`.

### D-001 — The ground (AV-001, 2026-10-26)

- The repository builds with `scripts\fetch-deps.ps1` (FFmpeg n7.1.5 LGPL shared, pinned by SHA-256, newest-7.1
  fallback) and `scripts\build.ps1` (MSVC v143 by vswhere, Ninja, Qt 6.8.3; `/W4 /WX /permissive- /utf-8`; outputs in
  `build\bin` with windeployqt's Qt DLLs and the FFmpeg DLLs). `avloop-cli version` prints FFmpeg's version,
  configuration and licence ("LGPL version 3 or later") and fails on `--enable-gpl` or `--enable-nonfree`.
- `avloop.exe` opens maximized (minimum 1280×720; the normal geometry and both splitters remembered in
  `%APPDATA%\AvLoop\AvLoop.ini`): the 36 px top bar (☰, the logo, *AvLoop*, Open | the options bar's slot | Preview,
  Export — the only three buttons), the 40 px tool rail (nine tools in four groups; tooltips "Name (Key)"; V M U E H Z I
  O K switch at once; the active tool orange), the canvas area with the drop zone (dashed, Open video…), the 260 px
  side panel (a preset row; Crop, Loop, Output, Quality sections, collapsible, state remembered), the timeline area
  (transport row: Go to In, Prev, Play, Next, Go to Out, Loop; the time readout) and the 22 px status bar. Open,
  Preview, Export and the transport are disabled until a video can be opened (AV-002 on).
- The ☰ menu lists New, Open Project…, Save, Save As…, Recent, Undo, Redo, Batch Queue…, Loop Finder…, Theme (Navy,
  Black), Settings…, Keyboard shortcuts, About AvLoop, Exit; Theme and Exit work, the rest wait for their phases.
- Navy (default) and Black come from one token table (`Theme.cpp`) into `resources/themes/app.qss` + the theme's own
  file, written with `@token` placeholders only; the switch needs no restart.
- 43 icons of spec §5.3 drawn by the app (24×24, stroke 1.75, `currentColor`), recoloured per state (normal text2,
  hover text, active accent, disabled 40%) and rendered per device pixel ratio; the app icon `avloop.ico` (16 … 256)
  made from `avloop.svg` by `tools/icons/iconbake.cpp`; every window uses it.
- The lab: `--shot <png>`, `--walk <script> --walk-out <dir>`, `--theme`, `--tool`, `--scale`; lab runs keep their
  settings in a temporary folder.

(sections below: the owner's specification as written)

## 1. Summary

AvLoop is a Windows desktop app (C++20, Qt 6, FFmpeg). It cuts a chosen time range (In to Out) and a chosen screen area (crop) out of any video and turns it into a seamless looping WebM for OBS Studio's Media Source.

This document is written for an AI implementation agent. The agent first builds a phase system (per §2, following the owner's example), then builds the whole application phase by phase. Nothing that is not written here is built. Where something is unclear, the agent records a "reading" and continues; the owner can veto readings later.

### 1.1 The owner's words (translated; quoted in README §1)

> "I want to build a piece of software called AvLoop. Its main job is to take any video, cut it at a specific In/Out and screen area, and create a loop video of that size. … The point is that a resized WebM is exported that feels like a loop."

Decided with the owner in discussion:

1. Windows only. Used in OBS.
2. All suggested features are kept; audio is dropped.
3. The Crop tool decides which part is exported. The Selection tool moves and scales the source video on the canvas.
4. The crossfade happens inside the selected range (its end blends into its start).
5. Tools instead of buttons wherever possible. One Preview button opens a separate window with a slider that shows only the exported part, exactly as it will be exported.
6. Output quality is adjustable. Maximum output is 1080p (long side 1920 px). No 4K output.
7. Several aspect ratios can be chosen while cropping. Frame rate is adjustable.
8. Compact GUI. Orange with black or navy blue theme. The app draws its own icons.
9. "Full screen" means a maximized window, not true full screen.
10. No code file over 600 lines. Phases are created following the owner's example (the CabTV AirTicker2 programme).

Added by the owner later (2026-10-26):

1. Maximum care with memory (§3.7). Use the GPU (§3.6).
2. State which libraries are used and why (§3.8). Any video must be importable (§3.9).
3. No code signing. Windows only.
4. Autonomous mode: the AI does all coding, testing, e2e and walks itself, decides on its own when stuck, and asks the owner nothing (§2.8).
5. Auto-push to `https://github.com/rubeldbc/avLoop` after every phase. Public repository; anyone can contribute. The README is always professional and up to date, with app screenshots (§2.9).

### 1.2 What is built (scope)

| # | Item | In short |
| --- | --- | --- |
| 1 | Ground and shell | CMake project, maximized main window, theme, icons, gate tools |
| 2 | Media | Open any video, probe, frame-accurate decode, cache |
| 3 | Canvas and Selection | Canvas, Zoom/Hand, move and scale the video |
| 4 | Crop tools | Rect, Rounded Rect, Ellipse, ratios, Feather |
| 5 | Timeline | Thumbnails, playhead, In/Out, playback |
| 6 | Loop engine | Simple, Crossfade, Ping-pong, Smooth turn, Speed, FPS |
| 7 | Render pipeline | One frame renderer shared by preview and export |
| 8 | Preview window | Live and Encoded preview, seam marker, background |
| 9 | Export | VP9/VP8 alpha WebM, quality, target file size |
| 10 | Project and settings | `.avloop` file, Undo/Redo, presets, autosave |
| 11 | Keyframes | Keyframes for video position and scale |
| 12 | Auto Loop Finder | Find the best In/Out pairs |
| 13 | Batch and other formats | Export queue, GIF, APNG, MP4, ProRes 4444 |
| 14 | Installer | Windows installer with Inno Setup |

### 1.3 What is not built

- Audio: no audio is decoded, played or exported.
- Output above 1080p.
- A video rotation tool (rotation metadata is only corrected), colour correction, text or stickers, multiple video layers.
- Multiple crop areas at once. The crop frame itself is not keyframed (output size is fixed).
- macOS or Linux builds, auto-updater, licence keys, telemetry.
- Code signing (the owner: not needed).

## 2. Mandatory rules for the AI developer

Before writing any code, the agent builds a complete phase system that follows the owner's example (`26-10-25-005447-CabTV-AirTicker2`) exactly. The example's structure, file names, headings, statuses and close procedure carry over unchanged; only the programme settings below change.

### 2.1 Programme identity

| In the example | In AvLoop |
| --- | --- |
| Programme `CABTV-AIRTICKER2` | Programme `AVLOOP` |
| Prefix `T2`, phase `T2-NNN` | Prefix `AV`, phase `AV-NNN` (corrective: `AV-NNNa`) |
| `REQ-T2-NNN` | `REQ-AV-NNN` |
| ADR `T2ADR-NNNN` | ADR `AVADR-NNNN` |
| D-483 … (product series) | From D-001, one reserved per item |
| Commit `[T2-NNN]` | Commit `[AV-NNN]`; plan commit `[AV] Plan` |
| Folder `phases/26-10-25-005447-CabTV-AirTicker2` | `phases/<YY-MM-DD-HHMMSS>-AvLoop` (planning time, Asia/Dhaka) |
| Lab: test OBS | Lab: `avloop-cli` + ffprobe + screenshots (§2.6) |

### 2.2 What the phase folder holds (as in the example)

| File | Purpose |
| --- | --- |
| `README.md` | §1 the owner's words (verbatim, including this doc's §1.1) · §2 item table · §3 readings · §4 what is not built · §5 where things are |
| `AGENT_BOOTSTRAP.md` | The agent's starting page: launch boundary, repository location, reading list, rules for every phase |
| `EXECUTOR-PREAMBLE.md` | Order of authority, frozen outcomes, per-phase procedure, validation and close |
| `AUTONOMOUS_STATE.md` | Canonical state: Plan in one paragraph, Grants, Waiting legs, The lab here, Completion record |
| `CONTINUE.md` | Where to pick up after a break |
| `DECISIONS.md`, `RISKS.md`, `OWNER-ACTIONS.md`, `PLANNING-VALIDATION.md` | Decisions, risks, owner decisions/vetoes, plan validation |
| `PROGRESS.json` | Generated by `sync-progress.mjs`; never edited by hand |
| `requirements/01-00_REQUIREMENTS.md` | REQ-AV rows: ID, Requirement, Owner item, Phase, Evidence, S |
| `control/00_PHASE_INDEX.md` | Generated index |
| `control/01_PHASE_DEPENDENCY_GRAPH.md` | Dependency graph and reasons |
| `control/02_MAJOR_CHECKPOINTS.md` | §1 gates, §2 situations (S1, S2 …) |
| `control/03_…` to `control/NN_SHAPES_<ITEM>.md` | Detailed shape of each item (from the matching part of this doc) |
| `control/04_THE_LAB_AND_THE_WALK.md` | Evidence kits and rules (§2.6) |
| `control/phase-tools/` | `lib.mjs`, `check-loc.mjs`, `sync-progress.mjs`, `validate-phase-system.mjs`, `check-continuity.mjs`, `close-phase.mjs` (ported from the example, prefix `AV`) |
| `control/lab/` | `walk-phase.mjs` and AvLoop's own kit (`av-lab.mjs`) |
| `phases/AV-NNN-<slug>.md` | One contract per phase |
| `execution/_TEMPLATE-EXECUTION.md`, `execution/AV-NNN-EXECUTION.md`, `execution/AV-NNN-walk/` | Execution records and evidence |
| `adr/README.md`, `adr/AVADR-NNNN-<slug>.md` | Architecture decisions |

### 2.3 Phase file format

Frontmatter (YAML), exactly like the example:

```yaml
---
phase: AV-004
title: "The crop tools"
status: NOT_STARTED
owner_item: 4
track: A
checkpoint_id: CP-A1
is_checkpoint: false
risk: Medium
permission_class: VERIFY_ENVIRONMENT_FIRST
code_budget: 550
depends_on: [AV-003]
unlocks: [AV-005, AV-015]
requirements: [REQ-AV-004, REQ-AV-016]
read_scope:
  - phases/<folder>/control/07_SHAPES_THE_CROP.md
  - after AV-003: src/ui/canvas/Canvas.h
---
```

Body sections, in this order and with exactly this wording (the validator looks for them): `# <Title>`, `Canonical current execution state: ../AUTONOMOUS_STATE.md`, `**Goal.**`, `**Ground truth**`, `**Do**`, `**Do not**`, `**Done when**`, `**Decisions this phase must record**`, `**Validation**`, `**Seam if this splits.**`

- **Status values:** `NOT_STARTED`, `READY`, `IN_PROGRESS`, `TESTING`, `RECOVERY`, `BLOCKED_EXTERNAL`, `DONE`, `FAILED_RETRYABLE`, `DEFERRED`, `SPLIT`.
- **Risk:** `Low` / `Medium` / `High`. **Permission class:** `AUTO_ALLOWED`, `VERIFY_ENVIRONMENT_FIRST`, `EXTERNAL_CAPABILITY_REQUIRED`, `EXPLICIT_EXTERNAL_AUTHORIZATION_REQUIRED`.
- A normal phase's ordinal = its item number. A checkpoint phase has `owner_item: 0`, `is_checkpoint: true`.
- A later owner wish or a corrective takes the next free letter (`AV-004a`) and names `parent_phase`; an item after a closed checkpoint is `late: true`.

### 2.4 Line limits (hard)

1. **No code file over 600 lines**, anywhere in the repository: `.c .cpp .h .hpp .cmake CMakeLists.txt .qss .mjs .js .ps1 .py .iss .glsl .frag .vert .qrc .ui`. `check-loc.mjs` must PASS before every commit.
2. New files aim for under 450 lines. If a change would take a file past 600, split it at its seam first (e.g. `Timeline.cpp` → `Timeline.cpp` + `TimelinePaint.cpp`).
3. Programme documents and tools (inside `phases/`) stay under 300 lines.
4. Each phase's `code_budget` (hand-written lines) is normally 300–600. Going over 150% needs a recorded deviation or a split.

### 2.5 Per-phase procedure and close

1. Read `AUTONOMOUS_STATE.md`, `EXECUTOR-PREAMBLE.md`, `PROGRESS.json`, the active phase and its `read_scope`.
2. Check the working tree and the line counts of the files to be touched.
3. Build one coherent slice. Three distinct root-cause attempts on the same failure require an explicit decision (recovery or split). Never weaken a gate.
4. Focused tests → the gates the change touches → `check-loc.mjs` PASS.
5. Write `execution/AV-NNN-EXECUTION.md` from the template (The owner's words, What was built, Decisions recorded, Specification delta, Verification, The walk, The pictures, The regressions, External freshness, Files and line counts, Recovery log, Deviations, Blocked legs, Staged paths, Next).
6. `node control/phase-tools/close-phase.mjs AV-NNN "<one line>"`.
7. Commit explicit paths with `[AV-NNN]`, message from a file (`git commit -F`). `git fetch`, then `git push origin main` (never force).
8. Promote the next eligible phase. A failure found is fixed before the next phase.

### 2.6 Lab and walks (evidence)

- **`avloop-cli`**: a headless target built from the same core. `avloop-cli export <project.avloop> -o <out>`, `avloop-cli probe <video>`, `avloop-cli frame <project> --at N -o x.png`.
- **`avloop.exe --shot <file.png> [--project p] [--tool T]`**: opens the main window, takes a screenshot and exits (lab use only).
- **`tools/media/make-samples.mjs`**: generates test videos with ffmpeg (`testsrc2`, rotation 90°, VFR, HDR-tagged, interlaced, SAR 4:3, alpha WebM, ProRes 4444, 4K, a Bengali-named path).
- **`control/lab/av-lab.mjs`**: `probe(file)` (ffprobe JSON), `alphaOf(file)` (reads alpha with the libvpx decoder), `frameAt(file, n)`, `seamDiff(file)` (difference between last and first frame), `pixelAt(png, x, y)`.
- Evidence means: the build, unit tests, ffprobe output, screenshots (with a described look) and log lines. Writing only "passed" is not enough; record the observed value.
- Every command runs with a 5-minute timeout. A walk leaves no temp file or process behind.

### 2.7 Other rules

- Nothing unrequested is added; nothing requested is silently dropped.
- Every user-visible word goes through `tr()` (no literals in code). Labels at most 40 characters. UI language English (reading R-1).
- Project-file keys and settings keys are never renamed; a format change bumps `version` and adds a migration for the old one.
- Code style: `.clang-format` (LLVM-based, 120 columns), FFmpeg resources via RAII (`std::unique_ptr` + custom deleter), no raw `new/delete` (except Qt parents), warnings as errors (`/W4 /WX`).
- External dependency versions are checked on first use and written under External freshness.

### 2.8 Autonomous mode (the owner's words)

> "In autonomous mode, from start to finish, the AI does all coding and testing, e2e and walks by itself. If there is a problem it solves it on its own. If it gets stuck anywhere it makes the decision itself and gets the work done. It will not ask me anything. Once started, it finishes the whole job by itself."

1. **The go-ahead is given.** This document is the "word to code". The agent first builds the phase system, commits and pushes `[AV] Plan`, then runs from AV-001 to the last checkpoint without stopping.
2. **No questions to the owner.** The agent closes every row in `OWNER-ACTIONS.md` itself ("Decided by the agent" + reason). Every assumption is written as a reading (R-1, R-2 …) in README §3 and in `DECISIONS.md`, so the owner can veto later. Work never waits for it.
3. **When stuck:** three distinct root-cause attempts on the same failure, then choose the simplest alternative that meets **Done when**, record it under Deviations in the execution record, and move on. Gates are never weakened.
4. **External blockers** (network down, failed download): the phase goes `BLOCKED_EXTERNAL`, work continues on the next independent eligible phase, and the agent retries later. All `BLOCKED_EXTERNAL` must be cleared before the last checkpoint.
5. **The agent builds the environment itself.** Missing tools are installed with winget/pip: CMake, Ninja, Qt 6.8 (`aqtinstall`), FFmpeg (`scripts/fetch-deps.ps1`), Inno Setup, Node, `gh`. Versions go under External freshness.
6. **The agent runs all tests itself:** unit (`ctest`), e2e (`avloop-cli`), GUI walks (`avloop.exe --walk <script.json>`: replays clicks, drags and key presses on real widgets and takes screenshots), the memory walk and the GPU walk (§3.6, §3.7). No walk is deferred to a checkpoint; each phase runs its own walk before it closes.
7. **A failure found is fixed before the next phase, never carried.**
8. **At the end**, the Completion record in `AUTONOMOUS_STATE.md` says what was built, which readings were taken, what was dropped and why.

### 2.9 GitHub, public repository and README

- **Remote:** `https://github.com/rubeldbc/avLoop` (public, `main`). Authentication uses the machine's existing git credentials or `gh auth`.
- **Auto-push:** at every phase close, `git fetch origin` then `git push origin main`. Never force-push. If the push fails, retry after 30 s, 2 min and 10 min. If it still fails, work continues and the push happens at the next close; this is written in the record.
- **Because it is public:** no secrets, personal paths, emails or tokens are committed. `tools/check-secrets.mjs` runs before every commit. Large binaries (FFmpeg DLLs, builds, test videos) are in `.gitignore`; test videos are generated by `make-samples.mjs`.
- **Contribution files:**
  - `LICENSE`: MIT (reading R-4; compatible with the LGPL dependencies).
  - `CONTRIBUTING.md`: build steps, the 600-line rule, code style, commit format, PR process.
  - `CODE_OF_CONDUCT.md` (Contributor Covenant 2.1) and `SECURITY.md`.
  - Issue templates (Bug, Feature) and a PR template.
- **CI:** `.github/workflows/ci.yml` on `windows-latest`: fetch deps, build, `ctest` (non-GPU tests, using the CPU renderer), `check-loc.mjs`. After every push the agent watches the result with `gh run watch`; if red, it is fixed before the next phase.
- **README.md is always professional and current.** It is updated as part of every phase close:
  - Logo, badges (CI, License, Platform: Windows) and a one-line pitch.
  - **Real app screenshots, not mock-ups**: `docs/screenshots/` (main window, ellipse crop, timeline, preview window, export), taken with `avloop.exe --walk tools/walks/readme-shots.json` from a sample project. Retaken in every phase that changes the UI. Each PNG under 300 KB.
  - Feature list: only what actually works; planned features in a separate Roadmap section.
  - Guide for OBS: Media Source, Loop on, hardware decoding off.
  - Download and install, build from source, keyboard shortcut table, project structure.
  - Contributing, License and Third-party notices.

## 3. Tech stack and architecture

Preview and export use the same `FrameRenderer`, so what the preview shows is exactly what gets exported. This is the central architecture decision (AVADR-0001).

### 3.1 Stack

| Part | Choice | Note |
| --- | --- | --- |
| Language | C++20 | MSVC 2022 (v143), `/W4 /WX /permissive- /utf-8` |
| Build | CMake ≥ 3.28, Ninja | `scripts/build.ps1` (`-Fresh`, `-Config Release`) |
| GUI | Qt 6.8 LTS: Core, Gui, Widgets, OpenGLWidgets, Svg, Concurrent, Test | LGPL, dynamically linked |
| Video | FFmpeg 7.1, **LGPL shared** build (BtbN `win64-lgpl-shared-7.1`) | libavformat, libavcodec, libavutil, libswscale, libavfilter; with libvpx (BSD), dav1d, zimg |
| Tests | Qt Test + CTest | `ctest -R avloop` |
| Installer | Inno Setup 6 + `windeployqt` | Item 14 |
| Tooling | Node 20+ (phase tools), ffmpeg/ffprobe on PATH | Development only |

Licence rule: no GPL component (libx264, libx265) is linked. MP4 H.264 uses the Windows Media Foundation encoder `h264_mf`. The installer carries LGPL and third-party notices.

### 3.2 Modules

| Module | Depends on | Responsibility |
| --- | --- | --- |
| `core/` | Qt Core only | Project model (`Project`, `CropShape`, `VideoTransform`, `LoopSettings`, `OutputSettings`, `QualitySettings`, `Keyframes`), geometry, ratios, loop mapping (`LoopMap`), time mapping (`TimeMap`), JSON serialization, Undo commands |
| `media/` | core + FFmpeg | `MediaProbe`, `Decoder` (seek + frame-accurate), `SourceNormalizer` (rotation, SAR, deinterlace, HDR→SDR), `FrameCache`, `ThumbnailWorker`, `Encoder`, `Muxer`, `Interpolator` (minterpolate) |
| `render/` | core + media + Qt Gui | `FrameRenderer` (RGBA image of one output frame), `MaskBuilder` (Rect/Rounded/Ellipse + Feather), `Blend`, `CpuRenderer` |
| `export/` | render + media | `ExportJob` (background thread, progress, cancel), `ExportQueue`, `SizeEstimator`, `LoopFinder` |
| `ui/` | everything + Qt Widgets | `MainWindow`, tools, canvas, timeline, panels, `PreviewWindow`, theme, icons |
| `app/` | ui | `main.cpp` (`avloop.exe`) |
| `cli/` | export | `main.cpp` (`avloop-cli.exe`), the same code without a GUI |

Rules: `core/` never includes Widgets or FFmpeg. `ui/` never calls FFmpeg directly. The model changes only through Undo commands; `Project` emits `changed(Aspect)` on every change.

### 3.3 Coordinate system

- **Scene space**: an unbounded canvas in pixels. On load the video sits at `(0,0)`, scale 100%, size = its normalized size (after rotation and SAR).
- **VideoTransform**: centre `(x, y)`, `scale` (0.05–10.0), `flipH`, `flipV`. The video rectangle = centre ± (w·scale/2, h·scale/2).
- **CropShape**: a rectangle in scene space `(x, y, w, h)` + `kind` (Rect, Rounded, Ellipse) + `radius` + `feather`. The crop may extend past the video; that part is transparent.
- **Output size**: default the crop's `w×h`; if the long side exceeds 1920, scaled down keeping the ratio; finally both sides rounded to the nearest even number (minimum 16).

### 3.4 How one output frame is made

1. `LoopMap` turns output frame `k` into one or two segment frames + a weight (§8).
2. `TimeMap` turns each segment frame into a source time (via Speed and output FPS), then into one or two source frames according to the FPS method.
3. Normalized RGBA source frames come from `FrameCache` (or from the `Decoder` if missing).
4. Each is drawn into the crop rectangle using the `VideoTransform` at that time (keyframes interpolated), directly at the output size (premultiplied ARGB, smooth scaling).
5. With two frames, they are blended by weight.
6. The alpha mask from `MaskBuilder` is multiplied in (the mask is built once and cached).
7. The resulting RGBA goes to the preview, or for export through swscale to `yuva420p` and the encoder.

### 3.5 Threading

- Decoding and encoding never happen on the UI thread. The decoder has its own thread, thumbnails use a separate decoder instance, export runs in a `QThread`.
- `FrameCache`: in-memory LRU with a budget (§3.7). When Ping-pong or a large segment exceeds the budget, it spills to disk: `%LOCALAPPDATA%\AvLoop\cache\<session>\` (crop region at output size, raw RGBA). The session cache is deleted on exit, and stale sessions are deleted at the next start.

### 3.6 GPU use

The owner: "Use the GPU." The GPU is used wherever it helps, and every GPU path has a guaranteed CPU fallback.

| Step | GPU path | Fallback |
| --- | --- | --- |
| Decode | FFmpeg D3D11VA hwaccel (`AV_HWDEVICE_TYPE_D3D11VA`): H.264, HEVC (8/10-bit), VP9, AV1, MPEG-2, VC-1 | Software decode, automatically; logged once per file for unsupported codecs or errors |
| Composite (transform, crop, blend, mask, feather) | OpenGL 3.3 core shaders; offscreen FBO (`QOpenGLContext` + `QOffscreenSurface`); canvas, Preview and Export use **the same shaders** | CPU renderer (`render/CpuRenderer`) with the same result; results differ by ≤ 2/255, checked by `test-renderer` |
| Readback (export) | Double-buffered PBO, async | `glReadPixels` |
| Encode | **CPU (libvpx)**: there is no hardware encoder for VP8/VP9 with alpha; `row-mt=1`, `threads` = logical cores, `tile-columns` by output size | — |
| Motion interpolation | CPU (`minterpolate`) | — |

- Settings has "Hardware decoding: Auto / Off" and "GPU rendering: Auto / Off". Auto = try, fall back on failure, and say so in a toast.
- The status bar shows `GPU: <adapter name> · HW decode on`.
- GPU walk: export the same project with the GPU on and off; mean per-frame difference ≤ 1.0, maximum ≤ 4 (8-bit).

### 3.7 Memory rules (maximum care)

The owner: "Be extremely careful about memory." Every rule below is mandatory and checked by a gate.

1. **Every cache and queue has a fixed limit.** No unbounded containers.

   | Item | Limit (default) |
   | --- | --- |
   | `FrameCache` (RAM) | 1536 MB (Settings: 256–8192) |
   | GPU texture pool | 512 MB or 25% of VRAM, whichever is smaller |
   | Disk cache | 20 GB, oldest deleted first |
   | Thumbnails | 400, 160×90, about 23 MB |
   | Decoder packet queue | 64 packets or 32 MB |
   | Export pipeline | decode→render 6 frames, render→encode 6 frames (backpressure) |
   | Undo stack | 200 steps (small data only, never images) |
2. **A whole segment is never decoded into RAM.** The reversed half of Ping-pong is decoded GOP by GOP into the disk cache and read from there.
3. **Working resolution:** the cache holds only what the output needs (crop region × effective scale, at most the source size). A 4K source is scaled down right after decoding; full 4K frames are never kept.
4. **RAII everywhere:** every `AVFormatContext`, `AVCodecContext`, `AVFrame`, `AVPacket`, `SwsContext`, `AVFilterGraph`, `AVBufferRef` and GL object lives in `std::unique_ptr` + custom deleter (`media/FfmpegPtr.h`). Frames are reused through frame pools (`av_buffer_pool`).
5. **Lifetimes:** closing the Preview window releases its player, textures and cache. Opening a new video releases everything from the previous one. A finished or cancelled export deletes all its buffers and temp files.
6. **Under pressure:** if free system RAM drops below 1 GB, caches are halved and a toast is shown. If an allocation fails, the export stops cleanly with a message; it does not crash.
7. **Visible:** the status bar shows `RAM 812 MB · VRAM 210 MB · Cache 1.1/1.5 GB`.
8. **Checks (gates):**
   - `--mem-log <file>`: writes `GetProcessMemoryInfo` (Private Bytes) and DXGI VRAM every second.
   - Memory walk: 10 minutes of loop playback + 3 exports in a row + opening and closing Preview 5 times. After warm-up, Private Bytes growth < 50 MB, VRAM growth < 32 MB.
   - Leak check: zero CRT leak report in Debug; at checkpoints, all unit tests pass under AddressSanitizer (`/fsanitize=address`).

### 3.8 Libraries and why

| Library | Licence | Why it is used |
| --- | --- | --- |
| Qt 6.8 Core, Gui, Widgets | LGPLv3 | UI, events, settings, JSON, theme (QSS) |
| Qt OpenGL, OpenGLWidgets | LGPLv3 | GPU canvas and offscreen rendering (§3.6) |
| Qt Svg | LGPLv3 | Renders the app's own SVG icons with recolouring |
| Qt Concurrent | LGPLv3 | Thumbnails and Loop Finder in parallel |
| Qt Test | LGPLv3 | Unit and GUI tests |
| FFmpeg libavformat | LGPL 2.1+ | Reads any container; writes WebM/MP4/MOV |
| FFmpeg libavcodec | LGPL 2.1+ | Decodes all codecs; encodes |
| FFmpeg libavutil | LGPL 2.1+ | Frames, pixel formats, D3D11VA hwcontext |
| FFmpeg libswscale | LGPL 2.1+ | RGBA ↔ `yuva420p` conversion and scaling |
| FFmpeg libavfilter | LGPL 2.1+ | `minterpolate`, `zscale` + `tonemap` (HDR), `bwdif` (deinterlace), `palettegen` for GIF |
| libvpx (inside FFmpeg) | BSD | VP8/VP9 encoding **with alpha**; decoding WebM with alpha (the native vp9 decoder drops alpha) |
| dav1d (inside FFmpeg) | BSD | AV1 software decoding |
| zimg (inside FFmpeg) | WTFPL | HDR→SDR tone mapping, correct colour spaces |
| Windows SDK: D3D11, DXGI, PSAPI | Windows | Hardware decode device, VRAM and memory measurement |
| Media Foundation (`h264_mf`) | Windows | MP4 H.264 export without GPL |
| Inno Setup 6 | Free | Installer |
| Node 20, ffprobe | Dev tools | Phase tools and walk checks; not shipped with the app |

Not used: Qt Multimedia (not frame-accurate, no alpha), OpenCV (heavy, not needed), libx264/libx265 (GPL).

### 3.9 Importing any video

- The Open dialog filter is "All media (\*.\*)" plus known extensions. Files are opened by **probing content**, not by extension (`avformat_open_input` + `avformat_find_stream_info`, probesize 50 MB, analyzeduration 10 s).
- **Containers:** MP4, MOV, MKV, WebM, AVI, WMV/ASF, FLV, MPEG-TS/M2TS/MTS, MPEG-PS/VOB, MXF, 3GP, OGV, Y4M, animated GIF/APNG/WebP.
- **Codecs:** H.264, HEVC, AV1, VP8, VP9 (with alpha), MPEG-1/2/4, WMV/VC-1, ProRes (4444 with alpha), DNxHD/HR, MJPEG, CineForm, HAP, QuickTime Animation, PNG-in-MOV, Theora, FFV1, uncompressed.
- **Source alpha is kept**: WebM alpha via the libvpx decoder, others via FFmpeg's own decoders.
- **Audio streams** are ignored and never decoded. With several video streams, the default one is used.
- **Paths:** Unicode (including Bengali names) and long paths (`\\?\`) are supported.
- **If a file cannot be opened**, a clear message names the container or codec and the reason; never a crash.
- `make-samples.mjs` creates at least one sample of each family above. The import walk opens them all and checks the first, middle and last frames as PNG.

## 4. Project folder and file structure

Approximate line counts are shown next to some files; anything heading towards 600 must be split at its seam. The agent may change names and splits when needed, but must record that in the execution record.

```text
AvLoop/
  CMakeLists.txt                 (~80)  root: options, subdirectories
  CLAUDE.md                             agent rules (600-line rule, close procedure)
  README.md  LICENSE  CONTRIBUTING.md  CODE_OF_CONDUCT.md  SECURITY.md
  .clang-format  .gitignore  LICENSE-THIRD-PARTY.md
  .github/
    workflows/ci.yml  ISSUE_TEMPLATE/  PULL_REQUEST_TEMPLATE.md
  cmake/
    FindFFmpeg.cmake             (~120)
    Qt.cmake  Warnings.cmake  Deploy.cmake
  scripts/
    build.ps1  test.ps1  format.ps1  deploy.ps1  fetch-deps.ps1
  third_party/ffmpeg/            (fetched by fetch-deps.ps1, git-ignored)
  resources/
    icons/*.svg   app/avloop.ico   app/avloop.rc
    themes/navy.qss  themes/black.qss  (each < 600)
    resources.qrc
  src/
    core/
      Project.h/.cpp  Geometry.h/.cpp  AspectRatio.h/.cpp
      CropShape.h/.cpp  VideoTransform.h/.cpp  Keyframes.h/.cpp
      LoopSettings.h  OutputSettings.h  QualitySettings.h
      LoopMap.h/.cpp  TimeMap.h/.cpp  Timecode.h/.cpp
      ProjectJson.h/.cpp  ProjectMigrate.cpp  Commands.h/.cpp
      Presets.h/.cpp  AppSettings.h/.cpp
    media/
      FfmpegPtr.h  MediaInfo.h  MediaProbe.h/.cpp
      Decoder.h/.cpp  DecoderSeek.cpp  DecoderHw.cpp  SourceNormalizer.h/.cpp
      FrameCache.h/.cpp  DiskCache.cpp  ThumbnailWorker.h/.cpp
      Encoder.h/.cpp  EncoderOptions.cpp  Muxer.h/.cpp  Interpolator.h/.cpp
    render/
      FrameRenderer.h/.cpp  GlRenderer.cpp  CpuRenderer.cpp  Shaders.cpp
      MaskBuilder.h/.cpp  Blend.h/.cpp
    export/
      ExportJob.h/.cpp  ExportQueue.h/.cpp  SizeEstimator.h/.cpp
      LoopFinder.h/.cpp  FormatProfiles.h/.cpp  OutputVerify.h/.cpp
    ui/
      MainWindow.h/.cpp  MainWindowActions.cpp  MainMenu.cpp  WalkRunner.cpp
      theme/Theme.h/.cpp  theme/IconFactory.h/.cpp
      tools/Tool.h  ToolManager.h/.cpp  SelectionTool.cpp  CropTool.cpp
            CropHandles.cpp  ZoomTool.cpp  HandTool.cpp  InOutTool.cpp  KeyframeTool.cpp
      canvas/Canvas.h/.cpp  CanvasView.cpp  CanvasOverlay.cpp  CanvasGl.cpp
      options/OptionsBar.h/.cpp  SelectionOptions.cpp  CropOptions.cpp  ZoomOptions.cpp
      timeline/Timeline.h/.cpp  TimelinePaint.cpp  TimelineInput.cpp
               ThumbStrip.cpp  Transport.h/.cpp
      panels/SidePanel.h/.cpp  CropPanel.cpp  LoopPanel.cpp
             OutputPanel.cpp  QualityPanel.cpp  Section.h/.cpp
      widgets/StepSpinBox.h/.cpp  TimeEdit.h/.cpp  Toast.h/.cpp
      preview/PreviewWindow.h/.cpp  PreviewPlayer.h/.cpp  PreviewBackground.cpp
      export/ExportBar.h/.cpp  BatchWindow.h/.cpp  LoopFinderWindow.h/.cpp
      dialogs/SettingsDialog.h/.cpp  AboutDialog.cpp
    app/main.cpp
    cli/main.cpp
  tests/
    CMakeLists.txt  test-geometry.cpp  test-aspect.cpp  test-loopmap.cpp
    test-timemap.cpp  test-project-json.cpp  test-commands.cpp  test-mask.cpp
    test-probe.cpp  test-decoder-seek.cpp  test-renderer.cpp  test-export.cpp
    test-keyframes.cpp  test-loopfinder.cpp  test-presets.cpp  test-memory.cpp
  tools/
    media/make-samples.mjs  icons/check-icons.mjs  check-secrets.mjs
    walks/*.json  (GUI walk scripts, incl. readme-shots.json)
  installer/AvLoop.iss
  docs/SPEC.md  docs/lore/avloop.md  docs/screenshots/*.png
  phases/<YY-MM-DD-HHMMSS>-AvLoop/   (§2.2)
```

- `docs/SPEC.md` starts as a copy of this document. In its §0 every phase writes the delivered behaviour under its own D-number (newest first).
- `docs/lore/avloop.md` = "Things that bite": the standing rules for traps that were found.
- Repository: `https://github.com/rubeldbc/avLoop`, branch `main`, public.

## 5. UI layout, theme and icons

The app starts in a maximized window (`showMaximized()`), not true full screen. Minimum size 1280×720. When the owner restores the window, its normal size and the splitter positions are remembered. The interactive HTML mock (`avloop-gui-mock.html`) shows this layout.

### 5.1 Layout

```text
+--------------------------------------------------------------------------------+
| [=] [logo] AvLoop [Open] | Options bar (changes with the active tool) | [Preview] [Export] |  36 px
+----+---------------------------------------------------------------+-----------+
| V  |                                                               | Crop      |
| M  |                                                               | Loop      |
| U  |                 Canvas (QOpenGLWidget)                        | Output    |  260 px
| E  |       outside the crop: 60% black dim; checkerboard           | Quality   |  side panel,
| H  |       where nothing is drawn                                  | (collapsible|  splitter
| Z  |                                                               |  sections) |
| I O|                                                               |           |
| K  |                                                               |           |
+----+---------------------------------------------------------------+-----------+
| |< < > >| 00:01:12.15 / 00:04:10.00 | thumbnails, ruler, In/Out, crossfade, keyframes |  150 px
+--------------------------------------------------------------------------------+
| status: source info | GPU | export progress [Cancel] | RAM / VRAM / cache      |  22 px
+--------------------------------------------------------------------------------+
```

- Left tool rail 40 px, icons 20 px, buttons 32×32. Tooltips show name + shortcut ("Crop: Ellipse (E)").
- Only three buttons in the top bar: Open, Preview, Export. Everything else is in the menu (☰) and shortcuts: New, Open Project, Save, Save As, Recent, Batch Queue, Loop Finder, Theme, Settings, Keyboard shortcuts, About, Exit.
- Right panel: four collapsible sections; their open/closed state is remembered. A preset dropdown sits above them. All numbers are `StepSpinBox` (wheel and ↑↓, Shift = ×10).
- Drag and drop opens files (video or `.avloop`). With no video, the canvas shows a large drop zone + Open.
- Status bar: source (`1920×1080 · 29.97 fps · H.264 · VFR → CFR`), GPU, export progress + Cancel, RAM/VRAM/cache use.
- Fonts: Segoe UI 9 pt; numbers and timecodes Consolas 9 pt. HiDPI: `Qt::HighDpiScaleFactorRoundingPolicy::PassThrough`; checked with screenshots at 100%, 150% and 200% scale.

### 5.2 Theme colours

Two themes: **Navy** (default) and **Black**. Colours come from one token table (`Theme.cpp`) into the QSS; there are no hard-coded hex values in the QSS (`@bg`, `@accent` placeholders).

| Token | Navy | Black | Use |
| --- | --- | --- | --- |
| `bg` | `#0E1726` | `#0B0B0C` | Window, around the canvas |
| `panel` | `#16223A` | `#151517` | Tool rail, panels, timeline |
| `panel2` | `#1D2B47` | `#1E1E21` | Inputs, section headers, hover |
| `border` | `#2A3A5C` | `#2C2C30` | Borders, splitters |
| `text` | `#E6EAF2` | `#ECECEC` | Main text |
| `text2` | `#9AA6BD` | `#9A9AA0` | Quiet text, labels |
| `accent` | `#FF7A1A` | `#FF7A1A` | Active tool, crop border, In/Out area, playhead, primary button |
| `accentHover` | `#FF9447` | `#FF9447` | Hover |
| `accentPress` | `#E5650A` | `#E5650A` | Pressed |
| `accentSoft` | `#FF7A1A` at 18% | `#FF7A1A` at 18% | In/Out area fill, selected row |
| `warning` | `#FFC53D` | `#FFC53D` | Upscale warning |
| `danger` | `#FF4D4F` | `#FF4D4F` | Errors |
| `success` | `#3DD68C` | `#3DD68C` | Export done |

Corners 2 px (compact, crisp look). Input height 24 px, row gap 6 px in panels. The theme switches from the menu or Settings without a restart.

### 5.3 Icons

- The app draws all its icons itself: SVG, `viewBox="0 0 24 24"`, `stroke-width="1.75"`, `stroke="currentColor"`, `fill="none"`, round caps and joins. No third-party icon set.
- `IconFactory` replaces `currentColor` at runtime to make four states: normal `text2`, hover `text`, active `accent`, disabled 40% opacity. Rendered per DPI with QSvgRenderer and cached.
- `tools/icons/check-icons.mjs` checks every SVG's viewBox, that no colour is hard-coded, and that every listed icon exists.

| Group | Icons (file names) |
| --- | --- |
| Tools | `tool-select`, `tool-crop-rect`, `tool-crop-round`, `tool-crop-ellipse`, `tool-hand`, `tool-zoom`, `tool-in`, `tool-out`, `tool-keyframe` |
| Top bar | `menu`, `open`, `preview`, `export` |
| Transport | `play`, `pause`, `frame-prev`, `frame-next`, `go-in`, `go-out`, `loop-on`, `loop-off` |
| Options bar | `fit`, `fill`, `center`, `flip-h`, `flip-v`, `lock`, `unlock`, `reset`, `zoom-fit`, `zoom-100`, `kf-prev`, `kf-next`, `kf-delete` |
| Other | `undo`, `redo`, `settings`, `batch`, `finder`, `warning`, `check`, `close`, `chevron` |
| App icon | `avloop.ico` (16, 24, 32, 48, 64, 128, 256): an orange circular loop arrow on a navy square with a white play triangle inside; generated from one SVG |

Every window (main, Preview, Batch, Loop Finder, Settings, About) sets the app icon.

## 6. Tools

The crop frame is a window; the Selection tool arranges the video behind it. Only one tool is active at a time; the active tool shows an orange icon and the options bar shows its options. Each tool is a class implementing the `Tool` interface (`activate`, `deactivate`, `mousePress/Move/Release`, `keyPress`, `paintOverlay`, `cursor`).

| Tool | Shortcut | Job | Options bar |
| --- | --- | --- | --- |
| Selection | V | Move and scale the source video | X, Y, Scale %, W, H, Lock, Fit, Fill, Center, Flip H, Flip V, Reset |
| Crop: Rect | M | Rectangular export frame | Ratio, X, Y, W, H, Swap W/H, Clear |
| Crop: Rounded | U | Rounded rectangle | Ratio, X, Y, W, H, Radius, Feather |
| Crop: Ellipse | E | Ellipse or circle | Ratio, X, Y, W, H, Feather |
| Hand | H (middle-mouse drag always) | Pan the view | — |
| Zoom | Z | Zoom the view (no effect on output) | Fit, 100%, 200%, zoom % |
| In | I | Set In at the playhead / click on the timeline | In timecode |
| Out | O | Set Out at the playhead / click on the timeline | Out timecode, duration |
| Keyframe | K | Keyframes of the video transform | Add, Delete, Prev, Next, Interpolation (Linear/Ease) |

### 6.1 Crop tools (M / U / E)

- With no crop, dragging draws a new crop (marquee style). With a crop: dragging inside moves it, the 8 handles resize it, dragging outside draws a new crop that replaces the old one (one Undo step).
- Switching to another crop tool changes the current crop's `kind`; the rectangle stays the same.
- Modifiers: Shift = lock ratio (1:1 when Free, a perfect circle with Ellipse); Alt = draw/resize from the centre; arrows = move 1 px, Shift+arrows = 10 px; Esc = cancel drawing; Delete = remove the crop; double-click = crop equal to the whole video.
- Ratio dropdown: Free, Original (the video's), 1:1, 4:3, 3:4, 16:9, 9:16, 21:9, 3:2, 2:3, 4:5, Custom (two W:H fields). Changing the ratio keeps the crop centre and keeps the area as close as possible.
- Overlay: 60% black outside the crop, a 1.5 px orange crop border, 8×8 white handles with orange borders; for Ellipse and Rounded a dashed bounding box; while dragging, rule-of-thirds guides and a size label (`640 × 640 → 640 × 640`, output size on the right).
- Snapping: to the video's edges and centre within 8 px (screen units); holding Ctrl turns snapping off.
- With no crop, Preview and Export use the whole video rectangle.
- Feather: 0–200 px (output units), softening inward from the edge. Even at Feather 0 the edge is anti-aliased (1 px). Radius: 0 up to half the short side.
- A Rect crop fully inside the video with feather 0 needs no alpha; Output has "Alpha: Auto / On / Off" (Auto = only when needed).

### 6.2 Selection tool (V)

- Dragging inside the video moves it. Dragging one of the four corner handles scales it keeping its ratio (the opposite corner stays fixed; Alt = from the centre). The video's ratio never changes (scale is one number).
- The video border is a quiet blue-white dashed line (distinct from the crop's orange). The crop frame stays still.
- Options bar: X/Y (video centre, scene px), Scale % (5–1000, step 0.1), W/H (when locked, changing one changes the other; unlocked = display only, the video is never distorted — reading R-2).
- Fit: the whole video fits inside the crop, centred. Fill: the crop is completely filled, centred. Center: brought to the crop's centre, scale unchanged. Reset: 100%, `(0,0)`, flips off.
- Warnings: if the effective output scale (video scale × output/crop ratio) is above 100%, a yellow `warning` icon appears next to Scale with the tooltip "Upscaled: output may look soft". If any part of the crop is outside the video, the status bar says "Transparent area in output".
- With keyframes, Selection changes are written to the keyframe at the playhead; if there is none at the playhead, a new keyframe is created (like After Effects).

### 6.3 Zoom (Z) and Hand (H)

- Zoom: click = zoom in (steps 10, 25, 33, 50, 67, 100, 150, 200, 300, 400, 800%), Alt+click = zoom out, drag = zoom to a rectangle. In any tool: Ctrl+wheel = zoom around the cursor, Ctrl+0 = Fit, Ctrl+1 = 100%, Ctrl+= / Ctrl+- zoom.
- Hand: drag = pan. In any tool, middle-mouse drag = pan, wheel = vertical pan, Shift+wheel = horizontal pan. (Space is reserved for Play/Pause — reading R-3.)
- Zoom and pan are not saved in the project's Undo history.

### 6.4 In (I), Out (O) and Keyframe (K)

- Pressing I/O sets In/Out at the playhead (without changing the tool). When the tool is active, clicking on the timeline sets it there. Alt+I / Alt+O = clear back to the start/end.
- In < Out always; setting In after Out moves Out to the end of the file (and the reverse). Minimum length 2 output frames. Values snap to the output FPS frame grid (§8).
- K: add/update a keyframe at the playhead with the current `VideoTransform`. Diamond marks on the timeline; drag a diamond to move it in time, Delete removes it, `[` / `]` = previous/next keyframe. Interpolation per keyframe: Linear or Ease (smoothstep).

## 7. Timeline

The timeline is frame-accurate: the frame under the playhead is exactly the frame the canvas shows. Height 150 px including the transport row (splitter range 110–260).

### 7.1 Rows (top to bottom)

1. **Ruler (18 px):** timecode `HH:MM:SS.FF` or frame number (double-click the ruler to switch). Tick spacing follows the zoom (1 frame up to 10 minutes); labels never overlap.
2. **Thumbnail strip (54 px):** thumbnails sized to fit, built by `ThumbnailWorker` on its own decoder with keyframe seeks. The visible part first, the rest later. Until ready, a `panel2` block.
3. **In/Out overlay:** the In–Out area has an `accentSoft` fill + a 3 px orange bar on top; outside it, 50% black. A draggable bracket handle (10 px) at each end. Dragging a handle shows that frame on the canvas.
4. **Crossfade band:** in Crossfade mode, an orange gradient (transparent to solid) over the F frames before Out, and a dashed outline over the F frames after In (the part that gets blended). Dragging the gradient's left edge changes F (range 1 to ⌊D/2⌋). In Ping-pong + Smooth turn, both ends of the turn are filled with a quiet orange.
5. **Keyframe row (16 px):** diamonds (selected = solid orange, others = hollow). Ease keyframes are diamonds with rounded corners.
6. **Playhead:** an orange vertical line + a triangle on top, over all rows.

### 7.2 Input

| Action | Result |
| --- | --- |
| Click/drag on the ruler or strip | Scrub; fast keyframe preview while dragging, exact frame on release |
| Ctrl+wheel | Zoom around the cursor (whole file down to 12 px per frame) |
| Wheel / Shift+wheel | Horizontal scroll |
| `\` (backslash) | Fit whole file; Shift+`\` = fit In–Out |
| Drag an In/Out handle | Change In/Out (one Undo step) |
| Drag the orange bar above the In–Out area | Move the whole area, keeping its length |
| Frame snapping | All values on the output FPS frame grid; the playhead snaps to In/Out/keyframes within 8 px |

### 7.3 Transport and playback

- A row of small icons on the left: Go to In, Prev frame, Play/Pause, Next frame, Go to Out, Loop on/off; then a `TimeEdit` (type to jump) and the total duration, then the In, Out and length readout.
- Shortcuts: Space = Play/Pause; ←/→ = 1 frame; Shift+←/→ = 10 frames; Home/End = to In/Out; Ctrl+Home/End = start/end of file; L = loop on/off.
- Editor playback shows the **source video** (without the loop mode, with Speed). With loop on, it cycles between In and Out. The real output with the loop mode is seen only in the Preview window (§9).
- Playback clock: `QElapsedTimer` + display refresh. If decoding falls behind, frames are dropped and the clock never stops; the status bar counts dropped frames. 1080p H.264 with hardware decoding drops no frames at 30/60 fps.
- Seek: first `av_seek_frame(BACKWARD)` to the previous keyframe, then decode up to the target PTS. Forward moves within the same GOP keep decoding without seeking. `test-decoder-seek` checks frame numbers over 200 random seeks (reading the counter burned into testsrc2).

## 8. Loop modes, frame rate and Speed

All loop maths are pure functions in `core/` (`TimeMap`, `LoopMap`) and can be unit-tested without any video. Output is always constant frame rate (CFR), and its length is always a whole number of frames.

### 8.1 Definitions

- `fps` = output frame rate (rational, e.g. 30000/1001), `s` = Speed, `tIn`, `tOut` = In/Out (seconds).
- Segment frame count `D = round((tOut − tIn) × fps / s)`, minimum 2 (3 for Ping-pong).
- Source time of segment frame `i`: `τ(i) = tIn + i × s / fps`. `S[i]` = the image at that time (per the FPS method, §8.3).
- `O[k]` = output frame `k`, `L` frames in total. The output loop is O\[0\], O\[1\], …, O\[L−1\], then O\[0\] again.

### 8.2 The four loop modes

| Mode | Output length L | Options |
| --- | --- | --- |
| Simple | D | — |
| Crossfade | D − F | F (seconds or frames, 1 ≤ F ≤ ⌊D/2⌋, default 0.5 s), Curve: Linear / Smooth |
| Ping-pong | 2D − 2 | — |
| Ping-pong + Smooth turn | 2D − 2 | T (turn frames, 1 ≤ T ≤ ⌊(D−1)/2⌋, default 0.3 s) |

**Simple:** `O[k] = S[k]`.

**Crossfade** (inside the selected range, the owner's decision): the output starts at `S[F]`. Over the last F frames, the end of the segment gradually blends into the first F frames of the segment, so `S[F]` follows the last frame naturally.

```latex
O[j] = \begin{cases} S[F+j] & 0 \le j < D-2F \\ (1-w_j)\,S[F+j] + w_j\,S[j-(D-2F)] & D-2F \le j < D-F \end{cases}
\qquad w_j = c\!\left(\frac{j-(D-2F)+1}{F+1}\right)
```

- `c(x) = x` (Linear) or `c(x) = x²(3 − 2x)` (Smooth).
- Check: the dominant part of `O[L−1]` is `S[F−1]`, and `O[0] = S[F]` — adjacent frames, so the seam is invisible.
- Blending happens in premultiplied RGBA (correct for sources with alpha too).

**Ping-pong:** `O[k] = S[k]` (k < D), `O[k] = S[2D − 2 − k]` (k ≥ D). The turn frames (`S[D−1]`, `S[0]`) appear once, never twice.

**Ping-pong + Smooth turn:** speed eases to zero near the turns and stays constant in the middle (trapezoid velocity). `a = T/(D−1)`; on the forward half `x = k/(D−1)`, on the backward half `x = (2D−2−k)/(D−1)`; position `p = (D−1) · e(x)/(1−a)`, where:

```latex
e(x) = \begin{cases} \dfrac{x^2}{2a} & x < a \\ x - \dfrac{a}{2} & a \le x \le 1-a \\ 1 - a - \dfrac{(1-x)^2}{2a} & x > 1-a \end{cases}
```

When `p` is fractional, the two neighbouring segment frames are blended (weight = the fraction).

### 8.3 Frame rate (the owner: "there will be a frame rate setting")

- **Choices:** Source (the source's `avg_frame_rate`; for VFR the nearest common value), 15, 23.976, 24, 25, 29.97, 30, 50, 59.94, 60, Custom (1–60, three decimals). Maximum 60.
- **Methods (how `S[i]` is made):**

| Method | Rule | Best for |
| --- | --- | --- |
| Nearest (Drop/Duplicate, default) | The source frame whose PTS is nearest to `τ(i)`; on a tie, the earlier one | Fast; lowering FPS |
| Frame Blend | The source frames before and after `τ(i)`, weighted by time | Smoother motion, slight blur |
| Motion Interpolation | A `minterpolate` pre-pass over the segment (`mi_mode=mci`, `mc_mode=aobmc`, `vsbmc=1`), result in the disk cache; with progress and cancel | Raising FPS, slow motion |

- In, Out, F and T always sit on the output FPS frame grid. When the FPS changes, the values in seconds stay the same and the frame counts are recalculated.
- With a VFR source, mapping is by PTS, never by frame index.

### 8.4 Speed

- 0.10× to 4.00×, step 0.05; default 1.00×. In/Out stay in source time; output length = (tOut − tIn)/s.
- With Speed < 1 and the Nearest method, a quiet hint appears next to the option: "Motion Interpolation gives smoother slow motion".

### 8.5 Loop panel

Rows: Mode, Crossfade (s and frames, linked), Curve, Turn (s), Speed, FPS method. Below them a quiet summary: `Loop 4.97 s · 149 frames @ 30 fps`. Invalid values (e.g. F > D/2) are clamped automatically and the field flashes yellow for a moment.

## 9. Preview window

The Preview window shows exactly what will be exported: crop, mask, feather, video transform and keyframes, loop mode, Speed, FPS and output size all applied. It uses the same `FrameRenderer` as export (AVADR-0001).

### 9.1 Window

- The **Preview** button in the top bar, or F5. A separate, non-modal, resizable window (default: output size + controls, up to 80% of the screen). Position and size are remembered.
- Only one Preview window at a time; pressing again brings it to the front. When anything changes in the main window, the Live preview updates by itself after 250 ms (debounce).
- Closing it fully releases its player, textures and its own cache (§3.7).

```text
+--------------------------------------------------------------+
| Live | Encoded    Background: [Checker v]   Zoom: [Fit v]     |
+--------------------------------------------------------------+
|                                                              |
|                 output frame (checker = transparent)         |
|                                                              |
+--------------------------------------------------------------+
| [>] [||]  |=========|==============o=========|  2.31 / 4.97 s |
|             seam ^                       [Around seam]       |
+--------------------------------------------------------------+
| 640x640 . 30 fps . 149 frames . VP9 alpha . ~1.8 MB          |
+--------------------------------------------------------------+
```

### 9.2 Controls

| Control | Behaviour |
| --- | --- |
| Play/Pause (Space) | Always loops: O\[0\] follows O\[L−1\] |
| Slider | 0 to L−1 frames, frame-accurate scrubbing; ←/→ = 1 frame |
| Seam marks | Where the loop joins (end/start of the slider); in Crossfade the fade part is an orange strip; in Ping-pong both turn points |
| Around seam | Plays only the last 2 s + the first 2 s, repeatedly (the whole loop if it is short) |
| Background | Checker (default), Black, White, Green `#00FF00`, Navy, Image… (put an OBS scene screenshot behind it) |
| Zoom | Fit, 50%, 100%, 200%; pixel-exact at 100% |
| Live / Encoded | See below |

### 9.3 Live and Encoded

- **Live:** instant rendering on the GPU. On the first pass, frames are stored in the cache (a thin bar under the slider shows the cached part). If it cannot keep real time, it plays slower rather than dropping frames and shows "Caching…"; once cached, it plays at the correct speed.
- **Encoded:** encodes a temporary WebM with the current Output and Quality settings in the background (with progress), then plays that file through the **libvpx decoder** (with alpha), as OBS will. This also shows compression artefacts. The actual file size is shown next to it. If settings change, the Encoded result says "Out of date" and does not re-encode by itself. The temp file is deleted when the window closes. If Export is pressed afterwards with the same settings, that encoded result is copied instead of re-encoding.
- The info row at the bottom: output W×H, FPS, frame count, duration, format, whether it has alpha, estimated or actual size.

## 10. Export pipeline and settings

The default output is a VP9 alpha WebM, which OBS's Media Source plays directly as a transparent loop. Export always runs in the background from a snapshot of the project; editing during an export does not change the running export.

### 10.1 Flow

1. **Export** button or Ctrl+E. The first time, a Save dialog: default name `<source>_loop_<W>x<H>.webm`, last used folder. Ctrl+Shift+E = add to the Batch queue.
2. Pipeline (bounded queues, §3.7): Decoder → `FrameRenderer` (GPU) → PBO readback → swscale `yuva420p` → Encoder → Muxer. Every output frame has `pts = k`, `time_base = 1/fps` (CFR).
3. Written to `<name>.part`; on success, an atomic rename (`MoveFileEx` + `REPLACE_EXISTING`). On cancel or error the `.part` file is deleted.
4. Progress in the status bar: `Exporting 86/149 · 58% · 41 fps · ETA 0:02` + Cancel.
5. **Verification (`OutputVerify`):** reopens the file and checks frame count = L, FPS, W×H, codec, alpha (decodes the first frame with libvpx and reads the alpha plane) and that the first frame is a keyframe. A mismatch gives an error toast and a log entry.
6. Success toast: `Exported 640×640 · 4.97 s · 1.8 MB` + Show in folder. The first time in a session, an OBS tip: "Media Source → Loop on · Use hardware decoding off (keeps transparency)", with Don't show again.

### 10.2 Output panel

| Row | Values |
| --- | --- |
| Format | WebM VP9 (default), WebM VP8, GIF, APNG, MP4 H.264, MOV ProRes 4444 |
| Size | Crop size (default), Long side: 1920 / 1280 / 1080 / 720 / 512 / 256, Custom W×H (with lock) |
| Alpha | Auto (default: only when there are transparent parts), On, Off |
| Background | Colour for transparent parts when Alpha is Off or the format has no alpha (default black) |
| FPS / FPS method | §8.3 |

- All sizes are even numbers, minimum 16, long side at most 1920. Past the limit, the size is scaled down keeping the ratio, with a small note next to the field.

### 10.3 Quality panel and encoder parameters

| Preset | VP9 | VP8 |
| --- | --- | --- |
| Low | `crf 40`, `cpu-used 4` | `crf 30`, `b:v 2M` |
| Medium (default) | `crf 31`, `cpu-used 2` | `crf 15`, `b:v 6M` |
| High | `crf 24`, `cpu-used 1` | `crf 6`, `b:v 12M` |
| Custom | CRF 4–63, Speed 0–8, Max bitrate (optional), Two-pass, Target size (MB) | Same |

- VP9 common: `-c:v libvpx-vp9 -pix_fmt yuva420p -b:v 0 -deadline good -row-mt 1 -threads <cores> -tile-columns <log2(W/256)> -g <L> -auto-alt-ref 1 -lag-in-frames 25`. With Alpha Off, `yuv420p`.
- VP8 common: `-c:v libvpx -pix_fmt yuva420p -auto-alt-ref 0` (required for VP8 alpha) `-deadline good -g <L>`.
- The first frame is always a keyframe; the whole loop is one GOP (L at most 9999). OBS decodes from the start on each loop, so this is enough for a smooth loop.
- No audio stream is ever written.
- **Target size:** two-pass, `b:v = target_bytes × 8 / duration × 0.9` (headroom for alpha and the container). If the result is more than 5% over the target, encode once more at a corrected bitrate.
- **Estimated size:** `SizeEstimator` quickly encodes 3 spread-out parts of the loop (10 frames each) and extrapolates; updates 1 s after a settings change; shown in the panel as `~1.8 MB`.

### 10.4 Other formats (item 13)

| Format | Encoder | Alpha | Note |
| --- | --- | --- | --- |
| GIF | `palettegen` (stats diff) + `paletteuse` (bayer dither) | 1-bit (50% threshold) | `-loop 0`; at most 50 fps |
| APNG | `apng` | Full | `-plays 0` |
| MP4 H.264 | `h264_mf` (Media Foundation, hardware when a GPU is present) | None; Background colour | `-movflags +faststart`, `yuv420p` |
| MOV ProRes 4444 | `prores_ks -profile:v 4444` | Full (`yuva444p10le`) | Large files, high quality |

### 10.5 Batch queue (item 13)

- Each entry = a project snapshot + an output path. Entries run one after another (only one at a time, for memory).
- Batch window: list (name, format, size, state, progress), drag to reorder, Start/Pause/Remove/Clear done. The queue is saved in `%APPDATA%\AvLoop\queue.json`; unfinished entries come back as Pending when the app reopens.
- If an entry fails, its reason is shown and the queue moves on to the next one.

## 11. Project file, Undo, settings, shortcuts and Loop Finder

### 11.1 `.avloop` project file (UTF-8 JSON)

```json
{
  "format": "avloop", "version": 1,
  "source": { "path": "D:/clips/fire.mp4", "relativePath": "../clips/fire.mp4",
              "size": 48213344, "mtime": "2026-10-26T10:12:00Z", "stream": 0 },
  "range": { "in": 12.400, "out": 17.400 },
  "transform": { "x": 960, "y": 540, "scale": 1.0, "flipH": false, "flipV": false,
                 "keyframes": [ { "t": 12.4, "x": 960, "y": 540, "scale": 1.0, "interp": "ease" } ] },
  "crop": { "kind": "ellipse", "x": 640, "y": 220, "w": 640, "h": 640,
            "ratio": "1:1", "radius": 0, "feather": 4 },
  "loop": { "mode": "crossfade", "fade": 0.5, "curve": "smooth", "turn": 0.3,
            "speed": 1.0, "fpsMethod": "nearest" },
  "output": { "format": "webm-vp9", "size": "crop", "w": 640, "h": 640,
              "alpha": "auto", "background": "#000000", "fps": "30/1" },
  "quality": { "preset": "medium", "crf": 31, "speed": 2, "maxBitrate": 0,
               "twoPass": false, "targetMB": 0 },
  "export": { "lastPath": "D:/obs/loops/fire_loop_640x640.webm" },
  "view": { "zoom": 0.5, "panX": 0, "panY": 0, "tool": "crop-ellipse" }
}
```

- All times in seconds (double); all geometry in scene px. Unknown keys are ignored on read but are not lost on save (kept in `extra`).
- Finding the source: `path` → `relativePath` (relative to the project file) → if not found, a "Locate source…" dialog. If size/mtime differ, a warning in the status bar.
- When `version` increases, `ProjectMigrate.cpp` reads older files; opening a newer file in an older app gives a clear message.
- Saving is atomic (`QSaveFile`). Window title: `AvLoop — <name>.avloop*` (`*` when unsaved). Before closing: Save / Don't save / Cancel.
- Autosave: every 2 minutes when there are changes, to `%LOCALAPPDATA%\AvLoop\autosave\`. After a crash, the next start offers "Recover unsaved project?".
- Recent: the last 10 projects and videos. Command line: `avloop.exe <video|project>` opens it directly.

### 11.2 Undo/Redo

- `QUndoStack`, at most 200 steps. Commands: SetCrop, SetTransform, SetRange, SetLoop, SetOutput, SetQuality, AddKeyframe, MoveKeyframe, DeleteKeyframe.
- One continuous drag, or repeated typing/wheel in the same field = one Undo step (`mergeWith`, 500 ms window).
- Zoom, pan, playhead and tool changes are not part of Undo.

### 11.3 Settings and presets

- `QSettings` INI: `%APPDATA%\AvLoop\AvLoop.ini`. Settings window (tabs): **General** (Theme, open last project at start, autosave minutes), **Performance** (Hardware decoding, GPU rendering, RAM cache MB, disk cache folder and GB, Clear cache), **Export** (default folder, name template `{source}_loop_{w}x{h}`, show OBS tips).
- Presets: Loop + Output + Quality saved together under a name (`%APPDATA%\AvLoop\presets\*.json`). Shipped presets: "OBS Overlay 1:1 512", "OBS Lower Third 16:9 1280", "OBS Vertical 9:16 1080". A preset dropdown + save/delete icons above the right panel.
- Logs: `%LOCALAPPDATA%\AvLoop\logs\avloop-YYYYMMDD.log`, last 7 days, each under 10 MB. Every line has a time + module; on a crash, a minidump (`MiniDumpWriteDump`).

### 11.4 Keyboard shortcuts

| Action | Shortcut |
| --- | --- |
| Open video / project | Ctrl+O |
| Save / Save As | Ctrl+S / Ctrl+Shift+S |
| Undo / Redo | Ctrl+Z / Ctrl+Y (Ctrl+Shift+Z) |
| Preview | F5 |
| Export / Add to queue | Ctrl+E / Ctrl+Shift+E |
| Tools | V, M, U, E, H, Z, K; I/O = set In/Out |
| Play/Pause, frame, jump | Space, ←/→, Shift+←/→, Home/End, Ctrl+Home/End |
| Loop playback | L |
| Previous/next keyframe | \[ / \] |
| Zoom | Ctrl+0 Fit, Ctrl+1 100%, Ctrl+= / Ctrl+- |
| Fit timeline | \\ / Shift+\\ |
| Loop Finder | Ctrl+L |
| Settings | Ctrl+, |
| Shortcut list | F1 (a small overlay) |

### 11.5 Auto Loop Finder (item 12)

- Menu → Loop Finder (Ctrl+L). Input: search range (default current In–Out ± 5 s, or the whole file), minimum and maximum length (default 2–10 s).
- Method: each frame's crop region is reduced to a 64×64 greyscale fingerprint (memory: 4 KB per frame). Score = mean absolute difference between frames a and b + motion difference (a−a−1 versus b−b−1). All pairs are scored with QtConcurrent, with progress and Cancel.
- Result: the best 5 pairs (no two within 1 s of each other), each with In/Out, length, score (0–100) and thumbnails of both ends. Click = set In/Out (one Undo step); double-click = set and open Preview.
- The fingerprints are released when the window closes.

## 12. Edge cases and error handling

Rule: the app never crashes, never hangs and never silently writes wrong output. Every error message says what happened and what to do, and the log gets the full detail.

### 12.1 Source video

| Case | Behaviour |
| --- | --- |
| Rotation metadata (phones, display matrix 90/180/270, flip) | `SourceNormalizer` corrects it right after decoding; the canvas shows it upright |
| Variable frame rate (VFR) | Mapped by PTS, CFR output; status bar shows `VFR → CFR` |
| HDR (PQ/HLG, BT.2020) | `zscale` + `tonemap=hable` → SDR BT.709; status bar shows `HDR → SDR` |
| Interlaced | `bwdif` deinterlace (frame mode) |
| Anamorphic (SAR ≠ 1) | Scaled to square pixels; display size in scene space |
| 10/12-bit, 4:2:2/4:4:4 | Internally RGBA8 (dithered from RGBA16F on the GPU) |
| Source has alpha | Alpha is kept and multiplied with the mask |
| Corrupt packets/frames | Skipped, the previous good frame is shown, counted in the log; a warning toast on export |
| No video stream (audio only) or unknown codec | "This file has no video AvLoop can decode (codec: X)" |
| Very short video (< 2 frames) or a single image | Opens, but Export is disabled with the reason shown |
| Very large file (> 10 GB, hours long) | Thumbnails for the visible part first; opening < 2 s |
| Network/USB drive, file moved away | On a read error, playback stops and a "Source unavailable" banner + Locate appear |

### 12.2 Settings and limits

- If In–Out is very short, F and T are clamped automatically (§8.2). With D < 2, Export/Preview are disabled with the reason shown.
- A crop of zero size or < 16 px: drawing is cancelled. A crop entirely outside the video: allowed, but the status bar says "Output is fully transparent" and Export asks for confirmation.
- Odd sizes go to the nearest even number (keeping the centre). With a 1920 maximum, no codec size limit is exceeded.
- Keyframes outside In–Out still affect interpolation (the usual After Effects behaviour).

### 12.3 Export and system

| Case | Behaviour |
| --- | --- |
| Disk full (export or cache) | Export stops, `.part` is deleted, the message says how much space is needed; the cache stops writing to disk and falls back to RAM |
| Output file already exists / locked by OBS | Overwrite confirmation in the Save dialog; if the rename fails, 5 retries 1 s apart, then offer `name (1).webm` |
| No write permission | Message + the Save dialog again |
| GPU lost (device lost, driver reset) | Context is recreated; if that fails, fall back to the CPU renderer and continue the running export from that frame |
| No OpenGL 3.3 (Remote Desktop, Basic Display) | CPU path from the start, one toast |
| App closed during an export | "Export in progress — cancel and quit?" |
| Several app instances | Allowed. The cache is in per-session folders, so no conflict; the queue file is locked |
| App crash | Minidump + autosave recovery at the next start; the old cache is deleted |
| HiDPI or moving the window between monitors | Icons and canvas stay sharp at the new DPI; a saved window position off-screen returns to the primary monitor |

## 13. Phase plan

14 items, two tracks, two checkpoints. A normal phase's ordinal = its item number; a checkpoint takes the free ordinal after the last item (the example's rule). Order is set by `depends_on`. The agent builds `phases/AV-NNN-*.md`, `control/01`, `requirements` and `PROGRESS.json` from this table.

```text
Track A — the loop maker (items 1-10)
AV-001 -> AV-002 -> AV-003 -> AV-004 -> AV-005 -> AV-006 -> AV-007 -> AV-008 -> AV-009 -> AV-010 -> AV-015 (CP-A1)
Track B — the extras (items 11-14)
AV-015 -> AV-011 -> AV-012 -> AV-013 -> AV-014 -> AV-016 (CP-B1)
```

### 13.1 Phase list

| Phase | Title | Item | Depends on | Risk | Budget | What it builds (doc sections) |
| --- | --- | --- | --- | --- | --- | --- |
| AV-001 | The ground | 1 | — | Medium | 600 | Repo, CMake, `fetch-deps.ps1`, maximized shell + layout, Theme, IconFactory + all icons, `--shot`/`--walk` skeleton, `avloop-cli` skeleton, phase tools, open-source files, CI, README v1 (§2, §4, §5) |
| AV-002 | The media | 2 | AV-001 | High | 600 | MediaProbe, Decoder + D3D11VA, SourceNormalizer, FrameCache + DiskCache, any-video import, `make-samples.mjs` (§3.6–3.9, §12.1) |
| AV-003 | The canvas and the Selection | 3 | AV-002 | High | 550 | GL canvas, scene space, Zoom/Hand, Selection + options bar, VideoTransform, Undo base (§3.3, §6.2, §6.3) |
| AV-004 | The crop tools | 4 | AV-003 | Medium | 550 | Rect/Rounded/Ellipse, ratios, handles, Feather, MaskBuilder (GPU + CPU), Crop panel (§6.1) |
| AV-005 | The timeline | 5 | AV-004 | High | 600 | Ruler, thumbnails, In/Out, transport, playback clock, I/O tools (§6.4, §7) |
| AV-006 | The loop engine | 6 | AV-005 | High | 500 | TimeMap, LoopMap (4 modes), FPS methods, Speed, Interpolator, Loop panel (§8) |
| AV-007 | The renderer | 7 | AV-006 | High | 550 | FrameRenderer (shared GPU path + CpuRenderer), blend, PBO, memory budgets, AVADR-0001 (§3.4, §3.6, §3.7) |
| AV-008 | The preview window | 8 | AV-007 | Medium | 500 | Live/Encoded, seam, background, Around seam (§9) |
| AV-009 | The export | 9 | AV-008 | High | 600 | VP9/VP8 alpha, Output/Quality panels, Target size, SizeEstimator, OutputVerify, progress/cancel, OBS tip (§10.1–10.3) |
| AV-010 | The project and the settings | 10 | AV-009 | Medium | 550 | `.avloop`, migration, autosave, Recent, Settings, presets, logs, shortcut overlay (§11.1–11.4) |
| AV-015 | **CP-A1 the loop maker walked** | 0 | AV-001 … AV-010 | High | 200 | Full e2e + memory + GPU + ASan + import walks, fresh README screenshots (§14) |
| AV-011 | The keyframes | 11 | AV-015 | Medium | 450 | K tool, keyframe row, Linear/Ease, interpolation in the renderer (§6.4) |
| AV-012 | The loop finder | 12 | AV-011 | Medium | 400 | Fingerprints, scoring, results window (§11.5) |
| AV-013 | The batch and the other formats | 13 | AV-012 | Medium | 550 | Batch queue, GIF, APNG, MP4 (`h264_mf`), ProRes 4444 (§10.4, §10.5) |
| AV-014 | The installer | 14 | AV-013 | Medium | 300 | Inno Setup, `windeployqt`, `.avloop` file association, Start menu, uninstaller, third-party notices |
| AV-016 | **CP-B1 the whole walked** | 0 | AV-011 … AV-014, AV-015 | High | 200 | Every situation on one fresh release build, installed from the installer; GitHub Release `v1.0.0` + installer (R-5); final README |

- Each phase file's **Done when** names the matching situations from §14; **Seam if this splits** says where to split (e.g. AV-002 → AV-002a: disk cache and D3D11VA).
- Reserved D-numbers: D-001 … D-014 (one per item). ADRs: AVADR-0001 (one renderer, GPU/CPU parity; accepted by AV-007), AVADR-0002 (memory budgets and caches; accepted by AV-002).
- A later wish from the owner becomes a new item, and the checkpoint moves to the next free ordinal (`control/01`, *an item added*).

### 13.2 Readings (the agent's decisions, open to the owner's veto later)

| R | Reading |
| --- | --- |
| R-1 | UI language is English; all strings go through `tr()`, so a Bengali translation can be added later |
| R-2 | Unlocking Selection's W/H never changes the video's ratio (distortion was not asked for) |
| R-3 | Space = Play/Pause; pan = middle mouse or H |
| R-4 | Licence MIT (public, open to everyone's contributions) |
| R-5 | At CP-B1, a GitHub Release `v1.0.0` is created with the installer, so the README's Download link works |
| R-6 | Default output: VP9, Medium, Alpha Auto, Crop size, Source FPS (maximum 60) |
| R-7 | Blending in premultiplied sRGB (not linear light), matching how OBS composites |
| R-8 | One crop at a time; with no crop, the whole video |

## 14. Acceptance criteria

This section goes into `control/02_MAJOR_CHECKPOINTS.md` as is. Every result is written in the execution record with its observed value; every command runs with a 5-minute timeout.

### 14.1 Gates (§1)

| Gate | Command / measure | When |
| --- | --- | --- |
| **Line limit** | `node control/phase-tools/check-loc.mjs` PASS: no code file > 600 | Every commit |
| Build | `scripts\build.ps1` (Release), zero warnings | Every phase |
| Unit | `ctest --test-dir build -C Release --output-on-failure --timeout 300`, all pass | Every phase |
| Form | `scripts\format.ps1 -Check` (clang-format), `node tools/icons/check-icons.mjs` | Every phase |
| Secrets | `node tools/check-secrets.mjs`: no tokens, emails or personal paths | Every commit |
| Licence | The bundled FFmpeg DLLs have no `--enable-gpl` (checked by reading `avcodec_configuration()`) | AV-001, checkpoints |
| Memory | The §3.7 walk: Private Bytes growth < 50 MB, VRAM < 32 MB; zero Debug leaks | AV-002, AV-007, AV-008, AV-009, checkpoints |
| ASan | All unit tests in a `/fsanitize=address` build | Checkpoints |
| GPU parity | Same project GPU vs CPU: mean difference ≤ 1.0, maximum ≤ 4 | From AV-007 |
| CI | GitHub Actions green (`gh run watch`) | Every push |
| README | Screenshots are of the current UI (the phase's `--walk readme-shots`); the feature list is true | Every phase that changes the UI |
| Phase system | `validate-phase-system.mjs` PASS, `check-continuity.mjs` | Every phase |

### 14.2 Situations (§2)

| S | Situation | What must happen |
| --- | --- | --- |
| S1 | The app | Second start < 2 s; maximized; screenshots of the Navy and Black themes at 100/150/200% DPI; all icons visible; only Open/Preview/Export buttons in the top bar |
| S2 | Import | Every file from `make-samples` opens; rotation, SAR, interlacing and HDR come out upright/correct (checked on first/middle/last frame PNGs); a Bengali path works; a broken file gives a message, not a crash; HW decode on and off give the same frames |
| S3 | Seek | Correct frame over 200 random seeks (testsrc2 frame number); 1080p H.264 at 30/60 fps plays 10 s with 0 dropped frames |
| S4 | Selection | Moving/scaling the video, Fit/Fill/Center/Flip; options bar values match; upscale warning; the crop stays still |
| S5 | Crop | All three shapes, every ratio, Shift/Alt, snapping, arrows; in the export, Ellipse corner pixels have alpha = 0 and the centre alpha = 255; with Feather, the edge steps down gradually |
| S6 | Timeline | In/Out via the tool, the shortcut and dragging; dragging the crossfade band changes F; no label overlap at any zoom |
| S7 | Loop modes | In all four modes the exported frame count = L from §8.2; `seamDiff` (last→first frame difference) ≤ 1.5 × the median difference between neighbouring frames; no duplicate frame at Ping-pong turns |
| S8 | FPS and Speed | 15/24/30/60 and Custom 12; all three methods; `r_frame_rate` correct in ffprobe, CFR; correct length at Speed 0.5× and 2× |
| S9 | Preview | Shows only the exported part; Live frame N = exported frame N (mean difference ≤ 1.0, excluding compression); Encoded keeps alpha; memory returns on close |
| S10 | Export | ffprobe: `codec_name=vp9`, W×H, `alpha_mode=1` (when Alpha is On), no audio stream, first frame is a keyframe; alpha plane present via the libvpx decoder; no `.part` after Cancel; Target size within ±5%; VP8 the same |
| S11 | OBS compatibility | The exported file read twice in a loop through FFmpeg's libvpx decoder (`-stream_loop 1`) gives 2L frames and no error at the seam; if OBS is installed, play it in a Media Source via obs-websocket and confirm transparency in a screenshot |
| S12 | Project | Save → open gives the same render (identical pixels); moving the source triggers relativePath/Locate; Undo/Redo for every command; autosave recovery after a crash |
| S13 | Memory and GPU | The §3.7 and §3.6 walks; the whole set again with the GPU off |
| S14 | Keyframes, Loop Finder, Batch, formats | Positions between keyframes as expected; on a known loop (a generated sample repeating every 4 s) the Finder ranks it first (±2 frames); a 3-entry Batch; GIF/APNG/MP4/ProRes correct in ffprobe |
| S15 | Installer | Silent install (`/VERYSILENT`), launch, `.avloop` opens on double-click, uninstall leaves nothing behind (except settings) |

No walk leaves a test process, temp file or cache behind; screenshots and logs stay in `execution/AV-NNN-walk/`, each with a description of what it shows.
