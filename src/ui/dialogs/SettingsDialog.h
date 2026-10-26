#pragma once

#include <QDialog>

class QTabWidget;

namespace avloop {

// Settings (spec §11.3; Ctrl+,): General (Theme, open the last project at start, autosave minutes), Performance
// (hardware decoding, GPU rendering, the RAM cache, the disk cache's folder and size, Clear cache) and Export (the
// default folder, the name template, the OBS tips). A change is saved at once (AppSettings); what applies later says
// so.
class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget* parent = nullptr);

    void showTab(int index);
    QTabWidget* tabs() const { return tabs_; }

signals:
    void themeChanged(const QString& id);
    void ramCacheChanged(int megabytes);
    void autosaveChanged(int minutes);
    void clearCacheRequested();

private:
    QWidget* general();
    QWidget* performance();
    QWidget* exporting();

    QTabWidget* tabs_ = nullptr;
};

} // namespace avloop
