#pragma once

#include "core/ClipEffects.h"
#include "core/CropShape.h"
#include "core/Keyframes.h"
#include "core/LoopSettings.h"
#include "core/Rational.h"
#include "core/TimeRange.h"
#include "core/VideoLayer.h"
#include "core/VideoTransform.h"

#include <QDateTime>
#include <QJsonObject>
#include <QObject>
#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QString>
#include <QUndoStack>

namespace avloop {

struct ProjectData;
struct Sequence;

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
    // Time: the playhead moved and, with keyframes, the transform shown with it (nothing to export changed).
    // Shapes: which shapes are selected (D-022; not in Undo, not in the file).
    // Layers: the video layers above the main video (D-027); LayerSelection: which one is selected (not in Undo).
    enum class Aspect {
        Source,
        Range,
        Transform,
        Crop,
        Loop,
        Output,
        Quality,
        Keyframes,
        View,
        Time,
        Effects,
        Shapes,
        Layers,
        LayerSelection
    };
    Q_ENUM(Aspect)
    static constexpr int kUndoLimit = 200;

    explicit Project(QObject* parent = nullptr);

    const SourceInfo& source() const { return source_; }
    const TimeRange& range() const { return range_; }
    // The transform at the playhead: the keyframes' value there, else the one transform (spec §6.2, §6.4).
    const VideoTransform& transform() const { return current_; }
    // The transform without keyframes, and the flips always (they hold — reading R-47).
    const VideoTransform& baseTransform() const { return transform_; }
    VideoTransform transformAt(double seconds) const;
    const Keyframes& keyframes() const { return keyframes_; }
    double time() const { return time_; }
    // The playhead (no Undo): with keyframes the transform shown follows it.
    void setTime(double seconds);
    double keyTolerance() const; // half an output frame: one keyframe a frame
    int keyframeAtTime() const;  // the keyframe at the playhead, else −1
    const CropShape& crop() const { return crop_; }
    // The shapes the Shape tool, its bar and the Crop section act on: valid indices, ascending.
    QList<int> selectedShapes() const;
    void setSelectedShapes(const QList<int>& indices);
    const LoopSettings& loop() const { return loop_; }
    const OutputSettings& output() const { return output_; }
    const QualitySettings& quality() const { return quality_; }
    const ClipEffects& effects() const { return effects_; }
    // The videos on the tracks above the main one (D-027), and the one selected (−1: none, the main video's).
    const QList<VideoLayer>& layers() const { return layers_; }
    int selectedLayer() const { return selectedLayer_ < layers_.size() ? selectedLayer_ : -1; }
    void setSelectedLayer(int index);
    void changeLayers(const QList<VideoLayer>& after, const QString& mergeKey = {});
    void applyLayers(const QList<VideoLayer>& l);
    // What the Clip effects section edits (D-029): the selected layer's effects, else the main video's.
    const ClipEffects& panelEffects() const;
    void changePanelEffects(const ClipEffects& after, const QString& mergeKey = {});
    // Half an output frame of the layers' own time: one key a frame (as the main video's, D-011).
    double layerKeyTolerance() const { return keyTolerance(); }
    // The output frame grid In, Out and the playhead snap to (spec §6.4, §8.3): the Output section's FPS.
    Rational outputFps() const;
    // The export's frame as the Output section makes it (spec §10.2): the scene rectangle, its size, its alpha.
    QRectF exportFrame() const;
    QSize outputSize(bool* limited = nullptr) const;
    bool outputAlpha() const;
    const ViewState& view() const { return view_; }
    const QJsonObject& extra() const { return extra_; }
    QUndoStack* undoStack() { return &undo_; }

    // A new source starts a new document: the transform at its initial place and Undo cleared.
    void setSource(const SourceInfo& source);
    // Everything as a value (to save, to snapshot for an export) and back (a project read: Undo cleared).
    ProjectData snapshot() const;
    void restore(const ProjectData& data);
    // The project as AvLoop Studio's sequence of tracks and clips (D-031): made from the fields above until the editor
    // edits the sequence itself (AS-003, RS-9).
    Sequence sequence() const;
    // The view and the unknown keys are kept outside Undo.
    void setView(const ViewState& view);
    void setExtra(const QJsonObject& extra);
    const QString& exportPath() const { return exportPath_; }
    void setExportPath(const QString& path) { exportPath_ = path; }

    // Pushes the change as one Undo step; changes under the same merge key within 500 ms (or one drag) merge.
    void changeTransform(const VideoTransform& after, const QString& mergeKey = {});
    void changeCrop(const CropShape& after, const QString& mergeKey = {});
    void changeRange(const TimeRange& after, const QString& mergeKey = {});
    void changeLoop(const LoopSettings& after, const QString& mergeKey = {});
    // A new frame rate keeps In and Out in seconds, on the new grid (one Undo step with the range — reading R-29).
    void changeOutput(const OutputSettings& after, const QString& mergeKey = {});
    void changeQuality(const QualitySettings& after, const QString& mergeKey = {});
    void changeEffects(const ClipEffects& after, const QString& mergeKey = {});
    // A preset (spec §11.3): its Loop, Output and Quality as one Undo step.
    void changeSettings(const LoopSettings& loop, const OutputSettings& output, const QualitySettings& quality,
                        const QString& name);
    // Keyframes (spec §6.4, §11.2: AddKeyframe, MoveKeyframe, DeleteKeyframe). With keyframes, changeTransform writes
    // into the keyframe at the playhead, making one there if none (like After Effects).
    void addKeyframe(); // at the playhead with the transform shown; one there is updated
    void deleteKeyframe(int index);
    void moveKeyframe(int index, double seconds, const QString& mergeKey = {});
    void copyKeyframe(int index, double seconds, const QString& mergeKey = {}); // Alt+drag; the original stays
    void setKeyframeEase(int index, bool ease);

    // Only commands call these.
    void applyTransform(const VideoTransform& t);
    void applyCrop(const CropShape& c);
    void applyRange(const TimeRange& r);
    void applyLoop(const LoopSettings& l);
    void applyOutput(const OutputSettings& o);
    void applyQuality(const QualitySettings& q);
    void applyKeyframes(const Keyframes& k);
    void applyEffects(const ClipEffects& e);

signals:
    void changed(avloop::Project::Aspect aspect);

private:
    SourceInfo source_;
    TimeRange range_;
    void refreshCurrent(Aspect aspect);
    void pushKeyframes(int id, const QString& text, const Keyframes& after, const QString& mergeKey = {});

    VideoTransform transform_;
    VideoTransform current_; // transform_ with the keyframes at time_
    Keyframes keyframes_;
    double time_ = 0;
    CropShape crop_;
    QList<int> selectedShapes_;
    LoopSettings loop_;
    OutputSettings output_;
    QualitySettings quality_;
    ClipEffects effects_;
    QList<VideoLayer> layers_;
    int selectedLayer_ = -1;
    ViewState view_;
    QJsonObject extra_;
    QString exportPath_;
    QUndoStack undo_;
};

} // namespace avloop
