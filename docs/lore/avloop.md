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

## The timeline and playback

- **Ninja can keep a stale object**: after a virtual was added to `Tool`, three tool objects were not recompiled (the
  deps log held an out-of-date record) and a call to `previewCrop()` landed on `createOptions()` of the old vtable —
  a QWidget made with a garbage parent, a crash in the canvas's paint. Rule: a crash that makes no sense after a
  header change → delete `build\**\*.obj` and build again; CI always builds clean. (AV-005)
- **swscale's `accurate_rnd` turns its SIMD off**: 10 ms a 1080p frame against 0.7 ms — 60 fps playback dropped
  frames on it. Rule: `bicubic+full_chroma_int` without `accurate_rnd` (rounding within 1/255). (AV-005)
- **Next to a QOpenGLWidget every raster repaint costs a whole-window composition** (Qt copies the backing store and
  composes it with the GL texture, 4–6 ms a flush, whatever the dirty area): a timeline repainted at every frame of a
  60 fps video put the UI thread over 16.7 ms. Rule: during playback the timeline and the time field follow at most
  30 times a second; widgets that fill themselves set `WA_OpaquePaintEvent`; the clock shows frames in order and
  drops only a frame more than two frames late. (AV-005)
- **A GUI build's `qInfo` does not reach the console**: diagnostics for a walk go to `stdout` with `fprintf` and are
  taken out again. (AV-005)
- **A click on a frame boundary lands a pixel early half the time**: the walk's points are view pixels. Rule: timeline
  clicks in walks aim at the middle of a frame. (AV-005)
- **Keys sent to a widget must have it focused**: the window's shortcuts ask the focus widget first (ShortcutOverride).
  Rule: the walk's `key` step focuses its target; a tool that wants a key the window also binds (the crop's arrows)
  says so through `Tool::wantsKey`. A time field gives the focus back after Enter, so I and O work again. (AV-005)
- **`--mem-log`'s first column is milliseconds**, not seconds. (AV-005)
- **A full-screen lab window catches the owner's clicks**: in a minute-long walk the owner's click anywhere on the
  screen landed on it and made it the foreground window. Rule: lab windows are placed behind the owner's windows
  (`HWND_BOTTOM`) with `WS_EX_NOACTIVATE`, so no click can activate them; pictures come from the widgets' own
  rendering, not the screen. (AV-005)

## The loop engine

- **`minterpolate` makes an output frame only once it holds the input after it**: the pre-pass stopped one frame
  short. Rule: feed a few source frames past the segment's end, then flush. (AV-006)
- **`minterpolate` works in YUV**: RGBA in would be converted and lose its alpha. Rule: colour through
  `format=yuv444p`, alpha through `alphaextract` and a second `minterpolate`, joined by `alphamerge`. (AV-006)
- **A combo box's size hint is its longest item**: "Ping-pong + Smooth turn" widened the side panel past its width
  and clipped every row. Rule: `Section::addRow` gives combo boxes `AdjustToMinimumContentsLengthWithIcon`; paired
  fields take 44 px at least. (AV-006)
- **Crossfade's last frame is mostly S[F − 1]** — exactly half when F is 1, and O[0] is itself blended when D = 2F:
  tests of §8.2 allow both. (AV-006)

## The renderer

- **One pass over the output, not a quad per layer**: a quad leaves the half pixel at the video's edge to the
  rasteriser, where the CPU twin samples the transparent border — the two disagreed on every edge. Rule: both back
  ends map each output pixel back to texel space by the same affine. (AV-007)
- **A texture pool sized by bytes alone filled 512 MB of frames that never came back** — 1.1 GB of Private Bytes at
  1080p. Rule: the pool keeps the last two frames' layers; the byte budget is only the ceiling. (AV-007)
- **GL needs a QGuiApplication and a surface made on the GUI thread**: the CLI makes a GUI application only for the
  commands that render; `GlRenderer` makes its `QOffscreenSurface` in its constructor, so it is constructed there.
  (AV-007)
- **windeployqt deploys what the exe it is given imports**: the CLI's new Qt6Concurrent import was missing beside it
  and it would not start. Rule: `scripts\build.ps1` runs windeployqt for every program. (AV-007)
- **A walk's 295 s timeout kills the Node script, not the program it started**: an over-long leg left avloop-cli
  running. Rule: size a walk's legs well under the limit (the memory walk renders the 640p sample). (AV-007)
- **Without `accurate_rnd`, swscale converts NV12 by a coarser chroma path than yuv420p**: hardware-decoded frames
  came out up to 180 apart from software-decoded ones at colour edges (AV-005's change; AV-002's walk caught it at
  AV-007). Rule: the normalizer makes NV12/P010 planar first, so both decoders' frames take one path. Regression runs
  replay every earlier walk, scripts included. (AV-007)
- **In AvLoop's process the GPU driver's memory grows with every thread started**: an NVIDIA OpenGL arena (its shader
  cache paths and extension strings fill it) took ~60 KB a thread and never gave it back — ~4 MB a video opened, as
  decoders, filter graphs and workers each started threads. A test process (no window composed by GL) shows none of
  it; only the whole app does. Found by `cdb -pv` snapshots: the heaps stayed level while one VirtualAlloc region grew.
  Rule: open a video without starting threads — one D3D11 device for every decoder, filter graphs on one thread, the
  decode and thumbnail workers on threads that live with the window, the thumbnail decoder single-threaded. (AV-007)
- **A memory check compares the floor, not the last sample**: Private Bytes after a burst swings with the heap's
  timing. Rule: the late rounds' minimum against the early rounds' maximum (test-memory `decodersComeAndGo`). (AV-007)

## The Preview and the encoder

- **A QOpenGLContext belongs to the thread that made it**: made with the GlRenderer on the GUI thread, it could not be
  made current on the render thread ("Cannot make QOpenGLContext current in a different thread", a fail-fast). Rule:
  `GlRenderer::init()` makes its context and program on the thread that renders; only the offscreen surface is made
  on the GUI thread. (AV-008)
- **VP8 cannot say BT.709**: frames written in BT.709 came back 14–16 levels off, as decoders read VP8 as BT.601. Rule:
  VP9 in BT.709, VP8 in BT.601, both tagged. (AV-008)
- **WebM keeps alpha only when the stream is marked**: the Matroska muxer writes AlphaMode (and the alpha in
  BlockAdditions) when the stream's metadata has `alpha_mode=1`; the colour planes are straight, so premultiplied
  frames are made straight first. (AV-008)
- **Keys typed into a second window need it as Qt's active window**: the walk's key step made the main window active,
  so the Preview's window shortcuts never fired. Rule: the key step activates (in Qt only) the window of its target.
  (AV-008)
- **A loop's wrap to an In between keyframes costs a GOP of decoding**: the editor's playback keeps the lap's first six
  frames and starts each lap with them while the decoder seeks behind them. (AV-008)

## The export

- **A GL call off the render thread crashes, not errors**: the export thread asked the FrameRenderer for its
  `backendName()`, which reads `glGetString` — with no current context there, an access violation. Rule: what needs the
  context is read on the render thread (the GPU's name at open) and passed over. (AV-009)
- **libvpx's two-pass VBR undershoots a target the loop does not need**: an easy loop asked for 3× its CRF size came
  out 20–40 % small, and VP8 saturates; `minrate` pushes it over at low rates instead. Rule: plain VBR, one corrected
  pass when more than 5 % off, and a reachable target in the tests. (AV-009)
- **An estimate is an export in small**: measured at every video opening it doubled the opening's memory (a 4K source:
  ≈ 400 → 1,000 MB, its decoders, textures and a 1080p VP9 encoder). Rule: measure after a change, a frame at a time,
  with no more look-ahead than the frames encoded. (AV-009)
- **A wrapped QLabel beside buttons asks for too little width**: the toast clipped its second line under its buttons.
  Rule: the text above, the buttons in a row beneath, the label fixed to its longest line (at most 340 px). (AV-009)

## The project and the settings

- **A QOpenGLWidget's grab paints over what floats above it**: the walk's window picture lost the F1 list over the
  canvas (the toast over the raster timeline survived). Rule: the picture draws the GL frames, then renders the
  floating widgets (toast, F1 list) on top again. (AV-010)
- **A probe name has one owner**: the window's old `window.title` (registered later) replaced the project's, which
  shows the `*`. Rule: a probe is registered once, where its meaning lives. (AV-010)
- **An unhandled-exception filter that returns EXECUTE_HANDLER ends a crash quietly**: the minidump is written and the
  process exits with 0xC0000005, no Windows Error Reporting dialog — so a walk can crash the app on purpose. (AV-010)
- **A walk answers what a user would be asked**: modal questions (Save / Don't save / Cancel, Recover / Discard, a
  name, Locate source…) take the walk's `answer` in a lab run instead of opening, so the focus stays the owner's.
  (AV-010)

## The checkpoint (CP-A1)

- **A QObject dies on the thread that owns it**: the render service destroyed its GlRenderer on the render thread, and
  with it the QOffscreenSurface the GUI thread made — Qt asserts in Debug ("Cannot send events to objects owned by a
  different thread"), Release stays silent. Rule: `GlRenderer::shutdown()` frees the GL objects and the context on the
  render thread; the surface goes with the object on the GUI thread. (AV-015)
- **A finished job's textures stay in the shared GPU context**: after the first export the app at rest kept ~70 MB
  more VRAM. Rule: `RenderService::trim()` after an export, an estimate or a Preview. (AV-015)
- **Under AddressSanitizer, Private Bytes only grows**: freed memory waits in ASan's quarantine, so the process-memory
  thresholds fail for ASan's reasons. Rule: an ASan run judges memory errors; the growth gates are judged in Release.
  (AV-015)
- **An exit-time CRT leak report lists Qt's process-lifetime allocations**: the Debug gate counts the debug heap's live
  blocks around repeated cycles instead (test-leaks: no block left by any cycle). (AV-015)
- **A verdict is a line of its own**: walk-phase read "WALK PASSED" quoted inside a failed walk's check and passed it.
  Rule: the verdict is matched at the start of a line, with the exit code. (AV-015)
- **In Bash, a double-quoted `node -e` runs backticks**: a comment with `` `node walk-memory.mjs start` `` was executed
  as a command substitution. Rule: scripts with backticks or backslashes are written with the Write tool. (AV-015)

## The keyframes

- **"The transform" became a value at a time**: with keyframes the project's transform is the one at the playhead, so
  the canvas, the options bar and the Selection kept working unchanged; the playhead moving is its own aspect
  (`Aspect::Time`), which the export, the Preview and the panels ignore. (AV-011)
- **A playhead move must be said even where the transform does not change**: between two equal keyframes the value
  stays, but the keyframe at the playhead changes — the options' Delete stayed disabled. Rule: with keyframes, every
  playhead move notifies. (AV-011)
- **The options row prefixes its names**: `OptionsRow::addIcon("keyframe.add")` is `options.keyframe.add`; a name given
  with the prefix doubles it. (AV-011)
- **A walk mapper must exist in the binary that runs the walk**: an unknown `space` falls back to raw pixels — [4, 0]
  became a ruler click at time 0. Rebuild before walking new steps. (AV-011)

## The Loop Finder

- **A log file open in a folder keeps the folder**: every lab run since the log came in left its `avloop-lab-*` folder
  in the temp folder (134 of them), and a message of the teardown could reach the file's statics after they were gone
  (an exit crash once). Rule: the log is closed and Qt's handler put back before `main` returns; walk-phase fails a
  walk that leaves a lab folder. (AV-012)
- **A perfectly periodic clip ties its multiples**: a 4 s loop scores exactly as well as the 8 s one. Rule: at a tie the
  shorter loop wins, then the earlier. (AV-012)
- **A cancel test needs a search long enough to cancel**: a 20 s search ends in 0.3 s; the walk asks for loops up to
  30 s before pressing Cancel. (AV-012)

## The batch and the other formats

- **A walk reads the CLI's PASS line**: AV-015's GPU-off walk wants `, CPU` at its end; the encoder's name added after
  the renderer failed it. Rule: a new field of a line a walk reads goes before the ones walks match at the end. (AV-013)
- **PowerShell 5.1 rewrites a file it touches**: `Get-Content -Raw` reads UTF-8 without a BOM as ANSI and
  `Set-Content -Encoding utf8` writes a BOM — every § and → in MainWindow.cpp came back mangled. Rule: files are
  edited with Node or the Edit tool, never round-tripped through PowerShell. (AV-013)
- **A Qt test run by hand needs its plugins**: without QT_PLUGIN_PATH the offscreen platform is not found and the
  executable waits on Qt's error box. Rule: QT_PLUGIN_PATH and QT_QPA_PLATFORM=offscreen, or ctest. (AV-013)
- **A batch walk writes into the lab run's own folder**: entries written into the walk folder would be committed, and
  Ctrl+Shift+E's default folder is the owner's Videos. Rule: `batchAdd` takes `"temp": true` (the lab data folder,
  gone with the run), and a lab run's default export folder is its data folder. (AV-013)
- **A probe that shows an error shows its path**: the batch's failure reason named the temp folder (and the owner).
  Rule: a probe that may carry a path replaces the temp and home folders. (AV-013)
- **A menu makes Qt think the application active**: after a walk's menu step Qt's state was Active though Windows
  never activated AvLoop; making the Settings dialog Qt's active window then asked the OS to activate it, and closing
  it activated the main window for real — the README run took the owner's focus. Rule: before each step, while
  another process holds the foreground, the walk runner tells Qt the application is inactive; it also logs the step
  after which the focus came, if it ever does. (AV-013)
- **At exit the render thread may hold a size estimate's frame**: members go in reverse order, the export thread's
  context before the render service, and an estimate's last render task posted to the deleted context (a crash after
  WALK PASSED). Rule: `~MainWindow` drops the batch, the export and the estimate, then drains render → export →
  render before any thread goes. (AV-013)

## The installer

- **The disk cache is not where the walks looked for it**: Qt's GenericCacheLocation is `%LOCALAPPDATA%\cache`, so the
  cache lives in `%LOCALAPPDATA%\cache\AvLoop\cache`; av-lab looked in `%LOCALAPPDATA%\AvLoop\cache` and its "no
  session left" check never saw a session. Rule: the lab reads the path DiskCache::defaultRoot() makes; the
  uninstaller removes that folder. (AV-014)
- **windeployqt deploys what Qt might load, not what the app links**: the TUIO touch plugin brought Qt6Network and its
  TLS plugins into an app that never uses the network. Rule: deploy with `--no-network` and skip those plugin types;
  the install walk fails a deployment with Qt6Network. (AV-014)
- **PowerShell reads `"$name:"` as a scoped variable**: `"... in $bin: run ..."` is a parse error. Rule: `${bin}:`.
  (AV-014)
- **An install walk changes the owner's machine**: it installs per user into a temporary folder, reads and runs what the
  registry holds, and uninstalls; the owner's AvLoop folders (which uninstalling removes) are copied aside first and
  put back, and a marker proves the settings folder stays. (AV-014)

## The checkpoint (CP-B1)

- **A peak shorter than the sampling interval can be missed**: the memory walk compared each export's peak with one
  sample a second; an export lasts about 2 s and its GPU peak under one, so the 2nd export's peak (372 MB, as the 3rd's)
  fell between samples and read 309 MB — a false "+63 MB". Rule: a walk that compares peaks samples faster than the
  peak lasts (`--mem-log-ms 200`); the limits stay. (AV-016)
- **The installed app is the one walked**: `AVLOOP_BIN_DIR` points every walk at an installed AvLoop instead of
  `build/bin`; a lab run of it keeps its data in its temp folder as the build's does. (AV-016)
