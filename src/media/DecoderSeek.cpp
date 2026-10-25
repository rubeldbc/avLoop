// Frame-accurate access (spec §7.3): a seek goes BACKWARD to the keyframe before the target and decodes up to the
// target's PTS; a move forward within two seconds decodes on without seeking. When a container's seek lands after the
// target (some MPEG-TS, AVI), the seek is retried earlier — 1, 2, 4, 8 s — and finally from the start.
#include "media/DecoderImpl.h"

#include <QDebug>

namespace avloop::media {

void Decoder::Impl::seekTo(std::int64_t pts) {
    rewound = pts <= info.startPts;
    int r = -1;
    // To the start of an MPEG transport or program stream: by byte — their timestamp search can land after the first
    // keyframe, where nothing decodes.
    const bool mpegStream = info.container == QLatin1String("mpegts") || info.container == QLatin1String("mpeg");
    if (rewound && mpegStream) r = av_seek_frame(fmt.get(), -1, 0, AVSEEK_FLAG_BYTE);
    if (r < 0) r = av_seek_frame(fmt.get(), info.streamIndex, pts, AVSEEK_FLAG_BACKWARD);
    if (r < 0) r = avformat_seek_file(fmt.get(), info.streamIndex, INT64_MIN, pts, pts, 0);
    if (r < 0) av_seek_frame(fmt.get(), info.streamIndex, info.startPts, AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_ANY);
    avcodec_flush_buffers(ctx.get());
    flushed = false;
    atEnd = false;
    waitKey = !keysUnflagged;
    droppedForKey = 0;
    prevRaw.reset();
    lastRaw.reset();
    pendingRaw.reset();
    lastFrame = {};
    lastPtsSeen = AV_NOPTS_VALUE;
    ++seekCount;
}

Frame Decoder::Impl::locate(std::int64_t target) {
    // Already on screen?
    if (lastRaw && lastRaw->pts <= target && (pendingRaw ? pendingRaw->pts > target : atEnd)) return current();

    const std::int64_t window = ptsOf(2.0) - ptsOf(0.0);
    const bool forward = lastRaw && lastRaw->pts <= target && target - lastRaw->pts <= window;
    if (!forward) {
        std::int64_t seekPts = target;
        for (int retry = 0;; ++retry) {
            seekTo(seekPts);
            FramePtr first = decodeRaw();
            if (needReseek) return {};
            const bool last = seekPts <= info.startPts || retry >= 5;
            if (first && (first->pts <= target || last)) {
                lastRaw = std::move(first);
                break;
            }
            if (last) return {}; // nothing decodes from the start: an empty or broken stream
            // Landed after the target, or at the end: go back further, or from the start at the last try.
            seekPts =
                retry >= 4 ? info.startPts : qMax(info.startPts, target - (ptsOf(double(1 << retry)) - ptsOf(0.0)));
        }
    }
    while (true) {
        FramePtr next = pendingRaw ? std::move(pendingRaw) : decodeRaw();
        if (needReseek) return {};
        if (!next) {
            atEnd = true; // the target is past the last frame: the last frame stays
            return current();
        }
        if (next->pts > target) {
            pendingRaw = std::move(next);
            return current();
        }
        prevRaw = std::move(lastRaw);
        lastRaw = std::move(next);
        lastFrame = {};
    }
}

Frame Decoder::Impl::frameAtPts(std::int64_t target) {
    // A hardware failure mid-way reopens the codec in software, a container found not to flag keyframes stops the
    // waiting for them; either way the position is then found again.
    for (int attempt = 0; attempt < 3; ++attempt) {
        needReseek = false;
        Frame f = locate(target);
        if (!needReseek) return f;
        prevRaw.reset();
        lastRaw.reset();
        pendingRaw.reset();
        lastFrame = {};
    }
    return {};
}

} // namespace avloop::media
