#pragma once

#include <QWidget>

#include <functional>

namespace avloop {

class Project;
class Timeline;

// The video tracks above the main video on the timeline (D-027, AV-031), as Premiere Pro's: one lane a track from the
// highest used down to V2, on the timeline's own time scale; each layer a bar with its name. A click selects it, a drag
// moves it in time (snapping to In, Out, the playhead and the other bars' ends within 8 px; Ctrl off) and up or down to
// another track (refused where it would overlap), its ends trim it (the start moving with an In trim), Alt+drag copies
// it; Delete removes the selected; a double-click takes the playhead to its start. Each one Undo step.
class TrackLanes : public QWidget {
    Q_OBJECT
public:
    TrackLanes(Project* project, Timeline* timeline, std::function<void(double)> seek, QWidget* parent = nullptr);

    static constexpr int kLane = 24; // px a track
    int topTrack() const;            // the highest track a layer is on (0 with none)
    int headTrack() const;           // the track of the first lane: one empty above the highest, while there is room
    double rowY(int track) const;    // a lane's middle
    int trackAt(double y) const;

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    enum class Drag { None, Move, TrimIn, TrimOut };
    void relayout();
    int barAt(const QPointF& at, Drag* edge) const;
    double snapped(double t, int except, bool off) const;

    Project* project_;
    Timeline* timeline_;
    std::function<void(double)> seek_;
    Drag drag_ = Drag::None;
    int index_ = -1;
    bool copy_ = false;
    bool copied_ = false;
    double pressT_ = 0;
    QString mergeKey_;
    int drags_ = 0;
    int frozenHead_ = 0; // the lanes do not move under a drag (a bar dragged into the empty lane would climb on)
};

} // namespace avloop
