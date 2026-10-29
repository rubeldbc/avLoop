#pragma once

#include <QObject>
#include <QPointF>
#include <QString>

#include <functional>

class QKeyEvent;
class QPainter;
class QWidget;

namespace avloop {

class Canvas;
class Project;
struct CropShape;

// A pointer event on the canvas, in scene and view coordinates.
struct ToolEvent {
    QPointF scene;
    QPointF view;
    Qt::MouseButton button = Qt::NoButton;
    Qt::MouseButtons buttons;
    Qt::KeyboardModifiers modifiers;
};

// One tool of the rail (spec §6): only one is active; it gets the canvas's pointer and key events in scene space,
// paints its overlay over the frame and gives the options bar its widget.
class Tool : public QObject {
    Q_OBJECT
public:
    Tool(QString id, Canvas* canvas, Project* project, QObject* parent = nullptr)
        : QObject(parent), id_(std::move(id)), canvas_(canvas), project_(project) {}

    QString id() const { return id_; }

    virtual void activate() {}
    virtual void deactivate() {}
    virtual void mousePress(const ToolEvent&) {}
    virtual void mouseMove(const ToolEvent&) {}
    virtual void mouseRelease(const ToolEvent&) {}
    virtual void mouseDoubleClick(const ToolEvent&) {}
    virtual bool keyPress(QKeyEvent*) { return false; }
    // A key the tool takes before the window's shortcuts (the crop's arrows over the timeline's frame steps).
    virtual bool wantsKey(const QKeyEvent*) const { return false; }
    virtual void paintOverlay(QPainter&) {}
    virtual Qt::CursorShape cursor(const ToolEvent&) const { return Qt::ArrowCursor; }
    // A crop being drawn, shown instead of the project's until it is committed (the crop tools).
    virtual const CropShape* previewCrop() const { return nullptr; }
    // The options bar's widget for this tool (spec §6's table); null when it has none.
    virtual QWidget* createOptions(QWidget* parent) {
        Q_UNUSED(parent);
        return nullptr;
    }

protected:
    QString id_;
    Canvas* canvas_;
    Project* project_;
};

// The tools of this phase (SelectionTool.cpp, ZoomTool.cpp, HandTool.cpp).
Tool* createSelectionTool(Canvas* canvas, Project* project);
Tool* createZoomTool(Canvas* canvas, Project* project);
Tool* createHandTool(Canvas* canvas, Project* project);
// CropTool.cpp: one tool a kind — M Rect, U Rounded, E Ellipse.
Tool* createCropTool(int kind, Canvas* canvas, Project* project);
// ShapeTool.cpp: S — shapes inside the crop's frame that select or cut (D-022).
Tool* createShapeTool(Canvas* canvas, Project* project);
// InOutTool.cpp: "in" or "out" — a click on the timeline sets it there (spec §6.4).
Tool* createInOutTool(bool out, Canvas* canvas, Project* project);
// KeyframeTool.cpp: `seek` takes the playhead to a keyframe (◀ / ▶).
Tool* createKeyframeTool(Canvas* canvas, Project* project, std::function<void(double)> seek);

} // namespace avloop
