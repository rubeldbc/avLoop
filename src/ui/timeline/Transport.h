#pragma once

#include <QWidget>

class QLabel;
class QToolButton;

namespace avloop {

class TimeEdit;

// The transport row above the timeline (spec §7.3): Go to In, Prev frame, Play/Pause, Next frame, Go to Out, Loop;
// the time now (a TimeEdit: type to jump) and the total; the In, Out and length readout. Disabled until a video is
// open.
class Transport : public QWidget {
    Q_OBJECT
public:
    explicit Transport(QWidget* parent = nullptr);

    QToolButton* goIn() const { return goIn_; }
    QToolButton* prev() const { return prev_; }
    QToolButton* play() const { return play_; }
    QToolButton* next() const { return next_; }
    QToolButton* goOut() const { return goOut_; }
    QToolButton* loop() const { return loop_; }
    TimeEdit* timeEdit() const { return now_; }

    void setTotal(const QString& total);
    void setRangeText(const QString& text);
    void setPlaying(bool playing);
    void setLoopOn(bool on);

private:
    QToolButton* button(const QString& name, const QString& icon, const QString& tip);

    QToolButton* goIn_ = nullptr;
    QToolButton* prev_ = nullptr;
    QToolButton* play_ = nullptr;
    QToolButton* next_ = nullptr;
    QToolButton* goOut_ = nullptr;
    QToolButton* loop_ = nullptr;
    TimeEdit* now_ = nullptr;
    QLabel* total_ = nullptr;
    QLabel* range_ = nullptr;
};

} // namespace avloop
