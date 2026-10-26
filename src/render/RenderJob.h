#pragma once

#include "core/LoopMap.h"
#include "core/ProjectJson.h"
#include "media/MediaInfo.h"

#include <QColor>
#include <QRectF>
#include <QSize>

namespace avloop::render {

// Everything one loop's frames are made from (spec §3.4, AVADR-0001): a snapshot, never the live project — an export
// runs on it while the user edits. Built once; the derived values (output size, the plan, the working size) with it.
struct RenderJob {
    media::MediaInfo source;
    TimeRange range;
    VideoTransform transform;
    CropShape crop;
    LoopSettings loop;
    OutputSettings output;
    QualitySettings quality;
    bool alpha = true; // Alpha Auto until AV-009: needed → kept, else composited over `background`
    QColor background = Qt::black;

    Rational fps;     // the output rate
    QRectF frame;     // the scene rectangle that becomes the output (the crop, else the video's rectangle)
    QSize outputSize; // even, ≥ 16, long side ≤ 1920
    LoopPlan plan;
    QSize workingSize; // what the decoders make: the display size × min(1, effective scale)

    static RenderJob of(const ProjectData& project, const media::MediaInfo& source);
    // The source time of segment frame i (τ(i), spec §8.1).
    double tau(int i) const;
};

// Where one source frame lands in the output (AVADR-0001's shared convention): output pixel centre (x + ½, y + ½) →
// the video's texture coordinates u = ax·x' + bx, v = ay·y' + by (0…1 across the frame, flips folded in); texel
// centres at +½, transparent outside.
struct Placement {
    double ax = 1, bx = 0, ay = 1, by = 0;
    static Placement of(const RenderJob& job);
};

} // namespace avloop::render
