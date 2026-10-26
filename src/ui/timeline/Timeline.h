#pragma once

#include "core/Rational.h"
#include "core/TimeRange.h"

#include <QWidget>

#include <memory>

class QPainter;
class QTimer;

namespace avloop {

namespace media {
struct MediaInfo;
class ThumbnailWorker;
} // namespace media

class Project;

// The timeline (spec §7.1, §7.2): the ruler, the thumbnail strip, the In/Out overlay with its bar and handles, the
// crossfade band and the keyframe row (filled by AV-006 and AV-011), and the playhead over all. Times are seconds of
// source time on the output frame grid. Timeline.cpp holds the view and the thumbnails, TimelinePaint.cpp paints,
// TimelineInput.cpp handles the mouse and the wheel.
class Timeline : public QWidget {
    Q_OBJECT
public:
    static constexpr int kRuler = 18;
    static constexpr int kBar = 8; // the 3 px orange bar sits at its foot
    static constexpr int kStripMax = 54;
    static constexpr int kStripMin = 24;
    static constexpr int kBand = 8;
    static constexpr int kKeys = 16;
    static constexpr int kMargin = 8;   // px either side, so the handles and the playhead show at the ends
    static constexpr int kSnapPx = 8;   // the playhead snaps to In and Out within this
    static constexpr int kHandlePx = 6; // a handle is grabbed this far either side

    explicit Timeline(Project* project, QWidget* parent = nullptr);
    ~Timeline() override;

    // The thumbnails are made for this video (null: none, everything of the last one released).
    void setSource(const media::MediaInfo* info);
    double playhead() const { return playhead_; }
    void setPlayhead(double t); // shown, the view following it; no signal
    // "in" or "out" while that tool is active: a click on the timeline sets In or Out there (spec §6.4).
    void setClickTool(const QString& id) { clickTool_ = id; }

    // The view (spec §7.2).
    double xOf(double t) const;
    double timeAt(double x) const;
    double pxPerSecond() const { return pxPerSecond_; }
    void zoomAbout(double factor, double x);
    void fitFile();
    void fitRange();
    bool frameLabels() const { return frameLabels_; }
    void setFrameLabels(bool on);
    int labelOverlaps() const { return overlaps_; } // in the last paint; never more than 0
    int thumbnailsMade() const;
    // Where the crossfade band's left edge is (the F frames before Out start), in seconds; Out when no crossfade.
    double fadeEdge() const;

signals:
    // The user moved the playhead: `fast` while a drag is moving (the keyframe preview), exact when it rests or ends.
    void scrubbed(double t, bool fast);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    enum class Drag { None, Scrub, In, Out, Bar, Fade };
    struct Rows {
        QRect ruler, bar, strip, band, keys;
        int overlayTop() const { return strip.top(); }
        int overlayBottom() const { return keys.bottom(); }
    };
    Rows rows() const;
    double duration() const;
    Rational fps() const;
    double segmentFrameSeconds() const; // one segment frame in source time: Speed / fps
    double visibleSeconds() const;
    void viewChanged();
    void requestThumbnails();
    // TimelinePaint.cpp
    void paintRuler(QPainter& p, const Rows& r);
    void paintStrip(QPainter& p, const Rows& r);
    void paintRange(QPainter& p, const Rows& r);
    void paintBand(QPainter& p, const Rows& r);
    void paintPlayhead(QPainter& p, const Rows& r);
    // TimelineInput.cpp
    Drag hit(const QPointF& at) const;
    double scrubTime(double x) const; // the grid, In and Out within kSnapPx
    void scrubTo(double t, bool fast);

    Project* project_;
    std::unique_ptr<media::ThumbnailWorker> thumbs_;
    QTimer* thumbsDue_ = nullptr; // coalesces repaints as thumbnails arrive
    QTimer* rest_ = nullptr;      // a scrub that rests turns exact
    double playhead_ = 0;
    double pxPerSecond_ = 100;
    double scroll_ = 0; // the time at the left margin
    bool fit_ = true;   // the whole file shown, through resizes, until the user zooms
    bool frameLabels_ = false;
    QString clickTool_;
    Drag drag_ = Drag::None;
    double dragFrom_ = 0; // the time under the pointer when the drag began
    TimeRange dragRange_;
    QString dragKey_;
    int drags_ = 0;
    int overlaps_ = 0;
};

} // namespace avloop
