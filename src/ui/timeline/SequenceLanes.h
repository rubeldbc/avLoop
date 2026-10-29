#pragma once

#include "core/Sequence.h"

#include <QList>
#include <QWidget>

#include <functional>

namespace avloop {

class Project;
class Timeline;

// The sequence's tracks on the timeline (D-034), as Premiere Pro's: V1 at the bottom up to one empty lane above the
// highest used (V8 at most), on the timeline's own time scale; every clip a bar with its name and its keys. The
// Selection: a click selects a clip (Shift adds; a layer's selection is the Layers section's too), a drag moves the
// selected in time — snapping to In, Out, the playhead and the other clips' ends within 8 px (Ctrl off) — and to
// another track (refused where it would overlap or lock), its ends trim it, Alt+drag copies it. The Razor (C; V or Esc
// leaves it): a click cuts the clip there. Ctrl+K cuts the selected (else every clip on an unlocked track) at the
// playhead; Delete leaves a gap, Shift+Delete closes it. A header's right click hides, shows, locks or unlocks its
// track. A double-click takes the playhead to the clip's start (or the key under it). One Undo step a gesture; V1
// always keeps a clip (the main video, RS-17).
class SequenceLanes : public QWidget {
    Q_OBJECT
public:
    SequenceLanes(Project* project, Timeline* timeline, std::function<void(double)> seek, QWidget* parent = nullptr);

    static constexpr int kLane = 24;   // px a track
    static constexpr int kHeader = 30; // px: the track's name, its right-click menu
    int topTrack() const;              // the highest track (1 = V1) a clip is on
    int headTrack() const;             // the first lane's: one empty above the highest, while there is room
    double rowY(int track) const;      // a lane's middle
    int trackAt(double y) const;
    void setRazor(bool on);
    bool razor() const { return razor_; }
    const QList<int>& selected() const { return selected_; }
    void select(const QList<int>& ids); // the Layers section follows a layer
    void cutAtPlayhead();               // Ctrl+K

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    enum class Drag { None, Move, TrimIn, TrimOut };
    void relayout();
    // The clip under `at` (its id, 0 for none) and which part: an end trims.
    int clipAt(const QPointF& at, Drag* edge) const;
    double snapped(double t, const QList<int>& except, bool off) const;
    void moveTo(const QPointF& at, bool noSnap);
    // `after` as one step (`text`), or said and dropped when it would leave V1 without a clip.
    bool commit(Sequence after, const QString& text, const QString& mergeKey = {});
    void removeSelected(bool ripple);
    void followLayerSelection();

    Project* project_;
    Timeline* timeline_;
    std::function<void(double)> seek_;
    QList<int> selected_;
    bool razor_ = false;
    Drag drag_ = Drag::None;
    int pressed_ = 0; // the clip the drag holds
    int pressTrack_ = 0;
    bool copy_ = false;
    double pressT_ = 0;
    Sequence start_; // the sequence when the drag began: every move is made from it (one Undo step)
    QString mergeKey_;
    int drags_ = 0;
    int frozenHead_ = 0; // the lanes do not move under a drag
    double shownX0_ = 0; // the timeline's scroll and zoom, and the playhead's column, last painted
    double shownScale_ = 0;
    int shownPlayhead_ = 0;
};

} // namespace avloop
