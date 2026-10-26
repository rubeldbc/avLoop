#include "ui/canvas/Canvas.h"
#include "ui/canvas/CanvasGlState.h"

#include "core/Project.h"
#include "ui/tools/Tool.h"

#include <QColor>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>

#include <algorithm>

namespace avloop {

namespace {

ToolEvent toolEvent(const Canvas* c, const QMouseEvent* e) {
    ToolEvent t;
    t.view = e->position();
    t.scene = c->viewToScene(e->position());
    t.button = e->button();
    t.buttons = e->buttons();
    t.modifiers = e->modifiers();
    return t;
}

} // namespace

Canvas::Canvas(Project* project, QWidget* parent) : QOpenGLWidget(parent), project_(project) {
    setObjectName(QStringLiteral("canvas"));
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    connect(project_, &Project::changed, this, [this](Project::Aspect a) {
        if (a == Project::Aspect::Source) fitNextFrame_ = true;
        update();
    });
}

void Canvas::setFrame(const media::Frame& frame) {
    frame_ = frame;
    frameDirty_ = true;
    if (fitNextFrame_ && width() > 0 && !frame_.isNull()) {
        fitNextFrame_ = false;
        fitView();
    }
    update();
}

void Canvas::setTool(Tool* tool) {
    if (tool_ == tool) return;
    if (tool_) tool_->deactivate();
    tool_ = tool;
    if (tool_) tool_->activate();
    unsetCursor();
    update();
}

void Canvas::pickColour(std::function<void(const QColor&)> done) {
    pickDone_ = std::move(done);
    setCursor(Qt::CrossCursor);
    setFocus(Qt::OtherFocusReason); // Esc reaches the canvas
}

QColor Canvas::colourAt(const QPointF& view) const {
    if (frame_.isNull()) return {};
    const QRectF r = videoRect();
    const QPointF p = viewToScene(view);
    double u = (p.x() - r.left()) / r.width();
    double v = (p.y() - r.top()) / r.height();
    if (u < 0 || u >= 1 || v < 0 || v >= 1) return {};
    const VideoTransform& t = project_->transform();
    if (t.flipH) u = 1 - u;
    if (t.flipV) v = 1 - v;
    const int x = std::clamp(int(u * frame_.width), 0, frame_.width - 1);
    const int y = std::clamp(int(v * frame_.height), 0, frame_.height - 1);
    const std::uint8_t* px = frame_.data.get() + std::ptrdiff_t(y) * frame_.stride + x * 4;
    if (px[3] == 0) return {};
    auto straight = [&](int c) { return std::min(255, (px[c] * 255 + px[3] / 2) / px[3]); };
    return QColor(straight(0), straight(1), straight(2));
}

void Canvas::mousePressEvent(QMouseEvent* e) {
    setFocus(Qt::MouseFocusReason);
    if (pickDone_ && e->button() == Qt::LeftButton) {
        const QColor c = colourAt(e->position());
        auto done = std::move(pickDone_);
        pickDone_ = nullptr;
        pickRelease_ = true;
        unsetCursor();
        if (c.isValid()) done(c);
        return;
    }
    if (e->button() == Qt::MiddleButton) { // pan in every tool (spec §6.3)
        panning_ = true;
        lastPanPos_ = e->position();
        setCursor(Qt::ClosedHandCursor);
        return;
    }
    if (tool_) tool_->mousePress(toolEvent(this, e));
    update();
}

void Canvas::mouseMoveEvent(QMouseEvent* e) {
    if (panning_) {
        panBy(e->position() - lastPanPos_);
        lastPanPos_ = e->position();
        return;
    }
    if (!tool_) return;
    const ToolEvent t = toolEvent(this, e);
    if (e->buttons() == Qt::NoButton)
        setCursor(pickDone_ ? QCursor(Qt::CrossCursor) : tool_->cursor(t));
    else
        tool_->mouseMove(t);
    update();
}

void Canvas::mouseReleaseEvent(QMouseEvent* e) {
    if (pickRelease_ && e->button() == Qt::LeftButton) {
        pickRelease_ = false;
        return;
    }
    if (e->button() == Qt::MiddleButton && panning_) {
        panning_ = false;
        unsetCursor();
        return;
    }
    if (tool_) tool_->mouseRelease(toolEvent(this, e));
    update();
}

void Canvas::mouseDoubleClickEvent(QMouseEvent* e) {
    if (tool_ && e->button() == Qt::LeftButton) tool_->mouseDoubleClick(toolEvent(this, e));
    update();
}

void Canvas::wheelEvent(QWheelEvent* e) {
    const QPoint delta = e->angleDelta();
    const int notch = delta.y() != 0 ? delta.y() : delta.x();
    if ((e->modifiers() & Qt::ShiftModifier) || delta.y() == 0) {
        panBy(QPointF(notch / 2.0, 0)); // across (Windows turns Shift+wheel sideways)
    } else {
        // The wheel (Ctrl or not) zooms about the cursor, a step a notch; a finer wheel or a touchpad adds up to one.
        if (wheelZoom_ * notch < 0) wheelZoom_ = 0;
        wheelZoom_ += notch;
        for (; wheelZoom_ >= QWheelEvent::DefaultDeltasPerStep; wheelZoom_ -= QWheelEvent::DefaultDeltasPerStep)
            zoomStep(1, e->position());
        for (; wheelZoom_ <= -QWheelEvent::DefaultDeltasPerStep; wheelZoom_ += QWheelEvent::DefaultDeltasPerStep)
            zoomStep(-1, e->position());
    }
    e->accept();
}

bool Canvas::event(QEvent* e) {
    // The active tool's keys come before the window's shortcuts: the crop's arrows, not the timeline's frame steps.
    // While picking, Esc is the canvas's (it cancels the pick).
    if (e->type() == QEvent::ShortcutOverride && pickDone_ && static_cast<QKeyEvent*>(e)->key() == Qt::Key_Escape) {
        e->accept();
        return true;
    }
    if (e->type() == QEvent::ShortcutOverride && tool_ && tool_->wantsKey(static_cast<QKeyEvent*>(e))) {
        e->accept();
        return true;
    }
    return QOpenGLWidget::event(e);
}

void Canvas::keyPressEvent(QKeyEvent* e) {
    if (pickDone_ && e->key() == Qt::Key_Escape) {
        pickDone_ = nullptr;
        unsetCursor();
        return;
    }
    if (tool_ && tool_->keyPress(e)) {
        update();
        return;
    }
    QOpenGLWidget::keyPressEvent(e);
}

void Canvas::viewChangedNow() {
    ViewState v = project_->view();
    v.zoom = zoom_;
    v.panX = pan_.x();
    v.panY = pan_.y();
    project_->setView(v);
    emit viewChanged();
    update();
}

} // namespace avloop
