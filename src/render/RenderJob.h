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
    VideoTransform transform; // without keyframes; the flips always
    Keyframes keyframes;      // position and scale over source time (spec §6.4)
    CropShape crop;
    LoopSettings loop;
    OutputSettings output;
    QualitySettings quality;
    bool alpha = true; // Alpha: Auto (needed → kept), On, Off (composited over `background`)
    QColor background = Qt::black;
    bool sizeLimited = false; // the long-side limit of 1920 took the asked size down (spec §10.2's note)

    Rational fps;     // the output rate
    QRectF frame;     // the scene rectangle that becomes the output (the crop, else the video's rectangle)
    QSize outputSize; // even, ≥ 16, long side ≤ 1920
    LoopPlan plan;
    QSize workingSize; // what the decoders make: the display size × min(1, effective scale)

    static RenderJob of(const ProjectData& project, const media::MediaInfo& source);
    // The source time of segment frame i (τ(i), spec §8.1).
    double tau(int i) const;
    // Where the video stands at a source time: each source frame drawn takes its own (spec §3.4 step 4).
    VideoTransform transformAt(double seconds) const;
};

// Where one source frame lands in the output (AVADR-0001's shared convention): output pixel centre (x + ½, y + ½) →
// the video's texture coordinates u = ax·x' + bx, v = ay·y' + by (0…1 across the frame, flips folded in); texel
// centres at +½, transparent outside.
struct Placement {
    double ax = 1, bx = 0, ay = 1, by = 0;
    static Placement of(const RenderJob& job); // the transform without keyframes
    static Placement of(const RenderJob& job, const VideoTransform& transform);
};

} // namespace avloop::render
