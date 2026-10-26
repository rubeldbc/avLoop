#pragma once

#include "core/QualitySettings.h"

#include <QList>
#include <QSize>
#include <QString>

namespace avloop::exporting {

// What each output format is (spec §10.2, §10.4): its name, its file extension, whether it carries alpha, its highest
// rate. WebM VP9 and VP8 now; GIF, APNG, MP4 and ProRes join at AV-013.
struct FormatProfile {
    OutputFormat format = OutputFormat::WebmVp9;
    QString name;      // "WebM VP9"
    QString extension; // "webm"
    bool alpha = true;
    int maxFps = 60;
};

const QList<FormatProfile>& formatProfiles();
const FormatProfile& profileOf(OutputFormat format);
// "<source>_loop_<W>x<H>.<ext>" (spec §10.1).
QString defaultFileName(const QString& sourcePath, QSize size, OutputFormat format);
// The first free "name (1).webm", "name (2).webm" … beside `path` (spec §12.3: a file OBS holds).
QString alternativePath(const QString& path);

} // namespace avloop::exporting
