#pragma once

#include "media/MediaInfo.h"

#include <QString>

#include <optional>

namespace avloop::media {

// Opens a file by its content (spec §3.9: probesize 50 MB, analyzeduration 10 s), never by its extension, and reads
// what AvLoop needs of its default video stream. Audio streams are ignored.
class MediaProbe {
public:
    struct Result {
        std::optional<MediaInfo> info;
        QString error; // set when info is empty: what happened and the container or codec
    };
    static Result probe(const QString& path);
    // The codecs D3D11VA decodes (spec §3.6): H.264, HEVC (8/10-bit), VP9, AV1, MPEG-2, VC-1.
    static bool hardwareCodec(const QString& codec);
};

} // namespace avloop::media
