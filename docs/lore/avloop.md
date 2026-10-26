# AvLoop — things that bite

The standing rules for traps found while building AvLoop. Each entry: the trap, what it does, the rule. Newest at the
end of its section.

## Build and tools

- **PowerShell 5.1 takes a native tool's stderr for an error** when `$ErrorActionPreference = 'Stop'` (vcvars64,
  cmake, windeployqt all write notes to stderr). Rule: native calls go through `Invoke-Native` in `scripts/build.ps1`
  (local `Continue`, the exit code decides). (AV-001)
- **vcvars64.bat looks for `vswhere.exe` on PATH and complains**; harmless, but it must be silenced (`>nul 2>&1`)
  inside `cmd /c`. (AV-001)
- **A `.ico` cannot be made by Qt's image writer with several sizes.** Rule: `tools/icons/iconbake.cpp` renders the
  SVG and writes a PNG-entry `.ico` itself; `resources/app/avloop.ico` is committed and checked by `check-icons.mjs`;
  the `.rc` is added only when the `.ico` exists (a fresh configure after it appears). (AV-001)
- **A GUI-subsystem exe started from PowerShell without a pipe is not waited for.** Rule: walks run the app from Node
  (`spawnSync`), or pipe its output. (AV-001)

## Qt

- **A `QToolButton`'s own shortcut animates a click** (100 ms) before the button toggles; a walk's next step reads the
  old tool. Rule: tool keys are window `QAction`s that call `selectTool` at once. (AV-001)
- **A token written in a QSS comment is a token**: the theme's expansion leaves `@name` in a comment unresolved and the
  test fails. Rule: QSS comments never contain an at-sign. (AV-001)
- **`QMenu::exec` runs its own event loop**: a walk that presses the ☰ button blocks inside the press. Rule: the next
  menu press is armed by a timer *before* the button is pressed (`WalkRunner::stepMenu`). (AV-001)
- **Thin strokes at 20 px have no fully opaque pixel**: a 1.75-unit line at 20/24 scale covers two pixel rows at ~73%.
  Rule: tests that check an icon is drawn look for alpha above 100, not 200. (AV-001)

## Media (FFmpeg)

- **MPEG-TS and PS seek by a timestamp search that can land after the first keyframe**: a "seek to the start" put the
  read at byte 12596 of a 2 s file, after its only IDR, and nothing decoded. Rule: a rewind of an MPEG transport or
  program stream is a byte seek to 0; after every seek decoding waits for a keyframe packet. (AV-002)
- **"No keyframe after the seek point" is not "the container flags no keyframes"**: concluding the latter after a seek
  mid-way into a long GOP turned the wait off and nothing decoded. Rule: keyframes count as unflagged only when a
  rewind to the start found none. (AV-002)
- **FFmpeg 7.1's filters negotiate colour space and range**: a buffer source made without them gets "Changing video
  frame properties on the fly", and zscale "no path between colorspaces". Rule: `av_buffersrc_parameters_set` with
  the first frame's `colorspace` and `color_range`. (AV-002)
- **bwdif (and yadif) halve the output time base** even in `send_frame` mode: the output's pts are twice the input's.
  Rule: compare in the source's time base (`av_rescale_q` from `av_buffersink_get_time_base`). (AV-002)
- **No FFmpeg decodes animated WebP** (7.1 and 8.1 both: "image data not found", width 0). Rule: the probe says so in
  plain words (reading R-15); the sample stays in the set as a refusal case. (AV-002)

## The lab

- **Qt's log goes to the debugger, not stderr, when the process has no console window** (a pipe from Git Bash or Node).
  Rule: `QT_FORCE_STDERR_LOGGING=1` when reading a binary's log in the lab. (AV-002)
- **A Python or shell heredoc rewrites backslashes and `${…}`** in the C++ or JavaScript it carries (a `\n` became a
  real newline in a string literal). Rule: edits that carry backslashes or template literals go through the Edit or
  Write tool. (AV-002)
- **A memory walk compares like with like**: Private Bytes and VRAM swing with the video open (D3D11 surfaces); the
  growth is the later rounds' peaks against the first round's, not a peak against an arbitrary moment. (AV-002)

## The canvas (Qt and OpenGL)

- **`QWidget::grab()` leaves out what QPainter drew on a QOpenGLWidget**: the GL clear and draws are in the picture,
  the painter's overlay is not (on screen it shows; `grabFramebuffer()` has it). Rule: walks and `--shot` take pictures
  through `WalkRunner::grabWidget`, which draws each visible QOpenGLWidget's own framebuffer in again. (AV-003)
- **A new Qt module needs its DLL in build\bin**: windeployqt ran once at AV-001 and the OpenGL modules were missing —
  the app exited with 0xC0000135 and no message. Rule: `scripts\build.ps1` runs windeployqt after every build. (AV-003)
- **Python's text mode writes CRLF on Windows**: files edited by a Python script turned CRLF and multi-line edits by
  pattern stopped matching. Rule: scripted edits go through Node (`fs.writeFileSync` keeps LF) or the Edit tool;
  `.gitattributes` stores LF. (AV-003)
- **QUndoStack lives in Qt Gui in Qt 6**: the core links Qt Gui for it alone (reading R-17); no other Gui class enters
  `src/core`. (AV-003)
- **A walk must never take the system's focus**: it runs on the owner's desktop while the owner works; a walk that
  raised its window caught the owner's real keystrokes ("I" switched the tool to In), and a plain `showMaximized()`
  took the foreground in 2 of 3 lab starts. Rule: lab windows show without activating (`WA_ShowWithoutActivating`),
  are sized by `SetWindowPos(SWP_NOACTIVATE)` (`WalkRunner::placeQuietly`: the work area for "maximized"), key and
  menu steps set only Qt's own active window, and every walk ends with the check "the owner's focus untouched". No
  exploratory loops of window launches either: each one lands on the owner's screen. (AV-003)
- **Windows' resize borders are invisible**: a window filling the work area reports a frame 8 px inside it on each
  side. Rule: "covers the work area" allows 12 px. (AV-003)
- **windows.h defines `near` and `far`** (empty macros): a variable named `near` stopped compiling the moment a file
  included it. Rule: no identifiers `near`, `far`; `NOMINMAX` is already set. (AV-003)
- **A drag in scene px lands on whole view pixels**: at a fit zoom of 1.62 a 100 px scene move came out 99.4. Rule:
  canvas expectations allow one view pixel (±1 scene px at fit zooms). (AV-003)

## The crop

- **A mask is sampled at pixel centres**: for a shape of whole-pixel size the edge falls between pixels on the axes,
  so a row through the centre has no partly covered pixel at all and "count the grey pixels on a row" tests nothing.
  Rule: test the anti-aliased band geometrically — every partly covered pixel within half a pixel of the true edge.
  (AV-004)
- **Every tool's options row exists at once**: the options bar keeps all rows and shows one, so a walk probe named
  once per tool kind is overwritten by the last row made. Rule: an options probe carries its row's name
  (`options.rect.w`, `options.ellipse.feather`); a probe to a widget that a later change may delete holds a
  `QPointer` (`probeValue`). (AV-004)
- **`class QComboBox* f();` inside `namespace avloop` declares `avloop::QComboBox`**: the build then fails far away,
  where the real Qt class meets the invented one. Rule: forward declarations of Qt classes go at global scope, before
  the namespace. (AV-004)
- **A Qt Test executable's output can vanish in these shells**: a failing test printed nothing and exited 2. Rule:
  run it with `-o <file>,txt` and read the file. (AV-004)
- **A custom ratio taken from a crop's size can be large** (180 × 320): fields capped at 100 clipped it. Rule: Custom…
  reduces the crop's size to lowest terms (9:16), and the W:H fields take up to 9999. (AV-004)
