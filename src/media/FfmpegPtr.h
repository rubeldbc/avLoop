#pragma once

// RAII for every FFmpeg object AvLoop holds (spec §3.7.4, AVADR-0002): a std::unique_ptr with the matching free
// function. Only media/ (and render/ export/ later) include this header; ui/ never sees FFmpeg.

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavformat/avformat.h>
#include <libavutil/buffer.h>
#include <libavutil/frame.h>
#include <libswscale/swscale.h>
}

#include <QString>

#include <memory>

namespace avloop::media {

struct FormatContextDeleter {
    void operator()(AVFormatContext* p) const { avformat_close_input(&p); }
};
struct CodecContextDeleter {
    void operator()(AVCodecContext* p) const { avcodec_free_context(&p); }
};
struct FrameDeleter {
    void operator()(AVFrame* p) const { av_frame_free(&p); }
};
struct PacketDeleter {
    void operator()(AVPacket* p) const { av_packet_free(&p); }
};
struct SwsContextDeleter {
    void operator()(SwsContext* p) const { sws_freeContext(p); }
};
struct FilterGraphDeleter {
    void operator()(AVFilterGraph* p) const { avfilter_graph_free(&p); }
};
struct BufferRefDeleter {
    void operator()(AVBufferRef* p) const { av_buffer_unref(&p); }
};

using FormatContextPtr = std::unique_ptr<AVFormatContext, FormatContextDeleter>;
using CodecContextPtr = std::unique_ptr<AVCodecContext, CodecContextDeleter>;
using FramePtr = std::unique_ptr<AVFrame, FrameDeleter>;
using PacketPtr = std::unique_ptr<AVPacket, PacketDeleter>;
using SwsContextPtr = std::unique_ptr<SwsContext, SwsContextDeleter>;
using FilterGraphPtr = std::unique_ptr<AVFilterGraph, FilterGraphDeleter>;
using BufferRefPtr = std::unique_ptr<AVBufferRef, BufferRefDeleter>;

inline FramePtr makeFrame() {
    return FramePtr(av_frame_alloc());
}
inline PacketPtr makePacket() {
    return PacketPtr(av_packet_alloc());
}

// FFmpeg's message for an error code.
inline QString avError(int code) {
    char text[AV_ERROR_MAX_STRING_SIZE] = {};
    av_strerror(code, text, sizeof text);
    return QString::fromUtf8(text);
}

} // namespace avloop::media
