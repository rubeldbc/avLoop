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
