#pragma once

#include "core/ClipEffects.h"
#include "media/MediaInfo.h"

#include <QVector>

#include <cstdint>
#include <vector>

namespace avloop::render {

// The clip's effects on one source frame, on the CPU (D-017): what the export, the Preview and avloop-cli draw. Both
// back ends are given the frame already made, so the GPU and the CPU agree by construction (AVADR-0001). Blur is
// three box passes each way — a close Gaussian, in the same time at any radius —, sharpen an unsharp mask of σ 1
// source px, both premultiplied with the edges held; then each pixel straight by ClipEffects. Rows run in parallel;
// the scratch (three frames at most) is kept from one frame to the next.
class EffectsPass {
public:
    // `in` with `effects` applied: a new frame of its size, pts and time — or `in` itself when they do nothing.
    // `pxScale`: the frame's pixels a source pixel (its width over the source's), for blur and sharpen.
    media::Frame apply(const ClipEffects& effects, const media::Frame& in, double pxScale);

private:
    struct Plane {
        const std::uint8_t* data = nullptr;
        int stride = 0;
    };
    // A Gaussian of σ px of `src` (w × h RGBA8), made in `one` and `two` by turns; returns where it ended.
    Plane gauss(Plane src, int w, int h, double sigma, std::vector<std::uint8_t>& one, std::vector<std::uint8_t>& two);

    std::vector<std::uint8_t> a_, b_, c_;
    QVector<int> rows_, strips_;
};

} // namespace avloop::render
