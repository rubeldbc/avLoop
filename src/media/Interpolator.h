#pragma once

#include "core/Rational.h"
#include "media/MediaInfo.h"

#include <QSize>
#include <QString>

#include <functional>

namespace avloop::media {

class DiskCache;

// Motion Interpolation's pre-pass (spec §8.3): the segment's source frames, normalized, through FFmpeg's
// `minterpolate` (mi_mode=mci, mc_mode=aobmc, vsbmc=1) at the output rate, Speed applied by stretching their
// timestamps; frame i of the result is the segment frame at τ(i) = tIn + i × s / fps. A source with alpha has its
// alpha interpolated on a branch of its own. The frames go into the disk cache under a key of every input, so another
// range, Speed, rate or size simply asks for new ones.
class Interpolator {
public:
    struct Job {
        MediaInfo info;
        double tIn = 0;
        int segment = 0; // D
        double speed = 1.0;
        Rational fps{30, 1};
        QSize size; // the working size (empty: the decoder's own)
    };
    // done of total frames after each frame; returning false cancels.
    using Progress = std::function<bool(int done, int total)>;

    explicit Interpolator(DiskCache* cache);

    // Runs on the calling thread (a worker's). False when cancelled or failed (`error` says which).
    bool run(const Job& job, const Progress& progress, QString* error = nullptr);
    // Segment frame i of a job that ran; null when it has not.
    Frame frame(const Job& job, int i) const;
    static QString key(const Job& job, int i);

private:
    DiskCache* cache_;
};

} // namespace avloop::media
