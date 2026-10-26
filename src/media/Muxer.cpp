#include "media/Muxer.h"

#include "media/EncoderOptions.h"

namespace avloop::media {

Muxer::~Muxer() {
    close();
}

bool Muxer::open(const QString& path, const AVCodecContext* codec, OutputFormat format, bool alpha, QString* error) {
    auto fail = [&](const QString& what, int r) {
        outOfSpace_ = r == AVERROR(ENOSPC);
        if (error) *error = QStringLiteral("%1: %2").arg(what, avError(r));
        close();
        return false;
    };
    const char* container = containerName(format);
    int r = avformat_alloc_output_context2(&fmt_, nullptr, container, path.toUtf8().constData());
    if (r < 0 || !fmt_) return fail(QString::fromLatin1(container), r);
    stream_ = avformat_new_stream(fmt_, nullptr);
    if (!stream_) return fail(QStringLiteral("stream"), AVERROR(ENOMEM));
    r = avcodec_parameters_from_context(stream_->codecpar, codec);
    if (r < 0) return fail(QStringLiteral("stream parameters"), r);
    codecTimeBase_ = codec->time_base;
    stream_->time_base = codec->time_base;
    stream_->avg_frame_rate = codec->framerate;
    // The Matroska muxer writes the AlphaMode element (and the alpha in BlockAdditions) only when told.
    if (alpha && QByteArray(container) == "webm") av_dict_set(&stream_->metadata, "alpha_mode", "1", 0);
    r = avio_open(&fmt_->pb, path.toUtf8().constData(), AVIO_FLAG_WRITE);
    if (r < 0) return fail(QStringLiteral("open %1").arg(path), r);
    AVDictionary* options = muxerOptions(format);
    r = avformat_write_header(fmt_, &options);
    av_dict_free(&options);
    if (r < 0) return fail(QStringLiteral("header"), r);
    headerWritten_ = true;
    return true;
}

bool Muxer::write(AVPacket* packet, QString* error) {
    packet->stream_index = stream_->index;
    av_packet_rescale_ts(packet, codecTimeBase_, stream_->time_base);
    const int r = av_interleaved_write_frame(fmt_, packet);
    outOfSpace_ = outOfSpace_ || r == AVERROR(ENOSPC);
    if (r < 0 && error) *error = QStringLiteral("write: %1").arg(avError(r));
    return r >= 0;
}

bool Muxer::finish(QString* error) {
    if (!fmt_ || !headerWritten_) return false;
    const int r = av_write_trailer(fmt_);
    outOfSpace_ = outOfSpace_ || r == AVERROR(ENOSPC);
    if (r < 0 && error) *error = QStringLiteral("trailer: %1").arg(avError(r));
    close();
    return r >= 0;
}

void Muxer::close() {
    if (!fmt_) return;
    if (fmt_->pb) avio_closep(&fmt_->pb);
    avformat_free_context(fmt_);
    fmt_ = nullptr;
    stream_ = nullptr;
    headerWritten_ = false;
}

} // namespace avloop::media
