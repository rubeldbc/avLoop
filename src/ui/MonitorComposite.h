#pragma once

#include <QImage>
#include <QObject>
#include <QRectF>

#include <memory>

class QPainter;

namespace avloop {

class Canvas;
class Project;

namespace render {
class SequenceRenderer;
} // namespace render

// The Program monitor's composite (D-033; MainWindowMonitor.cpp): when the project is more than AvLoop's one video,
// the canvas hides its GL video and paints this — the sequence at the playhead over the frame and every clip present.
class MonitorComposite : public QObject {
    Q_OBJECT
public:
    MonitorComposite(Project* project, Canvas* canvas, QObject* parent = nullptr);
    ~MonitorComposite() override;

    bool active() const;
    // Paints the last composite at its scene place; a new one is made right after when the playhead moved.
    void paint(QPainter& p);

private:
    void make();

    Project* project_;
    Canvas* canvas_;
    std::unique_ptr<render::SequenceRenderer> renderer_;
    QImage image_;
    QRectF rect_;
    double made_ = -1;
    bool pending_ = false;
};

} // namespace avloop
