#include "media/MediaProbe.h"

#include "media/FfmpegPtr.h"

extern "C" {
#include <libavutil/display.h>
#include <libavutil/pixdesc.h>
}

#include <QCoreApplication>
#include <QFileInfo>

#include <cmath>

namespace avloop::media {

namespace {

QString tr(const char* text) {
    return QCoreApplication::translate("MediaProbe", text);
}

Rational rationalOf(AVRational r) {
    return Rational{r.num, r.den};
}

// The display matrix's turn and flips, read as ffmpeg's own autorotate reads them.
void readDisplayMatrix(const AVCodecParameters* par, MediaInfo& info) {
    const AVPacketSideData* sd =
        av_packet_side_data_get(par->coded_side_data, par->nb_coded_side_data, AV_PKT_DATA_DISPLAYMATRIX);
    if (!sd || sd->size < 9 * sizeof(int32_t)) return;
    const auto* m = reinterpret_cast<const int32_t*>(sd->data);
    double theta = -std::round(av_display_rotation_get(m));
    theta -= 360 * std::floor(theta / 360 + 0.9 / 360);
    if (std::fabs(theta - 90) < 1.0) {
        info.rotation = 90;
        info.flipH = m[3] > 0; // transpose=cclock_flip == clockwise turn + horizontal flip
    } else if (std::fabs(theta - 180) < 1.0) {
        // ffmpeg turns 180 by flipping both ways; a single flip means a mirror.
        const bool h = m[0] < 0;
        const bool v = m[4] < 0;
        if (h && v)
            info.rotation = 180;
        else {
            info.flipH = h;
            info.flipV = v;
        }
    } else if (std::fabs(theta - 270) < 1.0) {
        info.rotation = 270;
        info.flipH = m[3] < 0;
    } else if (std::fabs(theta) < 1.0) {
        info.flipV = m[4] < 0;
    }
}

bool isHdr(AVColorTransferCharacteristic trc) {
    return trc == AVCOL_TRC_SMPTE2084 || trc == AVCOL_TRC_ARIB_STD_B67;
}

} // namespace

bool MediaProbe::hardwareCodec(const QString& codec) {
    static const QStringList codecs = {QStringLiteral("h264"), QStringLiteral("hevc"),       QStringLiteral("vp9"),
                                       QStringLiteral("av1"),  QStringLiteral("mpeg2video"), QStringLiteral("vc1"),
                                       QStringLiteral("wmv3")};
    return codecs.contains(codec);
}

MediaProbe::Result MediaProbe::probe(const QString& path) {
    Result result;
    const QString name = QFileInfo(path).fileName();
    if (!QFileInfo::exists(path)) {
        result.error = tr("Cannot open %1: the file does not exist.").arg(name);
        return result;
    }
    AVDictionary* options = nullptr;
    av_dict_set(&options, "probesize", "52428800", 0);
    av_dict_set(&options, "analyzeduration", "10000000", 0);
    AVFormatContext* raw = nullptr;
    const QByteArray utf8 = path.toUtf8();
    int err = avformat_open_input(&raw, utf8.constData(), nullptr, &options);
    av_dict_free(&options);
    if (err < 0) {
        result.error = tr("Cannot open %1: %2. It is not a video file AvLoop can read.").arg(name, avError(err));
        return result;
    }
    FormatContextPtr fmt(raw);
    err = avformat_find_stream_info(fmt.get(), nullptr);
    const QString container = QString::fromUtf8(fmt->iformat->long_name ? fmt->iformat->long_name : fmt->iformat->name);
    if (err < 0) {
        result.error = tr("Cannot read %1 (container: %2): %3.").arg(name, container, avError(err));
        return result;
    }
    const int index = av_find_best_stream(fmt.get(), AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (index < 0) {
        result.error = tr("This file has no video AvLoop can decode (container: %1).").arg(container);
        return result;
    }
    const AVStream* st = fmt->streams[index];
    const AVCodecParameters* par = st->codecpar;
    const AVCodec* decoder = avcodec_find_decoder(par->codec_id);
    const QString codec = QString::fromUtf8(avcodec_get_name(par->codec_id));
    if (par->codec_id == AV_CODEC_ID_WEBP && (par->width <= 0 || par->height <= 0)) {
        result.error = tr("This animated WebP cannot be decoded: FFmpeg reads still WebP images only. Convert it to "
                          "GIF, APNG or a video first.");
        return result;
    }
    if (!decoder || par->width <= 0 || par->height <= 0) {
        result.error = tr("This file has no video AvLoop can decode (codec: %1).").arg(codec);
        return result;
    }

    MediaInfo info;
    info.path = path;
    info.container = QString::fromUtf8(fmt->iformat->name);
    info.containerLong = container;
    info.codec = codec;
    const AVCodecDescriptor* desc = avcodec_descriptor_get(par->codec_id);
    info.codecLong = desc && desc->long_name ? QString::fromUtf8(desc->long_name) : codec;
    info.streamIndex = index;
    info.codedWidth = par->width;
    info.codedHeight = par->height;
    AVRational sar = av_guess_sample_aspect_ratio(fmt.get(), const_cast<AVStream*>(st), nullptr);
    if (sar.num <= 0 || sar.den <= 0) sar = AVRational{1, 1};
    info.sar = rationalOf(av_make_q(sar.num, sar.den)).reduced();
    readDisplayMatrix(par, info);
    // Square pixels: widen (SAR > 1) or heighten (SAR < 1) so no detail is lost; even sizes.
    int w = par->width;
    int h = par->height;
    if (sar.num != sar.den) {
        if (sar.num > sar.den)
            w = int(std::lround(double(w) * sar.num / sar.den));
        else
            h = int(std::lround(double(h) * sar.den / sar.num));
    }
    w += w & 1;
    h += h & 1;
    if (info.rotation == 90 || info.rotation == 270) std::swap(w, h);
    info.width = w;
    info.height = h;

    info.avgFps = rationalOf(st->avg_frame_rate);
    info.realFps = rationalOf(st->r_frame_rate);
    if (!info.avgFps.isValid()) info.avgFps = info.realFps;
    info.vfr = info.avgFps.isValid() && info.realFps.isValid() &&
               std::fabs(info.avgFps.value() - info.realFps.value()) > 0.01 * info.realFps.value();
    info.timeBase = rationalOf(st->time_base);
    info.startPts = st->start_time != AV_NOPTS_VALUE ? st->start_time : 0;
    if (st->duration != AV_NOPTS_VALUE && st->duration > 0)
        info.duration = st->duration * av_q2d(st->time_base);
    else if (fmt->duration != AV_NOPTS_VALUE)
        info.duration = double(fmt->duration) / AV_TIME_BASE;
    info.frameCount = st->nb_frames > 0 ? st->nb_frames : std::int64_t(std::llround(info.duration * info.fps()));

    const auto pixfmt = static_cast<AVPixelFormat>(par->format);
    const AVPixFmtDescriptor* pd = av_pix_fmt_desc_get(pixfmt);
    info.pixelFormat = pd ? QString::fromUtf8(pd->name) : QStringLiteral("unknown");
    info.bitDepth = pd ? pd->comp[0].depth : 8;
    const AVDictionaryEntry* alphaMode = av_dict_get(st->metadata, "alpha_mode", nullptr, 0);
    info.hasAlpha = (pd && (pd->flags & AV_PIX_FMT_FLAG_ALPHA)) || (alphaMode && QByteArray(alphaMode->value) == "1");
    info.hdr = isHdr(par->color_trc);
    info.transfer =
        QString::fromUtf8(av_color_transfer_name(par->color_trc) ? av_color_transfer_name(par->color_trc) : "unknown");
    info.interlaced = par->field_order != AV_FIELD_PROGRESSIVE && par->field_order != AV_FIELD_UNKNOWN;
    info.hardwareCandidate = hardwareCodec(codec) && !info.hasAlpha;
    result.info = info;
    return result;
}

QString MediaInfo::statusLine() const {
    static const QHash<QString, QString> names = {
        {QStringLiteral("h264"), QStringLiteral("H.264")},
        {QStringLiteral("hevc"), QStringLiteral("HEVC")},
        {QStringLiteral("mpeg2video"), QStringLiteral("MPEG-2")},
        {QStringLiteral("mpeg1video"), QStringLiteral("MPEG-1")},
        {QStringLiteral("mpeg4"), QStringLiteral("MPEG-4")},
        {QStringLiteral("prores"), QStringLiteral("ProRes")},
        {QStringLiteral("dnxhd"), QStringLiteral("DNxHD")},
        {QStringLiteral("cfhd"), QStringLiteral("CineForm")},
        {QStringLiteral("qtrle"), QStringLiteral("QuickTime Animation")},
        {QStringLiteral("theora"), QStringLiteral("Theora")},
        {QStringLiteral("rawvideo"), QStringLiteral("Uncompressed")},
        {QStringLiteral("wmv2"), QStringLiteral("WMV 8")},
        {QStringLiteral("wmv3"), QStringLiteral("WMV 9")},
        {QStringLiteral("vc1"), QStringLiteral("VC-1")},
        {QStringLiteral("flv1"), QStringLiteral("Sorenson H.263")},
        {QStringLiteral("hap"), QStringLiteral("HAP")},
    };
    const QString shown = names.value(codec, codec.toUpper());
    QString line = QStringLiteral("%1×%2 · %3 fps · %4").arg(width).arg(height).arg(fps(), 0, 'f', 2).arg(shown);
    if (vfr) line += QStringLiteral(" · VFR → CFR");
    if (hdr) line += QStringLiteral(" · HDR → SDR");
    if (interlaced) line += QStringLiteral(" · interlaced");
    if (hasAlpha) line += QStringLiteral(" · alpha");
    return line;
}

QString MediaProbe::libraryVersions() {
    auto v = [](unsigned n) {
        return QStringLiteral("%1.%2.%3").arg(AV_VERSION_MAJOR(n)).arg(AV_VERSION_MINOR(n)).arg(AV_VERSION_MICRO(n));
    };
    return QStringLiteral("FFmpeg %1 · libavcodec %2 · libavformat %3")
        .arg(QString::fromUtf8(av_version_info()), v(avcodec_version()), v(avformat_version()));
}

} // namespace avloop::media
