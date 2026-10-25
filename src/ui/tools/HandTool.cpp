// Hand (H, spec §6.3): a drag pans the view. (A middle-button drag pans in every tool — the canvas's own.)
#include "ui/canvas/Canvas.h"
#include "ui/tools/Tool.h"

namespace avloop {

namespace {

class HandTool : public Tool {
public:
    HandTool(Canvas* canvas, Project* project) : Tool(QStringLiteral("hand"), canvas, project) {}

    void mousePress(const ToolEvent& e) override {
        if (e.button != Qt::LeftButton) return;
        dragging_ = true;
        last_ = e.view;
        canvas_->setCursor(Qt::ClosedHandCursor);
    }
    void mouseMove(const ToolEvent& e) override {
        if (!dragging_) return;
        canvas_->panBy(e.view - last_);
        last_ = e.view;
    }
    void mouseRelease(const ToolEvent&) override {
        dragging_ = false;
        canvas_->setCursor(Qt::OpenHandCursor);
    }
    Qt::CursorShape cursor(const ToolEvent&) const override { return Qt::OpenHandCursor; }

private:
    bool dragging_ = false;
    QPointF last_;
};

} // namespace

Tool* createHandTool(Canvas* canvas, Project* project) {
    return new HandTool(canvas, project);
}

} // namespace avloop
