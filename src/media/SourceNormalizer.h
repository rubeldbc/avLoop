#pragma once

#include "media/FfmpegPtr.h"
#include "media/MediaInfo.h"

#include <QSize>
#include <QString>

namespace avloop::media {

// Turns a decoded frame into what AvLoop works with (spec §12.1): deinterlaced (bwdif), tone-mapped to SDR BT.709
// (zscale + tonemap=hable) when HDR, turned and flipped upright, square pixels, scaled to the working size, RGBA
// premultiplied. One AVFilterGraph a source, rebuilt when the input or the size changes. Internal to media/.
class SourceNormalizer {
public:
    explicit SourceNormalizer(const MediaInfo& info);

    // The output size, in display orientation.
    void setOutputSize(const QSize& size);
    QSize outputSize() const { return size_; }

    // Converts `cur`. An interlaced source also takes its neighbours (either may be null) so bwdif has its fields.
    bool convert(const AVFrame* prev, const AVFrame* cur, const AVFrame* next, Frame& out, QString* error = nullptr);

    // The filter chain for an input of this format (exposed for tests and the log).
    QString chain(int inWidth, int inHeight) const;

private:
    bool build(const AVFrame* sample, QString* error);
    bool push(const AVFrame* frame, QString* error);
    void wrap(AVFrame* filtered, std::int64_t pts, Frame& out) const;

    MediaInfo info_;
    QSize size_;
    FilterGraphPtr graph_;
    AVFilterContext* src_ = nullptr;
    AVFilterContext* sink_ = nullptr;
    int inWidth_ = 0;
    int inHeight_ = 0;
    int inFormat_ = -1;
};

} // namespace avloop::media
