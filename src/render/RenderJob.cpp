#include "render/RenderJob.h"

#include "core/Geometry.h"
#include "core/TimeMap.h"

#include <algorithm>
#include <cmath>

namespace avloop::render {

RenderJob RenderJob::of(const ProjectData& p, const media::MediaInfo& source) {
    RenderJob job;
    job.source = source;
    job.range = p.range;
    job.transform = p.transform;
    job.keyframes = p.keyframes;
    job.crop = p.crop;
    job.loop = p.loop.clamped();
    job.output = p.output;
    job.quality = p.quality;
    job.effects = p.effects.clamped();
    const QSizeF video(source.width, source.height);
    job.fps = p.output.effectiveFps(source.avgFps.isValid() ? source.avgFps : source.realFps, source.vfr);
    // Without a crop the frame is the video where it stands at In; the crop is never keyframed (spec §1.3).
    job.frame = geometry::exportFrame(p.crop.rect, job.transformAt(p.range.in), video);
    const QList<VideoTransform> extremes = keyframes::extremes(p.keyframes, p.transform);
    job.outputSize = p.output.sizeFor(job.frame.size(), &job.sizeLimited);
    using A = OutputSettings::Alpha;
    // Auto: alpha when the video leaves the output partly clear at any keyframe (between them it never goes further).
    // A format without alpha (MP4) takes the Background under (spec §10.4).
    // The clip's Opacity below 100 % leaves every pixel partly clear.
    const bool see = job.effects.on && job.effects.opacity < 100;
    const bool needed = see || std::any_of(extremes.begin(), extremes.end(), [&](const VideoTransform& t) {
                            return geometry::needsAlpha(p.crop, t, video, source.hasAlpha);
                        });
    job.alpha = formatHasAlpha(p.output.format) && (p.output.alpha == A::On || (p.output.alpha == A::Auto && needed));
    job.background = QColor(p.output.background);
    job.plan = LoopPlan::of(job.loop, job.range.in, job.range.out, job.fps);
    // Decode no larger than the output needs: the display size × min(1, effective scale), long side ≤ 1920 (R-16).
    double largest = 0;
    for (const VideoTransform& t : extremes)
        largest = std::max(largest, geometry::effectiveScale(t, job.frame));
    const double scale = std::min(1.0, largest);
    QSize working(std::max(16, int(std::lround(source.width * scale))),
                  std::max(16, int(std::lround(source.height * scale))));
    if (std::max(working.width(), working.height()) > 1920) working = working.scaled(1920, 1920, Qt::KeepAspectRatio);
    job.workingSize = QSize(working.width() & ~1, working.height() & ~1);
    return job;
}

double RenderJob::tau(int i) const {
    return timemap::tau(range.in, i, loop.speed, fps);
}

VideoTransform RenderJob::transformAt(double seconds) const {
    return keyframes::valueAt(keyframes, transform, seconds);
}

Placement Placement::of(const RenderJob& job) {
    return of(job, job.transform);
}

Placement Placement::of(const RenderJob& job, const VideoTransform& transform) {
    const QRectF v = transform.rect(QSizeF(job.source.width, job.source.height));
    const double W = job.outputSize.width();
    const double H = job.outputSize.height();
    Placement p;
    // Output px → scene: frame.left + x'·frame.w / W; scene → u: (scene − v.left) / v.w.
    p.ax = job.frame.width() / (W * v.width());
    p.bx = (job.frame.left() - v.left()) / v.width();
    p.ay = job.frame.height() / (H * v.height());
    p.by = (job.frame.top() - v.top()) / v.height();
    if (transform.flipH) {
        p.ax = -p.ax;
        p.bx = 1 - p.bx;
    }
    if (transform.flipV) {
        p.ay = -p.ay;
        p.by = 1 - p.by;
    }
    return p;
}

} // namespace avloop::render
