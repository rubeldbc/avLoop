#pragma once

// The lab's sample videos (tools/media/make-samples.mjs) for the media tests. A test whose sample is missing is
// skipped and says which, so a machine without the samples still runs the rest.

#include "media/MediaInfo.h"

#include <QDir>
#include <QFileInfo>
#include <QString>
#include <QTest>

namespace avloop::test {

inline QString samplePath(const QString& name) {
    const QString dir = qEnvironmentVariableIsSet("AVLOOP_SAMPLES") ? qEnvironmentVariable("AVLOOP_SAMPLES")
                                                                    : QStringLiteral(AVLOOP_SAMPLES_DIR);
    return QDir(dir).filePath(name);
}

// The frame counter burned into the counter samples' foot: 16 stripes, bit k = stripe k (spec §2.6, §7.3).
inline int readCounter(const media::Frame& f) {
    int n = 0;
    for (int k = 0; k < 16; ++k) {
        const int x = k * f.width / 16 + f.width / 32;
        const int y = f.height - 16; // the strip is 32 px at the native size
        const std::uint8_t* p = f.data.get() + std::ptrdiff_t(y) * f.stride + std::ptrdiff_t(x) * 4;
        if ((p[0] + p[1] + p[2]) / 3 > 128) n |= 1 << k;
    }
    return n;
}

} // namespace avloop::test

#define AVLOOP_NEED_SAMPLE(name)                                                                                       \
    if (!QFileInfo::exists(avloop::test::samplePath(name)))                                                            \
    QSKIP(qPrintable(QStringLiteral("sample %1 missing: node tools/media/make-samples.mjs").arg(name)))
