#pragma once

#include "media/MediaInfo.h"
#include "render/EffectsPass.h"

#include <QOpenGLWidget>
#include <QPointF>
#include <QRectF>
#include <QTransform>

class QPainter;

#include <functional>
#include <memory>

namespace avloop {

struct CropShape;

class Project;
class Tool;

// The canvas (spec §3.3, §6.3): an unbounded scene in pixels drawn by OpenGL 3.3 (the transform shader the renderer
// shares, AVADR-0001), with a QPainter overlay for the tools. Zoom and pan are the view's, never in Undo; the wheel
// (zoom about the cursor, Ctrl or not), Shift+wheel (pan across) and middle-drag (pan) work in every tool.
class Canvas : public QOpenGLWidget {
    Q_OBJECT
public:
    static constexpr double kMinZoom = 0.05;
    static constexpr double kMaxZoom = 8.0;

    explicit Canvas(Project* project, QWidget* parent = nullptr);
    ~Canvas() override;

    void setFrame(const media::Frame& frame);
    bool hasFrame() const { return !frame_.isNull(); }
    void setTool(Tool* tool);
    Tool* tool() const { return tool_; }

    // The view (CanvasView.cpp)
    double zoom() const { return zoom_; }
    QPointF pan() const { return pan_; }
    QPointF viewToScene(const QPointF& view) const;
    QPointF sceneToView(const QPointF& scene) const;
    QRectF sceneToView(const QRectF& scene) const;
    // The scene → view mapping as a transform (zoom and pan).
    QTransform sceneToViewTransform() const;
    void setZoom(double zoom, const QPointF& anchorView);
    void setZoom(double zoom);
    void zoomStep(int direction, const QPointF& anchorView);
    void zoomStep(int direction);
    void panBy(const QPointF& viewDelta);
    void setPan(const QPointF& scenePoint);
    void fitView();
    void zoomToRect(const QRectF& scene);
    // What Fit shows: the crop, else the video rectangle.
    QRectF fitTarget() const;
    QRectF videoRect() const;
    bool glReady() const;
    // The eyedropper (D-018): the next click on the video gives its colour there, as the clip is (before effects);
    // Esc or a click beside the video gives none. Overrides the tool for that one click.
    void pickColour(std::function<void(const QColor&)> done);
    bool picking() const { return bool(pickDone_); }

signals:
    void viewChanged();

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    bool event(QEvent* event) override;

private:
    struct Gl;
    void paintScene();                                    // CanvasGl.cpp
    void paintFallback();                                 // CanvasGl.cpp: QPainter only, when OpenGL 3.3 is missing
    void setEffectUniforms();                             // CanvasGl.cpp: the clip effects on the video
    void paintOverlay();                                  // CanvasOverlay.cpp
    void paintShapes(QPainter& p, const CropShape& crop); // the dim and outlines with shapes (D-022)
    void releaseGl();                                     // CanvasGl.cpp
    void viewChangedNow();

    Project* project_;
    Tool* tool_ = nullptr;
    media::Frame frame_;
    bool frameDirty_ = false;
    std::unique_ptr<Gl> gl_;
    render::EffectsPass fallbackPass_; // without OpenGL 3.3, the clip effects on the CPU …
    media::Frame fallbackFrame_;       // … made once a frame and a look
    media::Frame fallbackFrom_;        // held, so its buffer is never another frame's
    std::uint64_t fallbackKey_ = 0;
    double zoom_ = 1.0;
    QPointF pan_; // the scene point at the canvas's centre
    bool panning_ = false;
    QPointF lastPanPos_;
    std::function<void(const QColor&)> pickDone_;
    bool pickRelease_ = false;                  // the pick's click: its release is not the tool's
    QColor colourAt(const QPointF& view) const; // the clip's straight colour under a view point, invalid beside it
    int wheelZoom_ = 0;                         // the wheel's eighths of a degree not yet a zoom step
    bool fitMode_ = true;                       // Fit holds until the user zooms or pans; a resize fits again
    bool fitNextFrame_ = true;                  // a new source: its first frame is fitted
};

} // namespace avloop
