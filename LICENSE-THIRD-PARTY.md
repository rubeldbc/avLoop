# Third-party notices

AvLoop's own code is MIT-licensed (`LICENSE`). It links the libraries below **dynamically** and ships their DLLs
unmodified beside `avloop.exe`, so each can be replaced by a user-built version of the same release (the LGPL's
requirement). No GPL component (libx264, libx265) is linked; MP4 export uses the Windows Media Foundation encoder.

| Library | Version | Licence | Where its source is | Why AvLoop uses it |
| --- | --- | --- | --- | --- |
| Qt — Core, Gui, Widgets, OpenGL, OpenGLWidgets, Svg, Concurrent, Test | 6.8.3 | LGPLv3 | https://download.qt.io/official_releases/qt/6.8/6.8.3/ | UI, events, settings, JSON, theme; the GPU canvas and offscreen rendering; the app's SVG icons; parallel work; tests |
| FFmpeg — libavformat, libavcodec, libavutil, libswscale, libavfilter (and libswresample, which they load) | n7.1.5 (BtbN `win64-lgpl-shared-7.1`, autobuild 2026-07-31) | LGPL version 3 or later (`--enable-version3`, no `--enable-gpl`) | https://ffmpeg.org/download.html; the build's recipe: https://github.com/BtbN/FFmpeg-Builds | reading any container, decoding every codec, encoding WebM/GIF/APNG/MP4/ProRes, scaling and pixel formats, deinterlacing, tone mapping, motion interpolation |
| libvpx (inside the FFmpeg DLLs) | as built by BtbN | BSD-3-Clause | https://chromium.googlesource.com/webm/libvpx | VP8/VP9 encoding and decoding **with alpha** |
| dav1d (inside the FFmpeg DLLs) | as built by BtbN | BSD-2-Clause | https://code.videolan.org/videolan/dav1d | AV1 software decoding |
| zimg (inside the FFmpeg DLLs) | as built by BtbN | WTFPL | https://github.com/sekrit-twc/zimg | HDR→SDR tone mapping and colour-space conversion |

The FFmpeg DLLs also carry, statically, the other libraries BtbN's LGPL build enables (its configuration line is
printed by `avloop-cli version`); each is under a licence compatible with the LGPL. The full licence texts are
installed with AvLoop (AV-014) and are in `third_party/ffmpeg/LICENSE.txt` after `scripts\fetch-deps.ps1`.

Windows components used through the Windows SDK (D3D11, DXGI, PSAPI, Media Foundation) are part of Windows and are
not redistributed.
