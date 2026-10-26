#include "export/OutputVerify.h"

#include "media/FfmpegPtr.h"

extern "C" {
#include <libavutil/pixdesc.h>
}

#include <QCoreApplication>
#include <QFileInfo>

#include <cmath>

namespace avloop::exporting {

namespace {

QString tr(const char* text) {
    return QCoreApplication::translate("OutputVerify", text);
}

QString codecOf(OutputFormat f) {
    switch (f) {
    case OutputFormat::WebmVp8:
        return QStringLiteral("vp8");
    case OutputFormat::Gif:
        return QStringLiteral("gif");
    case OutputFormat::Apng:
        return QStringLiteral("apng");
    case OutputFormat::Mp4H264:
        return QStringLiteral("h264");
    case OutputFormat::MovProRes:
        return QStringLiteral("prores");
    case OutputFormat::WebmVp9:
        break;
    }
    return QStringLiteral("vp9");
}

// The first frame decoded — VP8/VP9 through libvpx, which reads WebM's alpha (FFmpeg's own decoders drop it) — and
// whether it came with an alpha channel.
bool decodeFirst(const AVCodecParameters* par, const AVPacket* first, bool* alpha) {
    const AVCodec* codec = par->codec_id == AV_CODEC_ID_VP8   ? avcodec_find_decoder_by_name("libvpx")
                           : par->codec_id == AV_CODEC_ID_VP9 ? avcodec_find_decoder_by_name("libvpx-vp9")
                                                              : avcodec_find_decoder(par->codec_id);
    if (!codec) return false;
    media::CodecContextPtr c(avcodec_alloc_context3(codec));
    if (!c || avcodec_parameters_to_context(c.get(), par) < 0 || avcodec_open2(c.get(), codec, nullptr) < 0)
        return false;
    media::FramePtr frame = media::makeFrame();
    if (avcodec_send_packet(c.get(), first) < 0) return false;
    avcodec_send_packet(c.get(), nullptr);
    if (avcodec_receive_frame(c.get(), frame.get()) < 0) return false;
    const AVPixFmtDescriptor* d = av_pix_fmt_desc_get(AVPixelFormat(frame->format));
    *alpha = d && (d->flags & AV_PIX_FMT_FLAG_ALPHA);
    return true;
}

} // namespace

Verified verifyOutput(const QString& path, const Expected& expected) {
    Verified v;
    v.bytes = QFileInfo(path).size();
    AVFormatContext* raw = nullptr;
    if (avformat_open_input(&raw, path.toUtf8().constData(), nullptr, nullptr) < 0) {
        v.problem = tr("the file cannot be opened");
        return v;
    }
    media::FormatContextPtr fmt(raw);
    if (avformat_find_stream_info(fmt.get(), nullptr) < 0) {
        v.problem = tr("the file's streams cannot be read");
        return v;
    }
    v.opened = true;
    int video = -1;
    for (unsigned i = 0; i < fmt->nb_streams; ++i) {
        const AVCodecParameters* par = fmt->streams[i]->codecpar;
        if (par->codec_type == AVMEDIA_TYPE_AUDIO) ++v.audioStreams;
        if (par->codec_type == AVMEDIA_TYPE_VIDEO && video < 0) video = int(i);
    }
    if (video < 0) {
        v.problem = tr("no video stream");
        return v;
    }
    AVStream* st = fmt->streams[video];
    v.codec = QString::fromLatin1(avcodec_get_name(st->codecpar->codec_id));
    v.size = QSize(st->codecpar->width, st->codecpar->height);
    v.fps = Rational{st->r_frame_rate.num, st->r_frame_rate.den};
    const AVDictionaryEntry* mode = av_dict_get(st->metadata, "alpha_mode", nullptr, 0);
    v.alphaMarked = mode && QByteArray(mode->value) == "1";

    media::PacketPtr packet = media::makePacket();
    media::PacketPtr first = media::makePacket();
    while (av_read_frame(fmt.get(), packet.get()) >= 0) {
        if (packet->stream_index == video) {
            if (v.frames == 0) {
                v.firstKey = packet->flags & AV_PKT_FLAG_KEY;
                av_packet_ref(first.get(), packet.get());
            }
            ++v.frames;
        }
        av_packet_unref(packet.get());
    }
    if (v.frames > 0 && !decodeFirst(st->codecpar, first.get(), &v.alphaPlane)) {
        v.problem = tr("the first frame does not decode");
        return v;
    }

    const bool webm = expected.format == OutputFormat::WebmVp9 || expected.format == OutputFormat::WebmVp8;
    const bool gif = expected.format == OutputFormat::Gif;
    const QString want = codecOf(expected.format);
    if (v.codec != want)
        v.problem = tr("codec %1, not %2").arg(v.codec, want);
    else if (v.size != expected.size)
        v.problem = tr("%1×%2, not %3×%4")
                        .arg(v.size.width())
                        .arg(v.size.height())
                        .arg(expected.size.width())
                        .arg(expected.size.height());
    // GIF keeps its delays in hundredths of a second: its rate is not read back.
    else if (!gif && std::abs(v.fps.value() - expected.fps.value()) > 5e-4 * expected.fps.value())
        v.problem =
            tr("%1/%2 fps, not %3/%4").arg(v.fps.num).arg(v.fps.den).arg(expected.fps.num).arg(expected.fps.den);
    else if (v.frames != expected.frames)
        v.problem = tr("%1 frames, not %2").arg(v.frames).arg(expected.frames);
    else if (!v.firstKey)
        v.problem = tr("the first frame is not a keyframe");
    else if (expected.alpha && !gif && !(v.alphaPlane && (!webm || v.alphaMarked)))
        v.problem = tr("the alpha is missing");
    else if (v.audioStreams > 0)
        v.problem = tr("an audio stream");
    return v;
}

} // namespace avloop::exporting
