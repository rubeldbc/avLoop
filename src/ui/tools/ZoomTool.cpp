// Zoom (Z, spec §6.3): a click zooms in a step about the point, Alt+click out, a drag zooms to the rectangle. The view
// only, never the output and never Undo.
#include "ui/canvas/Canvas.h"
#include "ui/options/OptionsBar.h"
#include "ui/theme/Theme.h"
#include "ui/tools/Tool.h"

#include <QPainter>

namespace avloop {

namespace {

constexpr double kDragThreshold = 4; // px: below it a drag is a click

class ZoomTool : public Tool {
public:
    ZoomTool(Canvas* canvas, Project* project) : Tool(QStringLiteral("zoom"), canvas, project) {}

    void mousePress(const ToolEvent& e) override {
        if (e.button != Qt::LeftButton) return;
        pressed_ = true;
        from_ = to_ = e.view;
    }
    void mouseMove(const ToolEvent& e) override {
        if (pressed_) to_ = e.view;
    }
    void mouseRelease(const ToolEvent& e) override {
        if (!pressed_) return;
        pressed_ = false;
        to_ = e.view;
        const QRectF r = QRectF(from_, to_).normalized();
        if (r.width() > kDragThreshold && r.height() > kDragThreshold) {
            canvas_->zoomToRect(QRectF(canvas_->viewToScene(r.topLeft()), canvas_->viewToScene(r.bottomRight())));
        } else {
            canvas_->zoomStep((e.modifiers & Qt::AltModifier) ? -1 : 1, e.view);
        }
    }
    Qt::CursorShape cursor(const ToolEvent&) const override { return Qt::CrossCursor; }
    void paintOverlay(QPainter& p) override {
        if (!pressed_) return;
        QPen pen(Theme::instance().tokens().text, 1, Qt::DashLine);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        p.drawRect(QRectF(from_, to_).normalized());
    }
    QWidget* createOptions(QWidget* parent) override { return createZoomOptions(canvas_, parent); }

private:
    bool pressed_ = false;
    QPointF from_;
    QPointF to_;
};

} // namespace

Tool* createZoomTool(Canvas* canvas, Project* project) {
    return new ZoomTool(canvas, project);
}

} // namespace avloop
