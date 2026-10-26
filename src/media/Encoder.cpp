#include "media/Encoder.h"

#include "media/EncoderOptions.h"
#include "media/GifPalette.h"
#include "media/Muxer.h"

#include <QFile>

#include <algorithm>
#include <vector>

namespace avloop::media {

struct Encoder::State {
    CodecContextPtr codec;
    SwsContextPtr sws;
    FramePtr frame;
    FramePtr gifFrame; // PAL8 from the quantizer
    PacketPtr packet;
    Muxer muxer;
    bool file = false;
    bool paletteOnly = false; // GIF's first pass: the palette, no file
    GifPaletteMaker palette;
    std::unique_ptr<GifQuantizer> quantizer;
    QByteArray statsIn;                 // pass 2's statistics, NUL-ended, for as long as the codec reads them
    std::vector<std::uint8_t> straight; // a frame made straight (not premultiplied) for the encoder
};

Encoder::Encoder(const EncodeSettings& settings) : settings_(settings), s_(std::make_unique<State>()) {}

Encoder::~Encoder() {
    if (s_->codec || s_->paletteOnly) abort();
}

bool Encoder::openCodec(QString* error) {
    // MP4: Media Foundation's H.264 first, OpenH264 (BSD) when it cannot open — never a GPL encoder (spec §3.1).
    for (const char* name : {encoderName(settings_), fallbackEncoderName(settings_)}) {
        if (!name) break;
        const AVCodec* codec = avcodec_find_encoder_by_name(name);
        if (!codec) continue;
        s_->codec.reset(avcodec_alloc_context3(codec));
        if (!s_->codec) {
            if (error) *error = QStringLiteral("no memory for the encoder");
            return false;
        }
        configure(s_->codec.get(), settings_);
        if (settings_.pass == 2 && !settings_.passStats.isEmpty() && settings_.format != OutputFormat::Gif) {
            s_->statsIn = settings_.passStats;
            s_->statsIn.append('\0');
            s_->codec->stats_in = s_->statsIn.data(); // ours: taken back before the codec is freed
        }
        AVDictionary* options = encoderOptions(settings_);
        const int r = avcodec_open2(s_->codec.get(), codec, &options);
        av_dict_free(&options);
        if (r >= 0) {
            encoderUsed_ = QString::fromLatin1(name);
            return true;
        }
        if (s_->codec) s_->codec->stats_in = nullptr;
        s_->codec.reset();
        if (error) *error = QStringLiteral("%1: %2").arg(QString::fromLatin1(name), avError(r));
    }
    if (error && error->isEmpty())
        *error = QStringLiteral("no %1 encoder in this FFmpeg").arg(QString::fromLatin1(encoderName(settings_)));
    return false;
}

bool Encoder::open(const QString& path, QString* error) {
    auto fail = [&](const QString& why) {
        if (error) *error = why;
        abort();
        return false;
    };
    path_ = path;
    written_ = 0;
    bytes_ = keyBytes_ = 0;
    stats_.clear();
    const int w = settings_.size.width();
    const int h = settings_.size.height();
    s_->straight.resize(std::size_t(w) * h * 4);
    const bool gif = settings_.format == OutputFormat::Gif;
    if (gif && settings_.pass == 1) {
        // GIF's first pass makes its palette from every frame (spec §10.4); no encoder, no file.
        s_->paletteOnly = true;
        QString why;
        return s_->palette.open(settings_.size, &why) || fail(why);
    }
    s_->file = !path.isEmpty() && settings_.pass != 1;
    QString why;
    if (!openCodec(&why)) return fail(why);
    if (s_->file && !s_->muxer.open(path, s_->codec.get(), settings_.format, settings_.alpha, &why)) {
        outOfSpace_ = s_->muxer.outOfSpace();
        return fail(why);
    }
    s_->packet = makePacket();
    if (gif) {
        s_->gifFrame = makeFrame();
        // Pass 2 has the palette of every frame; a single pass makes one from its first frame (in write()).
        if (settings_.pass == 2) {
            s_->quantizer = std::make_unique<GifQuantizer>();
            if (!s_->quantizer->open(settings_.size, settings_.passStats, &why)) return fail(why);
        }
        return true;
    }
    const AVPixelFormat target = s_->codec->pix_fmt;
    s_->sws.reset(sws_getContext(w, h, AV_PIX_FMT_RGBA, w, h, target, SWS_BICUBIC | SWS_FULL_CHR_H_INP, nullptr,
                                 nullptr, nullptr));
    if (!s_->sws) return fail(QStringLiteral("no colour conversion to the encoder's format"));
    if (target != AV_PIX_FMT_RGBA && target != AV_PIX_FMT_RGB24) {
        // RGB full range in, limited range out in the stream's matrix (BT.709; VP8's BT.601) — what it is tagged with.
        const int* matrix =
            sws_getCoefficients(s_->codec->colorspace == AVCOL_SPC_BT709 ? SWS_CS_ITU709 : SWS_CS_ITU601);
        sws_setColorspaceDetails(s_->sws.get(), matrix, 1, matrix, 0, 0, 1 << 16, 1 << 16);
    }
    s_->frame = makeFrame();
    s_->frame->format = target;
    s_->frame->width = w;
    s_->frame->height = h;
    if (av_frame_get_buffer(s_->frame.get(), 0) < 0) return fail(QStringLiteral("no memory for a frame"));
    return true;
}

bool Encoder::put(AVPacket* packet, QString* error) {
    // The alpha plane travels as the packet's side data (Matroska BlockAdditional): it counts toward the size.
    std::int64_t size = packet->size;
    for (int i = 0; i < packet->side_data_elems; ++i)
        size += std::int64_t(packet->side_data[i].size);
    bytes_ += size;
    if (packet->flags & AV_PKT_FLAG_KEY) keyBytes_ += size;
    bool ok = true;
    if (s_->file) {
        ok = s_->muxer.write(packet, error);
        outOfSpace_ = outOfSpace_ || s_->muxer.outOfSpace();
    }
    av_packet_unref(packet);
    return ok;
}

bool Encoder::send(AVFrame* frame, QString* error) {
    int r = avcodec_send_frame(s_->codec.get(), frame);
    while (r >= 0) {
        r = avcodec_receive_packet(s_->codec.get(), s_->packet.get());
        if (r == AVERROR(EAGAIN) || r == AVERROR_EOF) return true;
        if (r < 0) break;
        if (!put(s_->packet.get(), error)) return false;
    }
    if (error) *error = QStringLiteral("encode: %1").arg(avError(r));
    return false;
}

bool Encoder::write(const std::uint8_t* rgba, int stride, QString* error) {
    if (!s_->codec && !s_->paletteOnly) return false;
    const int w = settings_.size.width();
    const int h = settings_.size.height();
    // Premultiplied → straight: the encoder's alpha plane is separate, its colour planes unassociated.
    for (int y = 0; y < h; ++y) {
        const std::uint8_t* in = rgba + std::ptrdiff_t(y) * stride;
        std::uint8_t* out = s_->straight.data() + std::ptrdiff_t(y) * w * 4;
        for (int x = 0; x < w; ++x, in += 4, out += 4) {
            const int a = in[3];
            if (a == 255 || a == 0 || !settings_.alpha) {
                out[0] = in[0];
                out[1] = in[1];
                out[2] = in[2];
            } else {
                for (int c = 0; c < 3; ++c)
                    out[c] = std::uint8_t(std::min(255, (in[c] * 255 + a / 2) / a));
            }
            out[3] = settings_.alpha ? std::uint8_t(a) : 255;
        }
    }
    if (s_->paletteOnly) {
        if (!s_->palette.add(s_->straight.data(), w * 4, error)) return false;
        ++written_;
        return true;
    }
    AVFrame* frame = s_->frame.get();
    if (settings_.format == OutputFormat::Gif) {
        if (!s_->quantizer) {
            s_->quantizer = std::make_unique<GifQuantizer>();
            const QByteArray palette = GifQuantizer::paletteOf(settings_.size, s_->straight.data(), w * 4);
            if (!s_->quantizer->open(settings_.size, palette, error)) return false;
        }
        if (!s_->quantizer->quantize(s_->straight.data(), w * 4, s_->gifFrame.get(), error)) return false;
        frame = s_->gifFrame.get();
    } else {
        if (av_frame_make_writable(frame) < 0) return false;
        const std::uint8_t* src[1] = {s_->straight.data()};
        const int srcStride[1] = {w * 4};
        sws_scale(s_->sws.get(), src, srcStride, 0, h, frame->data, frame->linesize);
        frame->color_range = AVCOL_RANGE_MPEG;
        frame->colorspace = s_->codec->colorspace;
    }
    frame->pts = written_;
    frame->pict_type = written_ == 0 ? AV_PICTURE_TYPE_I : AV_PICTURE_TYPE_NONE;
    if (!send(frame, error)) return false;
    ++written_;
    return true;
}

bool Encoder::finish(QString* error) {
    if (s_->paletteOnly) {
        stats_ = s_->palette.finish(error); // GIF's first pass: the palette is its statistics
        s_->paletteOnly = false;
        close();
        return !stats_.isEmpty();
    }
    if (!s_->codec) return false;
    if (!send(nullptr, error)) return false;
    if (s_->codec->stats_out) stats_ = QByteArray(s_->codec->stats_out); // pass 1: set at the flush
    bool ok = true;
    if (s_->file) {
        ok = s_->muxer.finish(error);
        outOfSpace_ = outOfSpace_ || s_->muxer.outOfSpace();
    }
    close();
    return ok;
}

void Encoder::close() {
    if (s_->codec) s_->codec->stats_in = nullptr;
    s_->codec.reset();
    s_->sws.reset();
    s_->frame.reset();
    s_->gifFrame.reset();
    s_->packet.reset();
    s_->quantizer.reset();
    s_->statsIn.clear();
    s_->straight = {};
}

void Encoder::abort() {
    s_->muxer.close();
    s_->paletteOnly = false;
    close();
    if (s_->file && !path_.isEmpty()) QFile::remove(path_);
}

} // namespace avloop::media
