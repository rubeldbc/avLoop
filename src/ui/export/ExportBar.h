#pragma once

#include <QWidget>

class QLabel;
class QProgressBar;
class QToolButton;

namespace avloop {

namespace exporting {
class ExportJob;
}

// The export's place in the status bar (spec §10.1 step 4): `Exporting 86/149 · 58% · 41 fps · ETA 0:02`, a thin bar
// and Cancel; hidden when no export runs.
class ExportBar : public QWidget {
    Q_OBJECT
public:
    explicit ExportBar(QWidget* parent = nullptr);

    void track(exporting::ExportJob* job); // null: hidden
    QString text() const;

    static QString progressText(int done, int total, int pass, int passes, double fps, double eta);

signals:
    void cancelRequested();

private:
    void refresh();

    exporting::ExportJob* job_ = nullptr;
    QLabel* label_ = nullptr;
    QProgressBar* bar_ = nullptr;
    QToolButton* cancel_ = nullptr;
};

} // namespace avloop
