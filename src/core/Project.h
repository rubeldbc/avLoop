#pragma once

#include "core/CropShape.h"
#include "core/LoopSettings.h"
#include "core/Rational.h"
#include "core/TimeRange.h"
#include "core/VideoTransform.h"

#include <QDateTime>
#include <QJsonObject>
#include <QObject>
#include <QSizeF>
#include <QString>
#include <QUndoStack>

namespace avloop {

struct ProjectData;

// The source as the project knows it (spec §11.1 "source"): enough to find it again and to lay it in scene space.
struct SourceInfo {
    QString path;
    int width = 0; // display size (rotation and SAR applied)
    int height = 0;
    Rational fps;
    bool vfr = false;
    double duration = 0;
    qint64 frameCount = 0;
    bool hasAlpha = false;
    qint64 fileSize = 0;
    QDateTime modified;
    int stream = 0;

    bool isValid() const { return !path.isEmpty() && width > 0 && height > 0; }
    QSizeF size() const { return QSizeF(width, height); }
};

// What the canvas shows of the scene; not part of Undo (spec §11.2).
struct ViewState {
    double zoom = 1.0;
    double panX = 0; // the scene point at the canvas's centre
    double panY = 0;
    QString tool = QStringLiteral("select");
};

// The project model (spec §3.2): it changes only through Undo commands, and says what changed by `changed(Aspect)`.
// Undo keeps 200 steps of values only, never images (spec §3.7, §11.2).
class Project : public QObject {
    Q_OBJECT
public:
    enum class Aspect { Source, Range, Transform, Crop, Loop, Output, Quality, Keyframes, View };
    Q_ENUM(Aspect)
    static constexpr int kUndoLimit = 200;

    explicit Project(QObject* parent = nullptr);

    const SourceInfo& source() const { return source_; }
    const TimeRange& range() const { return range_; }
    const VideoTransform& transform() const { return transform_; }
    const CropShape& crop() const { return crop_; }
    const LoopSettings& loop() const { return loop_; }
    const OutputSettings& output() const { return output_; }
    const QualitySettings& quality() const { return quality_; }
    // The output frame grid In, Out and the playhead snap to (spec §6.4, §8.3): the Output section's FPS.
    Rational outputFps() const;
    const ViewState& view() const { return view_; }
    const QJsonObject& extra() const { return extra_; }
    QUndoStack* undoStack() { return &undo_; }

    // A new source starts a new document: the transform at its initial place and Undo cleared.
    void setSource(const SourceInfo& source);
    // Everything as a value (to save, to snapshot for an export) and back (a project read: Undo cleared).
    ProjectData snapshot() const;
    void restore(const ProjectData& data);
    // The view and the unknown keys are kept outside Undo.
    void setView(const ViewState& view);
    void setExtra(const QJsonObject& extra);

    // Pushes the change as one Undo step; changes under the same merge key within 500 ms (or one drag) merge.
    void changeTransform(const VideoTransform& after, const QString& mergeKey = {});
    void changeCrop(const CropShape& after, const QString& mergeKey = {});
    void changeRange(const TimeRange& after, const QString& mergeKey = {});
    void changeLoop(const LoopSettings& after, const QString& mergeKey = {});
    // A new frame rate keeps In and Out in seconds, on the new grid (one Undo step with the range — reading R-29).
    void changeOutput(const OutputSettings& after, const QString& mergeKey = {});
    void changeQuality(const QualitySettings& after, const QString& mergeKey = {});

    // Only commands call these.
    void applyTransform(const VideoTransform& t);
    void applyCrop(const CropShape& c);
    void applyRange(const TimeRange& r);
    void applyLoop(const LoopSettings& l);
    void applyOutput(const OutputSettings& o);
    void applyQuality(const QualitySettings& q);

signals:
    void changed(avloop::Project::Aspect aspect);

private:
    SourceInfo source_;
    TimeRange range_;
    VideoTransform transform_;
    CropShape crop_;
    LoopSettings loop_;
    OutputSettings output_;
    QualitySettings quality_;
    ViewState view_;
    QJsonObject extra_;
    QUndoStack undo_;
};

} // namespace avloop
