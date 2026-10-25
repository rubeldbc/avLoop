// The canvas's view (spec §6.3): scene ↔ view, the zoom steps, zoom about a point, pan, Fit and a zoom to a rectangle.
#include "core/Project.h"
#include "ui/canvas/Canvas.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace avloop {

namespace {

// Zoom: 10, 25, 33, 50, 67, 100, 150, 200, 300, 400, 800%.
constexpr std::array<double, 11> kSteps = {0.10, 0.25, 0.3333, 0.50, 0.6667, 1.0, 1.5, 2.0, 3.0, 4.0, 8.0};
constexpr double kFitMargin = 40; // view px around the fitted rectangle

} // namespace

QPointF Canvas::viewToScene(const QPointF& view) const {
    return (view - QPointF(width() / 2.0, height() / 2.0)) / zoom_ + pan_;
}

QPointF Canvas::sceneToView(const QPointF& scene) const {
    return (scene - pan_) * zoom_ + QPointF(width() / 2.0, height() / 2.0);
}

QRectF Canvas::sceneToView(const QRectF& scene) const {
    return QRectF(sceneToView(scene.topLeft()), sceneToView(scene.bottomRight()));
}

void Canvas::setZoom(double zoom, const QPointF& anchorView) {
    const QPointF anchorScene = viewToScene(anchorView);
    zoom_ = std::clamp(zoom, kMinZoom, kMaxZoom);
    // Keep the scene point under the anchor where it was.
    pan_ = anchorScene - (anchorView - QPointF(width() / 2.0, height() / 2.0)) / zoom_;
    fitMode_ = false;
    viewChangedNow();
}

void Canvas::setZoom(double zoom) {
    setZoom(zoom, QPointF(width() / 2.0, height() / 2.0));
}

void Canvas::zoomStep(int direction, const QPointF& anchorView) {
    double next = zoom_;
    if (direction > 0) {
        auto it = std::upper_bound(kSteps.begin(), kSteps.end(), zoom_ * 1.001);
        next = it == kSteps.end() ? kSteps.back() : *it;
    } else {
        auto it = std::lower_bound(kSteps.begin(), kSteps.end(), zoom_ * 0.999);
        next = it == kSteps.begin() ? kSteps.front() : *(it - 1);
    }
    setZoom(next, anchorView);
}

void Canvas::zoomStep(int direction) {
    zoomStep(direction, QPointF(width() / 2.0, height() / 2.0));
}

void Canvas::panBy(const QPointF& viewDelta) {
    pan_ -= viewDelta / zoom_;
    fitMode_ = false;
    viewChangedNow();
}

void Canvas::setPan(const QPointF& scenePoint) {
    pan_ = scenePoint;
    fitMode_ = false;
    viewChangedNow();
}

QRectF Canvas::videoRect() const {
    return project_->transform().rect(project_->source().size());
}

QRectF Canvas::fitTarget() const {
    return videoRect();
}

void Canvas::fitView() {
    const QRectF target = fitTarget();
    if (target.isEmpty() || width() <= 2 * kFitMargin || height() <= 2 * kFitMargin) return;
    zoomToRect(target);
    fitMode_ = true;
}

void Canvas::zoomToRect(const QRectF& scene) {
    if (scene.isEmpty()) return;
    const double z = std::min((width() - 2 * kFitMargin) / scene.width(), (height() - 2 * kFitMargin) / scene.height());
    zoom_ = std::clamp(z, kMinZoom, kMaxZoom);
    pan_ = scene.center();
    fitMode_ = false;
    viewChangedNow();
}

} // namespace avloop
