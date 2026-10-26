#pragma once

#include "core/QualitySettings.h"

#include <QList>
#include <QSize>
#include <QString>

namespace avloop::exporting {

// What each output format is (spec §10.2, §10.4): its name, its file extension, whether it carries alpha, its highest
// rate — WebM VP9 and VP8, GIF (1-bit alpha, at most 50 fps), APNG, MP4 H.264 (no alpha: the Background under) and
// MOV ProRes 4444.
struct FormatProfile {
    OutputFormat format = OutputFormat::WebmVp9;
    QString name;      // "WebM VP9"
    QString extension; // "webm"
    bool alpha = true;
    int maxFps = 60;
};

const QList<FormatProfile>& formatProfiles();
const FormatProfile& profileOf(OutputFormat format);
// Whether this FFmpeg build has the format's encoder (MP4: Media Foundation's or OpenH264); `reason` when it has not.
bool available(OutputFormat format, QString* reason = nullptr);
// The name template (Settings → Export; spec §10.1's "<source>_loop_<W>x<H>") filled in: {source}, {w}, {h}; then
// the format's extension.
QString defaultFileName(const QString& sourcePath, QSize size, OutputFormat format,
                        const QString& pattern = QStringLiteral("{source}_loop_{w}x{h}"));
// The first free "name (1).webm", "name (2).webm" … beside `path` (spec §12.3: a file OBS holds).
QString alternativePath(const QString& path);

} // namespace avloop::exporting
