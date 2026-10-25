# 16 Shapes — the batch and the other formats (item 13, AV-013)

Canonical current execution state: `../AUTONOMOUS_STATE.md`

Spec §10.4, §10.5, §3.1 (no GPL). D-013.

## 1. The formats (`export/FormatProfiles`)

| Format | Encoder | Alpha | Note |
| --- | --- | --- | --- |
| GIF | `palettegen` (stats_mode=diff, reserve_transparent) + `paletteuse` (dither=bayer) via an `AVFilterGraph`, then the `gif` encoder | 1-bit (alpha ≥ 128 opaque) | `loop=0`; at most 50 fps (higher → 50) |
| APNG | `apng` (`rgba`) | full | `plays=0` |
| MP4 H.264 | `h264_mf` (Media Foundation; hardware when present) | none; the Background colour under | `movflags=+faststart`, `yuv420p` |
| MOV ProRes 4444 | `prores_ks` `profile=4444` | full (`yuva444p10le`) | large files |

The Output panel's Format row offers them; the extension follows the format; the Quality panel shows what applies
(GIF/APNG: none; MP4: a bitrate from the preset; ProRes: the profile). `OutputVerify` per format (frame count, size,
codec; alpha where full). An encoder missing from the FFmpeg build → the format disabled with the reason.

## 2. The queue (`export/ExportQueue`)

Entries: a project snapshot (the `.avloop` JSON) + an output path + a state (Pending, Running, Done, Failed with its
reason, Paused). One at a time (memory); the queue file `%APPDATA%\AvLoop\queue.json` written on each change and
locked by the instance that runs it (`QLockFile`); at start, Running entries come back as Pending; a failed entry
keeps its reason and the next one runs.

## 3. The window (`ui/export/BatchWindow`)

Menu → Batch Queue; Ctrl+Shift+E adds the current project with the output path from the Save dialog or the name
template. The list: name, format, size, state, progress; drag to reorder; Start, Pause, Remove, Clear done; the app
icon.

## 4. Tests and walks

`test-export` grows GIF/APNG/MP4/ProRes (each small); the queue's persistence and order. Walks: `walk-formats.mjs`
(each format by `avloop-cli export --format`: ffprobe's codec, size, frame count; alpha for APNG/ProRes; GIF's
transparency), `walk-batch.json` (three entries added, Start, all Done, pictures; the app restarted with a Pending
entry).
