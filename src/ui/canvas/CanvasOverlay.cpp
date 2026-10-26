// What is painted over the frame (spec §6.1, §6.2): the crop in every tool — 60% black outside it, a 1.5 px orange
// outline following its shape, a dashed bounding box for the Ellipse and the Rounded rect — then the active tool's
// overlay (the Selection's border and handles, the crop's handles and guides, the Zoom tool's rectangle).
#include "core/Project.h"
#include "ui/canvas/Canvas.h"
#include "ui/theme/Theme.h"
#include "ui/tools/Tool.h"

#include <QPainter>
#include <QPainterPath>

namespace avloop {

namespace {

QPainterPath shapePath(const CropShape& c, const QRectF& view, double zoom) {
    QPainterPath path;
    switch (c.kind) {
    case CropShape::Kind::Ellipse:
        path.addEllipse(view);
        break;
    case CropShape::Kind::Rounded:
        path.addRoundedRect(view, c.radius * zoom, c.radius * zoom);
        break;
    case CropShape::Kind::Rect:
        path.addRect(view);
        break;
    }
    return path;
}

} // namespace

void Canvas::paintOverlay() {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const CropShape* preview = tool_ ? tool_->previewCrop() : nullptr;
    const CropShape crop = preview ? *preview : project_->crop();
    if (!crop.isEmpty() && project_->source().isValid()) {
        const ThemeTokens& t = Theme::instance().tokens();
        const QRectF r = sceneToView(crop.rect);
        const QPainterPath shape = shapePath(crop, r, zoom_);
        QPainterPath outside;
        outside.addRect(QRectF(rect()).adjusted(-1, -1, 1, 1));
        outside.addPath(shape);
        outside.setFillRule(Qt::OddEvenFill);
        p.fillPath(outside, QColor(0, 0, 0, 153)); // 60% black (spec §6.1)
        p.setPen(QPen(t.accent, 1.5));
        p.setBrush(Qt::NoBrush);
        p.drawPath(shape);
        if (crop.kind != CropShape::Kind::Rect) {
            QColor quiet = t.accent;
            quiet.setAlphaF(0.6f);
            QPen dashed(quiet, 1, Qt::CustomDashLine);
            dashed.setDashPattern({4, 4});
            p.setPen(dashed);
            p.drawRect(r);
        }
    }
    if (tool_) tool_->paintOverlay(p);
}

} // namespace avloop
