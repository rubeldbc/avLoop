#pragma once

#include "export/LoopFinder.h"

#include <QWidget>

#include <atomic>
#include <memory>

class QComboBox;
class QLabel;
class QListWidget;
class QProgressBar;
class QPushButton;

namespace avloop {

class Project;
class StepSpinBox;

// The Loop Finder's window (spec §11.5; Menu → Loop Finder, Ctrl+L): the search range (In–Out ± 5 s, or the whole
// file), the shortest and the longest loop (2–10 s), Find with its progress and Cancel, then the best five — In, Out,
// length, score and both ends' thumbnails. A click sets In and Out (one Undo step); a double-click also opens the
// Preview. The search runs off the GUI thread; closing cancels it and lets its fingerprints go.
class LoopFinderWindow : public QWidget {
    Q_OBJECT
public:
    LoopFinderWindow(Project* project, const media::MediaInfo& source, QWidget* parent = nullptr);
    ~LoopFinderWindow() override;

    void find();
    void choose(int row, bool preview);      // as a click (or a double-click) on that row
    QString state() const { return state_; } // idle, running, done, failed
    const QList<exporting::LoopFinder::Candidate>& results() const { return results_; }

signals:
    void previewRequested();

private:
    void showResults();

    Project* project_;
    media::MediaInfo source_;
    QComboBox* range_ = nullptr;
    StepSpinBox* min_ = nullptr;
    StepSpinBox* max_ = nullptr;
    QPushButton* find_ = nullptr;
    QPushButton* cancel_ = nullptr;
    QProgressBar* progress_ = nullptr;
    QLabel* status_ = nullptr;
    QListWidget* list_ = nullptr;
    QString state_ = QStringLiteral("idle");
    QList<exporting::LoopFinder::Candidate> results_;
    std::shared_ptr<std::atomic<bool>> cancelFlag_;
};

} // namespace avloop
