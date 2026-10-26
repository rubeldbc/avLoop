#include "render/MaskBuilder.h"

#include <algorithm>
#include <cmath>

namespace avloop::render {

namespace {

// Signed distance from the shape's edge in output px: negative inside.
double distance(CropShape::Kind kind, double hw, double hh, double radius, double px, double py) {
    const double ax = std::abs(px);
    const double ay = std::abs(py);
    if (kind == CropShape::Kind::Ellipse) {
        // An ellipse's distance, close near the edge (k0·(k0 − 1)/k1, the gradient-normalised implicit form).
        const double k0 = std::hypot(ax / hw, ay / hh);
        const double k1 = std::hypot(ax / (hw * hw), ay / (hh * hh));
        return k1 > 0 ? k0 * (k0 - 1.0) / k1 : -std::min(hw, hh);
    }
    const double r = kind == CropShape::Kind::Rounded ? std::clamp(radius, 0.0, std::min(hw, hh)) : 0.0;
    const double qx = ax - (hw - r);
    const double qy = ay - (hh - r);
    const double outside = std::hypot(std::max(qx, 0.0), std::max(qy, 0.0));
    return outside + std::min(std::max(qx, qy), 0.0) - r;
}

} // namespace

double MaskBuilder::coverage(CropShape::Kind kind, const QSize& size, double radius, double feather, int x, int y) {
    const double hw = size.width() / 2.0;
    const double hh = size.height() / 2.0;
    const double d = distance(kind, hw, hh, radius, x + 0.5 - hw, y + 0.5 - hh);
    // At the edge (d = 0) half a pixel; inside over max(feather, 1) px it rises to 1 (spec §6.1).
    return std::clamp((0.5 - d) / std::max(feather, 1.0), 0.0, 1.0);
}

QImage MaskBuilder::build(CropShape::Kind kind, const QSize& size, double radius, double feather) {
    QImage mask(size, QImage::Format_Grayscale8);
    for (int y = 0; y < size.height(); ++y) {
        uchar* row = mask.scanLine(y);
        for (int x = 0; x < size.width(); ++x)
            row[x] = uchar(std::lround(coverage(kind, size, radius, feather, x, y) * 255.0));
    }
    return mask;
}

QImage MaskBuilder::mask(const CropShape& crop, const QSize& outputSize) {
    // The radius is the crop's (scene px); the mask works in output px.
    const double k = crop.isEmpty() ? 1.0 : outputSize.width() / crop.rect.width();
    const double radius = crop.radius * k;
    if (cached_.isNull() || crop.kind != kind_ || outputSize != size_ || radius != radius_ ||
        crop.feather != feather_) {
        cached_ = build(crop.kind, outputSize, radius, crop.feather);
        kind_ = crop.kind;
        size_ = outputSize;
        radius_ = radius;
        feather_ = crop.feather;
        ++builds_;
    }
    return cached_;
}

} // namespace avloop::render
