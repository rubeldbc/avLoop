#include "media/FrameIndex.h"

#include "media/FfmpegPtr.h"

#include <algorithm>

namespace avloop::media {

QVector<double> frameTimes(const MediaInfo& info, double from, double to) {
    QVector<double> times;
    AVFormatContext* raw = nullptr;
    if (avformat_open_input(&raw, info.path.toUtf8().constData(), nullptr, nullptr) < 0) return times;
    FormatContextPtr fmt(raw);
    if (avformat_find_stream_info(fmt.get(), nullptr) < 0 || info.streamIndex >= int(fmt->nb_streams)) return times;
    const AVRational tb = fmt->streams[info.streamIndex]->time_base;
    const auto seconds = [&](std::int64_t pts) { return double(pts - info.startPts) * av_q2d(tb); };
    // A little before `from`, so the frame on screen at `from` is in the list.
    const std::int64_t start = info.startPts + std::int64_t((std::max(0.0, from - 2.0)) / av_q2d(tb));
    av_seek_frame(fmt.get(), info.streamIndex, start, AVSEEK_FLAG_BACKWARD);
    PacketPtr packet = makePacket();
    while (av_read_frame(fmt.get(), packet.get()) >= 0) {
        if (packet->stream_index == info.streamIndex) {
            const std::int64_t pts = packet->pts != AV_NOPTS_VALUE ? packet->pts : packet->dts;
            if (pts != AV_NOPTS_VALUE) {
                const double t = seconds(pts);
                times.append(t);
                // Packets come in decode order: read on a little past `to` so every frame shown before it is seen.
                if (t > to + 2.0) {
                    av_packet_unref(packet.get());
                    break;
                }
            }
        }
        av_packet_unref(packet.get());
    }
    std::sort(times.begin(), times.end());
    times.erase(std::unique(times.begin(), times.end()), times.end());
    return times;
}

} // namespace avloop::media
