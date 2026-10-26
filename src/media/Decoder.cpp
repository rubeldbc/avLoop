#include "media/Decoder.h"

#include "media/DecoderImpl.h"

#include <QDebug>

#include <cmath>

namespace avloop::media {

Decoder::Decoder(const MediaInfo& info) : Decoder(info, Options{}) {}

Decoder::Decoder(const MediaInfo& info, const Options& options) : d(std::make_unique<Impl>()) {
    d->info = info;
    d->options = options;
}

Decoder::~Decoder() = default;

bool Decoder::open(QString* error) {
    AVDictionary* opts = nullptr;
    av_dict_set(&opts, "probesize", "52428800", 0);
    av_dict_set(&opts, "analyzeduration", "10000000", 0);
    AVFormatContext* raw = nullptr;
    const QByteArray path = d->info.path.toUtf8();
    int r = avformat_open_input(&raw, path.constData(), nullptr, &opts);
    av_dict_free(&opts);
    if (r < 0) {
        if (error) *error = QStringLiteral("Cannot open %1: %2").arg(d->info.path, avError(r));
        return false;
    }
    d->fmt.reset(raw);
    r = avformat_find_stream_info(d->fmt.get(), nullptr);
    if (r < 0 || d->info.streamIndex < 0 || d->info.streamIndex >= int(d->fmt->nb_streams)) {
        if (error) *error = QStringLiteral("Cannot read %1: %2").arg(d->info.path, avError(r));
        return false;
    }
    // Audio and every other stream are never decoded (spec §3.9).
    for (unsigned i = 0; i < d->fmt->nb_streams; ++i)
        if (int(i) != d->info.streamIndex) d->fmt->streams[i]->discard = AVDISCARD_ALL;
    d->stream = d->fmt->streams[d->info.streamIndex];
    d->packet = makePacket();
    const double fps = d->info.fps() > 0 ? d->info.fps() : 25.0;
    d->frameDuration = qMax<std::int64_t>(1, std::llround(1.0 / (fps * av_q2d(d->stream->time_base))));
    if (!d->openCodec(d->options.hardware, error)) return false;
    d->normalizer = std::make_unique<SourceNormalizer>(d->info);
    d->normalizer->setOutputSize(d->computeWorkingSize());
    return true;
}

const MediaInfo& Decoder::info() const {
    return d->info;
}

Frame Decoder::frameAt(double seconds) {
    return d->frameAtPts(d->ptsOf(seconds));
}

Frame Decoder::keyframeAt(double seconds) {
    return d->keyframeAtPts(d->ptsOf(seconds));
}

Frame Decoder::frameAtIndex(std::int64_t index) {
    const double fps = d->info.fps() > 0 ? d->info.fps() : 25.0;
    return frameAt((double(index) + 0.5) / fps);
}

Frame Decoder::next() {
    FramePtr n = d->pendingRaw ? std::move(d->pendingRaw) : d->decodeRaw();
    if (!n) return {};
    d->prevRaw = std::move(d->lastRaw);
    d->lastRaw = std::move(n);
    d->lastFrame = {};
    if (d->info.interlaced && !d->pendingRaw) d->pendingRaw = d->decodeRaw();
    return d->current();
}

void Decoder::setWorkingSize(const QSize& size) {
    d->options.workingSize = size;
    if (d->normalizer) d->normalizer->setOutputSize(d->computeWorkingSize());
    d->lastFrame = {};
}

QSize Decoder::workingSize() const {
    return d->normalizer ? d->normalizer->outputSize() : d->computeWorkingSize();
}

bool Decoder::hardwareActive() const {
    return d->hwActive;
}

QString Decoder::hardwareNote() const {
    return d->hwNote;
}

int Decoder::corruptFrames() const {
    return d->corrupt;
}

int Decoder::seeks() const {
    return d->seekCount;
}

QSize Decoder::Impl::computeWorkingSize() const {
    QSize s = options.workingSize.isValid() && !options.workingSize.isEmpty() ? options.workingSize
                                                                              : QSize(info.width, info.height);
    const int longSide = qMax(s.width(), s.height());
    if (options.maxLongSide > 0 && longSide > options.maxLongSide) {
        const double k = double(options.maxLongSide) / longSide;
        s = QSize(int(std::lround(s.width() * k)), int(std::lround(s.height() * k)));
    }
    return QSize(qMax(2, s.width() + (s.width() & 1)), qMax(2, s.height() + (s.height() & 1)));
}

double Decoder::Impl::seconds(std::int64_t pts) const {
    return double(pts - info.startPts) * av_q2d(stream ? stream->time_base : AVRational{1, 1});
}

std::int64_t Decoder::Impl::ptsOf(double secs) const {
    return info.startPts + std::llround(secs / av_q2d(stream ? stream->time_base : AVRational{1, 1}));
}

bool Decoder::Impl::openCodec(bool hardware, QString* error) {
    const AVCodecParameters* par = stream->codecpar;
    const AVCodec* codec = nullptr;
    // The native VP8/VP9 decoders drop the alpha plane; libvpx keeps it (spec §3.8).
    if (info.hasAlpha && par->codec_id == AV_CODEC_ID_VP9) codec = avcodec_find_decoder_by_name("libvpx-vp9");
    if (info.hasAlpha && par->codec_id == AV_CODEC_ID_VP8) codec = avcodec_find_decoder_by_name("libvpx");
    if (!codec) codec = avcodec_find_decoder(par->codec_id);
    if (!codec) {
        if (error) *error = QStringLiteral("no decoder for %1").arg(info.codec);
        return false;
    }
    for (int attempt = 0; attempt < 2; ++attempt) {
        const bool tryHw = hardware && attempt == 0 && info.hardwareCandidate;
        CodecContextPtr c(avcodec_alloc_context3(codec));
        if (!c || avcodec_parameters_to_context(c.get(), par) < 0) break;
        c->pkt_timebase = stream->time_base;
        const bool hw = tryHw && setupHardware(c.get(), codec);
        if (!hw) {
            c->thread_count = options.threads; // 0: as many as the logical cores
            c->thread_type = FF_THREAD_FRAME | FF_THREAD_SLICE;
        }
        const int r = avcodec_open2(c.get(), codec, nullptr);
        if (r >= 0) {
            ctx = std::move(c);
            hwActive = hw;
            return true;
        }
        if (!hw) {
            if (error) *error = QStringLiteral("cannot open the %1 decoder: %2").arg(info.codec, avError(r));
            return false;
        }
        hwNote = QStringLiteral("HW decode off: %1").arg(avError(r));
    }
    if (error) *error = QStringLiteral("cannot open the %1 decoder").arg(info.codec);
    return false;
}

FramePtr Decoder::Impl::decodeRaw() {
    FramePtr frame = makeFrame();
    while (true) {
        int r = avcodec_receive_frame(ctx.get(), frame.get());
        if (r == 0) {
            if ((frame->flags & AV_FRAME_FLAG_CORRUPT) || frame->decode_error_flags) {
                ++corrupt; // skipped: the previous good frame stays on screen (spec §12.1)
                av_frame_unref(frame.get());
                continue;
            }
            std::int64_t pts = frame->best_effort_timestamp;
            if (pts == AV_NOPTS_VALUE) pts = frame->pts;
            if (pts == AV_NOPTS_VALUE)
                pts = lastPtsSeen == AV_NOPTS_VALUE ? info.startPts : lastPtsSeen + frameDuration;
            if (lastPtsSeen != AV_NOPTS_VALUE && pts <= lastPtsSeen) pts = lastPtsSeen + 1; // keep display order strict
            frame->pts = pts;
            lastPtsSeen = pts;
            return frame;
        }
        if (r == AVERROR_EOF) {
            atEnd = true;
            return nullptr;
        }
        if (r != AVERROR(EAGAIN)) {
            if (hwActive) {
                fallbackToSoftware(avError(r));
                return nullptr;
            }
            ++corrupt;
        }
        if (flushed) {
            atEnd = true;
            return nullptr;
        }
        r = av_read_frame(fmt.get(), packet.get());
        if (r < 0) {
            if (waitKey && droppedForKey > 0) {
                // No keyframe came after the seek point. After a rewind to the start that means the container does
                // not flag keyframes: seek again without waiting (frameAtPts retries). Otherwise the seek simply
                // landed past the last keyframe: the caller seeks earlier.
                if (rewound) {
                    keysUnflagged = true;
                    needReseek = true;
                }
                waitKey = false;
                return nullptr;
            }
            avcodec_send_packet(ctx.get(), nullptr);
            flushed = true;
            continue;
        }
        if (packet->stream_index != info.streamIndex) {
            av_packet_unref(packet.get());
            continue;
        }
        // After a seek a container without an index (MPEG-TS, PS) may land mid-GOP: decoding starts at a keyframe.
        if (waitKey) {
            if (!(packet->flags & AV_PKT_FLAG_KEY)) {
                av_packet_unref(packet.get());
                ++droppedForKey;
                continue;
            }
            waitKey = false;
        }
        r = avcodec_send_packet(ctx.get(), packet.get());
        av_packet_unref(packet.get());
        if (r < 0 && r != AVERROR(EAGAIN)) {
            if (hwActive) {
                fallbackToSoftware(avError(r));
                return nullptr;
            }
            ++corrupt;
        }
    }
}

Frame Decoder::Impl::current() {
    if (!lastFrame.isNull() || !lastRaw) return lastFrame;
    FramePtr prevSw, curSw, nextSw;
    if (!toSoftware(lastRaw.get(), curSw)) return {};
    if (info.interlaced) {
        if (prevRaw) toSoftware(prevRaw.get(), prevSw);
        if (pendingRaw) toSoftware(pendingRaw.get(), nextSw);
    }
    QString error;
    Frame out;
    if (!normalizer->convert(prevSw.get(), curSw.get(), nextSw.get(), out, &error)) {
        qWarning().noquote() << "[media]" << info.path << error;
        return {};
    }
    out.seconds = seconds(out.pts);
    lastFrame = out;
    return lastFrame;
}

} // namespace avloop::media
