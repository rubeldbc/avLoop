// What is painted over the frame (spec §6.1, §6.2): the crop in every tool — 60% black outside it (inside it when
// inverted, D-021, the export's frame outlined), a 1.5 px orange
// outline following its shape, a dashed bounding box for the Ellipse and the Rounded rect — then the active tool's
// overlay (the Selection's border and handles, the crop's handles and guides, the Zoom tool's rectangle).
#include "core/Project.h"
#include "ui/canvas/Canvas.h"
#include "ui/theme/Theme.h"
#include "ui/tools/Tool.h"

#include <QPainter>
#include <QPainterPath>
#include <QTransform>

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

// What the shapes keep of the frame (D-022), in view px: their outlines joined bottom to top by their roles — the dim's
// hard edge (feather and strength are the Preview's, R-22).
QPainterPath keptByShapes(const QList<MaskShape>& shapes, const QTransform& toView, const QRectF& frameView) {
    QPainterPath frame;
    frame.addRect(frameView);
    QPainterPath kept = maskshapes::start(shapes) > 0 ? frame : QPainterPath();
    for (const MaskShape& s : shapes) {
        if (!s.visible) continue;
        const QPainterPath p = toView.map(s.path());
        switch (s.role) {
        case MaskShape::Role::Add:
            kept = kept.united(p);
            break;
        case MaskShape::Role::Cut:
            if (s.strength >= 100) kept = kept.subtracted(p); // a weaker cut is dimmed by its strength, after
            break;
        case MaskShape::Role::KeepOnly:
            kept = kept.intersected(p);
            break;
        case MaskShape::Role::Exclude:
            kept = kept.united(p).subtracted(kept.intersected(p));
            break;
        }
    }
    return kept.intersected(frame);
}

} // namespace

void Canvas::paintOverlay() {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const CropShape* preview = tool_ ? tool_->previewCrop() : nullptr;
    const CropShape crop = preview ? *preview : project_->crop();
    if (maskshapes::anyVisible(crop.shapes) && project_->source().isValid()) {
        paintShapes(p, crop);
    } else if (!crop.isEmpty() && project_->source().isValid()) {
        const ThemeTokens& t = Theme::instance().tokens();
        const QRectF r = sceneToView(crop.rect);
        const QPainterPath shape = shapePath(crop, r, zoom_);
        if (crop.invert) {
            // The hole is dimmed (D-021); the export's frame — the video where it stands — outlined thinly.
            p.fillPath(shape, QColor(0, 0, 0, 153));
            QColor frame = t.accent;
            frame.setAlphaF(0.5f);
            p.setPen(QPen(frame, 1));
            p.drawRect(sceneToView(project_->exportFrame()));
        } else {
            QPainterPath outside;
            outside.addRect(QRectF(rect()).adjusted(-1, -1, 1, 1));
            outside.addPath(shape);
            outside.setFillRule(Qt::OddEvenFill);
            p.fillPath(outside, QColor(0, 0, 0, 153)); // 60% black (spec §6.1)
        }
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

QTransform Canvas::sceneToViewTransform() const {
    const QPointF o = sceneToView(QPointF(0, 0));
    return QTransform(zoom_, 0, 0, zoom_, o.x(), o.y());
}

void Canvas::paintShapes(QPainter& p, const CropShape& crop) {
    const ThemeTokens& t = Theme::instance().tokens();
    const QTransform toView = sceneToViewTransform();
    const QRectF frameView = sceneToView(project_->exportFrame());
    QPainterPath kept = keptByShapes(crop.shapes, toView, frameView);
    if (!crop.isEmpty()) {
        const QPainterPath own = shapePath(crop, sceneToView(crop.rect), zoom_);
        kept = crop.invert ? kept.subtracted(own) : kept.intersected(own);
    }
    QPainterPath dim;
    dim.addRect(QRectF(rect()).adjusted(-1, -1, 1, 1));
    p.fillPath(dim.subtracted(kept), QColor(0, 0, 0, 153)); // 60% black over what is not kept
    // A Cut below full strength (D-024): its part of what is kept dimmed by its strength — a hint; the Preview is
    // exact.
    for (const MaskShape& s : crop.shapes)
        if (s.visible && s.role == MaskShape::Role::Cut && s.strength < 100)
            p.fillPath(kept.intersected(toView.map(s.path())), QColor(0, 0, 0, int(153 * s.strength / 100)));
    p.setBrush(Qt::NoBrush);
    if (!crop.isEmpty()) {
        p.setPen(QPen(t.accent, 1.5));
        p.drawPath(shapePath(crop, sceneToView(crop.rect), zoom_));
    }
    // Each visible shape's outline: an adding one solid, the others dashed.
    for (const MaskShape& s : crop.shapes) {
        if (!s.visible) continue;
        QPen pen(t.accent, 1.25);
        if (s.role != MaskShape::Role::Add) pen.setStyle(Qt::DashLine);
        p.setPen(pen);
        p.drawPath(toView.map(s.path()));
    }
}

} // namespace avloop
