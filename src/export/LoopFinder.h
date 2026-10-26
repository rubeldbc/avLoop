#pragma once

#include "core/Keyframes.h"
#include "core/Rational.h"
#include "media/MediaInfo.h"

#include <QImage>
#include <QList>
#include <QRectF>
#include <QString>

#include <atomic>
#include <cstdint>
#include <functional>
#include <vector>

namespace avloop::exporting {

// The Auto Loop Finder (spec §11.5): every frame of the search range on the output grid reduced to a 64×64 greyscale
// fingerprint of what will be exported (the export's frame, the video where it stands — keyframes too — at that
// time; 4 KB a frame), then every pair (a, b) with a length within the bounds scored — the mean absolute difference of
// frames a and b plus that of their motions (a − (a−1) against b − (b−1)); lower is better. The best five, no two
// within a second at both ends, are the answer, with a 0–100 score (reading R-50).
class LoopFinder {
public:
    static constexpr int kSide = 64;
    static constexpr int kPrint = kSide * kSide;

    struct Input {
        media::MediaInfo source;
        QRectF frame;             // the export's frame in scene px
        VideoTransform transform; // without keyframes; the flips
        Keyframes keyframes;
        Rational fps{30, 1}; // the output grid
        double from = 0;     // the search range, seconds
        double to = 0;
        double minLength = 2; // seconds
        double maxLength = 10;
    };
    struct Candidate {
        double in = 0; // seconds: a loop of In … Out, Out the frame after its last (as the range)
        double out = 0;
        double raw = 0; // the score before scaling: grey levels
        int score = 0;  // 0–100, higher is better
        QImage first;   // 120×68 thumbnails of both ends
        QImage last;
    };
    using Progress = std::function<void(int done, int total)>;

    // The fingerprints of [from, to) on the grid, from a decoder of its own; false when cancelled or unreadable.
    static bool fingerprints(const Input& in, std::vector<std::uint8_t>& prints, int& frames,
                             const std::atomic<bool>& cancel, const Progress& progress = {}, QString* error = nullptr);
    // The best `count` pairs of `frames` fingerprints with b − a in [minFrames, maxFrames], none within `apart`
    // frames of a better one at both ends; scored in parallel, rows of a. Empty when cancelled.
    static QList<Candidate> best(const std::vector<std::uint8_t>& prints, int frames, int minFrames, int maxFrames,
                                 int apart, int count, const std::atomic<bool>& cancel, const Progress& progress = {});
    // The whole search, the thumbnails of the answer's ends included.
    static QList<Candidate> find(const Input& in, const std::atomic<bool>& cancel, const Progress& progress = {},
                                 QString* error = nullptr);
};

} // namespace avloop::exporting
