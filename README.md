<p align="center">
  <img src="resources/app/avloop.svg" width="96" alt="AvLoop logo">
</p>

<h1 align="center">AvLoop</h1>

<p align="center"><b>Cut any video into a seamless, transparent loop for OBS Studio.</b></p>

<p align="center">
  <a href="https://github.com/rubeldbc/avLoop/actions/workflows/ci.yml"><img src="https://github.com/rubeldbc/avLoop/actions/workflows/ci.yml/badge.svg" alt="CI"></a>
  <a href="LICENSE"><img src="https://img.shields.io/badge/license-MIT-orange.svg" alt="License: MIT"></a>
  <img src="https://img.shields.io/badge/platform-Windows-0E1726.svg" alt="Platform: Windows">
</p>

![AvLoop's main window](docs/screenshots/main-window.png)

AvLoop is a Windows desktop app (C++20, Qt 6, FFmpeg). You choose a time range (In to Out) and a screen area (the
crop — a rectangle, a rounded rectangle or an ellipse) of any video, and AvLoop turns it into a **seamless looping
WebM with transparency**, sized for OBS's Media Source — up to 1080p, with crossfade or ping-pong loops, adjustable
frame rate and quality, and a preview that shows exactly what will be exported.

> **Status: early development (v0.1).** AvLoop is built in public, phase by phase
> ([the plan](phases/26-10-26-034934-AvLoop/README.md)). The feature list below says only what works today; everything
> else is on the [roadmap](#roadmap).

## Features — working today

- A compact window that opens maximized: the top bar (Open, Preview, Export only), the tool rail, the canvas with a
  drop zone, the side panel's Crop / Loop / Output / Quality sections (collapsible, remembered), the timeline's
  transport row and the status bar.
- **Navy** (default) and **Black** themes in orange — switch from the ☰ menu without a restart.
- The app draws its own icons; they stay sharp at any display scale (checked at 100%, 150% and 200%).
- Tools switch by their keys: **V** Selection, **M** / **U** / **E** crop shapes, **H** Hand, **Z** Zoom, **I** / **O**
  In / Out, **K** Keyframe.
- `avloop-cli version` shows the FFmpeg build in use and refuses a GPL build.

## Roadmap

Built in this order (each line is a phase of [the plan](phases/26-10-26-034934-AvLoop/control/01_PHASE_DEPENDENCY_GRAPH.md)):

1. **Open any video** by its content — MP4, MOV, MKV, WebM, AVI, WMV, FLV, MPEG-TS, MXF, GIF/APNG/WebP and more; H.264,
   HEVC, AV1, VP8/VP9 (with alpha), ProRes 4444 … — frame-accurate, GPU-decoded (D3D11VA), rotation/HDR/interlacing
   corrected, within a fixed memory budget.
2. **Canvas and Selection**: zoom, pan, move and scale the video behind the crop frame.
3. **Crop tools**: rectangle, rounded rectangle, ellipse; aspect ratios (1:1, 16:9, 9:16, 4:5, custom …); feather.
4. **Timeline**: thumbnails, In/Out, frame-accurate playback.
5. **Loop engine**: Simple, Crossfade (inside the range), Ping-pong, Ping-pong with a smooth turn; Speed; frame rate with
   Nearest, Frame Blend or Motion Interpolation.
6. **One renderer** for preview and export, on the GPU with a CPU fallback of the same result.
7. **Preview window**: Live and Encoded, the seam marked, Around seam, backgrounds.
8. **Export**: VP9/VP8 alpha WebM, quality presets, target file size, verification.
9. **Projects** (`.avloop`), Undo/Redo, presets, autosave, settings.
10. Keyframes, the **Loop Finder**, a batch queue, GIF/APNG/MP4/ProRes 4444, and a Windows installer (v1.0.0).

## Using a loop in OBS

1. In OBS, add a **Media Source** and choose the exported `.webm` as the *Local File*.
2. Turn **Loop** on.
3. Turn **Use hardware decoding when available** off — OBS's hardware decoder drops the WebM's transparency.

## Download and install

The installer arrives with v1.0.0 on the [Releases](https://github.com/rubeldbc/avLoop/releases) page. Until then,
build from source.

## Build from source

Windows 10/11 x64, Visual Studio 2022 (C++), Node.js 20+ and Qt 6.8 (`msvc2022_64`):

```powershell
$env:QTDIR = 'C:\Qt\6.8.3\msvc2022_64'   # where Qt 6.8 is installed
scripts\fetch-deps.ps1                   # FFmpeg 7.1, LGPL shared build
scripts\build.ps1 -Config Release
scripts\test.ps1
build\bin\avloop.exe
```

Details, rules and the gates: [CONTRIBUTING.md](CONTRIBUTING.md).

## Keyboard shortcuts (working today)

| Action | Shortcut |
| --- | --- |
| Selection · Crop: Rectangle · Rounded · Ellipse | V · M · U · E |
| Hand · Zoom | H · Z |
| In · Out · Keyframe tools | I · O · K |

## Project structure

```text
src/core      the project model and the loop's maths (Qt Core only)
src/media     FFmpeg: probe, decode, normalize, cache, encode
src/render    the FrameRenderer (GPU and CPU) and the crop's mask
src/export    the export job, verification, the Loop Finder
src/ui        the main window, tools, canvas, timeline, panels, preview
src/app       avloop.exe            src/cli   avloop-cli.exe
resources     icons (SVG), themes (QSS with colour tokens), the app icon
tests         Qt Test unit tests (ctest -R avloop)
tools         gate tools (check-secrets, check-icons), walks, sample media
scripts       fetch-deps, build, test, format
phases        the development plan, its phase contracts and execution records
docs          the specification, the lore, screenshots
```

## Contributing

Contributions are welcome — read [CONTRIBUTING.md](CONTRIBUTING.md) and the [Code of Conduct](CODE_OF_CONDUCT.md).
Security reports: [SECURITY.md](SECURITY.md).

## License

AvLoop is MIT-licensed ([LICENSE](LICENSE)). It links Qt 6 (LGPLv3) and FFmpeg 7.1 (LGPL, with libvpx, dav1d and
zimg) dynamically; see [LICENSE-THIRD-PARTY.md](LICENSE-THIRD-PARTY.md).
