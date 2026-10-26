#pragma once

#include <QWidget>

class QLabel;
class QPushButton;
class QTreeWidget;

namespace avloop {

namespace exporting {
class ExportQueue;
}

// The Batch Queue's window (spec §10.5; Menu → Batch Queue): the entries — name, format, size, state, progress —, in
// the order they run, dragged to reorder; Add current (as Ctrl+Shift+E), Start, Pause, Remove and Clear done. A failed
// entry's reason is its tooltip. It shows the queue; closing it leaves the queue running.
class BatchWindow : public QWidget {
    Q_OBJECT
public:
    BatchWindow(exporting::ExportQueue* queue, QWidget* parent = nullptr);

    QString states() const; // "done,running,pending" — what walks read
    void setCanAdd(bool can, const QString& reason);
    void setCanStart(bool can, const QString& reason); // false while the main window's own export runs

signals:
    void addRequested();

private:
    void rebuild();
    void showProgress(int index);
    void refreshButtons();

    exporting::ExportQueue* queue_;
    QLabel* note_ = nullptr;
    QTreeWidget* list_ = nullptr;
    QPushButton* add_ = nullptr;
    QPushButton* start_ = nullptr;
    QPushButton* pause_ = nullptr;
    QPushButton* remove_ = nullptr;
    QPushButton* clear_ = nullptr;
    bool canStart_ = true;
};

} // namespace avloop
