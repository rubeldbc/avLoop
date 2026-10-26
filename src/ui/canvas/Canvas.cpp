#include "ui/canvas/Canvas.h"
#include "ui/canvas/CanvasGlState.h"

#include "core/Project.h"
#include "ui/tools/Tool.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>

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

void Canvas::mousePressEvent(QMouseEvent* e) {
    setFocus(Qt::MouseFocusReason);
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
        setCursor(tool_->cursor(t));
    else
        tool_->mouseMove(t);
    update();
}

void Canvas::mouseReleaseEvent(QMouseEvent* e) {
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
    if (e->type() == QEvent::ShortcutOverride && tool_ && tool_->wantsKey(static_cast<QKeyEvent*>(e))) {
        e->accept();
        return true;
    }
    return QOpenGLWidget::event(e);
}

void Canvas::keyPressEvent(QKeyEvent* e) {
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
