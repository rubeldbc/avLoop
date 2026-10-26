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
    job.crop = p.crop;
    job.loop = p.loop.clamped();
    job.output = p.output;
    const QSizeF video(source.width, source.height);
    job.fps = p.output.effectiveFps(source.avgFps.isValid() ? source.avgFps : source.realFps, source.vfr);
    job.frame = geometry::exportFrame(p.crop.rect, p.transform, video);
    job.outputSize = geometry::outputSize(job.frame.size());
    job.alpha = geometry::needsAlpha(p.crop, p.transform, video, source.hasAlpha);
    job.plan = LoopPlan::of(job.loop, job.range.in, job.range.out, job.fps);
    // Decode no larger than the output needs: the display size × min(1, effective scale), long side ≤ 1920 (R-16).
    const double scale = std::min(1.0, geometry::effectiveScale(p.transform, job.frame));
    QSize working(std::max(16, int(std::lround(source.width * scale))),
                  std::max(16, int(std::lround(source.height * scale))));
    if (std::max(working.width(), working.height()) > 1920) working = working.scaled(1920, 1920, Qt::KeepAspectRatio);
    job.workingSize = QSize(working.width() & ~1, working.height() & ~1);
    return job;
}

double RenderJob::tau(int i) const {
    return timemap::tau(range.in, i, loop.speed, fps);
}

Placement Placement::of(const RenderJob& job) {
    const QRectF v = job.transform.rect(QSizeF(job.source.width, job.source.height));
    const double W = job.outputSize.width();
    const double H = job.outputSize.height();
    Placement p;
    // Output px → scene: frame.left + x'·frame.w / W; scene → u: (scene − v.left) / v.w.
    p.ax = job.frame.width() / (W * v.width());
    p.bx = (job.frame.left() - v.left()) / v.width();
    p.ay = job.frame.height() / (H * v.height());
    p.by = (job.frame.top() - v.top()) / v.height();
    if (job.transform.flipH) {
        p.ax = -p.ax;
        p.bx = 1 - p.bx;
    }
    if (job.transform.flipV) {
        p.ay = -p.ay;
        p.by = 1 - p.by;
    }
    return p;
}

} // namespace avloop::render
