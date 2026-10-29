#include "render/MaskBuilder.h"

#include <QPainter>

#include <vector>

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

QImage MaskBuilder::shapeCoverage(const MaskShape& shape, const QRectF& frame, const QSize& size) {
    // Drawn white on clear into ARGB (QPainter's anti-aliasing), its alpha taken: scene px → output px.
    QImage layer(size, QImage::Format_ARGB32_Premultiplied);
    layer.fill(Qt::transparent);
    if (!frame.isEmpty()) {
        QPainter p(&layer);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(Qt::NoPen);
        p.setBrush(Qt::white);
        const double kx = size.width() / frame.width();
        const double ky = size.height() / frame.height();
        p.setTransform(QTransform(kx, 0, 0, ky, -frame.left() * kx, -frame.top() * ky));
        p.drawPath(shape.path());
    }
    QImage out(size, QImage::Format_Grayscale8);
    for (int y = 0; y < size.height(); ++y) {
        const auto* in = reinterpret_cast<const QRgb*>(layer.constScanLine(y));
        uchar* row = out.scanLine(y);
        for (int x = 0; x < size.width(); ++x)
            row[x] = uchar(qAlpha(in[x]));
    }
    return out;
}

namespace {

// The crop's own coverage with the shapes joined into it, bottom to top (D-022).
QImage withShapes(const QImage& own, const CropShape& crop, const QRectF& frame, const QSize& size) {
    const qsizetype n = qsizetype(size.width()) * size.height();
    std::vector<float> running(size_t(n), float(maskshapes::start(crop.shapes)));
    for (const MaskShape& shape : crop.shapes) {
        if (!shape.visible) continue;
        const QImage s = MaskBuilder::shapeCoverage(shape, frame, size);
        const float strength = float(shape.strength / 100.0);
        for (int y = 0; y < size.height(); ++y) {
            const uchar* row = s.constScanLine(y);
            float* r = running.data() + qsizetype(y) * size.width();
            for (int x = 0; x < size.width(); ++x)
                r[x] = float(maskshapes::join(shape.role, r[x], row[x] / 255.f * strength));
        }
    }
    QImage out(size, QImage::Format_Grayscale8);
    for (int y = 0; y < size.height(); ++y) {
        const uchar* base = own.constScanLine(y);
        const float* r = running.data() + qsizetype(y) * size.width();
        uchar* row = out.scanLine(y);
        for (int x = 0; x < size.width(); ++x)
            row[x] = uchar(std::lround(std::clamp(r[x], 0.f, 1.f) * base[x]));
    }
    return out;
}

} // namespace

QImage MaskBuilder::build(const CropShape& crop, const QRectF& frame, const QSize& size) {
    if (!crop.shapes.isEmpty()) {
        CropShape alone = crop;
        alone.shapes.clear();
        QImage own(size, QImage::Format_Grayscale8);
        if (crop.isEmpty())
            own.fill(255); // no crop: the whole frame, then the shapes
        else
            own = build(alone, frame, size);
        return withShapes(own, crop, frame, size);
    }
    if (!crop.invert || frame.isEmpty()) {
        // The radius is the crop's (scene px); the mask works in output px.
        const double k = crop.isEmpty() ? 1.0 : size.width() / crop.rect.width();
        return build(crop.kind, size, crop.radius * k, crop.feather);
    }
    // The hole: the shape at its place in the frame, in output px; outside it rises from 0 to 1 over the feather.
    const double kx = size.width() / frame.width();
    const double ky = size.height() / frame.height();
    const double cx = (crop.rect.center().x() - frame.left()) * kx;
    const double cy = (crop.rect.center().y() - frame.top()) * ky;
    const double hw = crop.rect.width() / 2 * kx;
    const double hh = crop.rect.height() / 2 * ky;
    const double radius = crop.radius * std::min(kx, ky);
    const double soft = std::max(crop.feather, 1.0);
    QImage mask(size, QImage::Format_Grayscale8);
    for (int y = 0; y < size.height(); ++y) {
        uchar* row = mask.scanLine(y);
        for (int x = 0; x < size.width(); ++x) {
            const double d = distance(crop.kind, hw, hh, radius, x + 0.5 - cx, y + 0.5 - cy);
            row[x] = uchar(std::lround(std::clamp((d + 0.5) / soft, 0.0, 1.0) * 255.0));
        }
    }
    return mask;
}

QImage MaskBuilder::mask(const CropShape& crop, const QRectF& frame, const QSize& outputSize) {
    if (cached_.isNull() || !(crop == crop_) || frame != frame_ || outputSize != size_) {
        cached_ = build(crop, frame, outputSize);
        crop_ = crop;
        frame_ = frame;
        size_ = outputSize;
        ++builds_;
    }
    return cached_;
}

} // namespace avloop::render
