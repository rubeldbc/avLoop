#pragma once

#include "media/MediaInfo.h"

#include <QMainWindow>
#include <QStringList>

#include <memory>
#include <optional>

class QAction;
class QActionGroup;
class QButtonGroup;
class QLabel;
class QMenu;
class QPushButton;
class QSplitter;
class QStackedWidget;
class QToolButton;

namespace avloop {

namespace media {
class DecodeWorker;
class DiskCache;
class FrameCache;
} // namespace media

class DropZone;
class SidePanel;
class Transport;

// The main window (spec §5.1): the top bar (☰, logo, Open | options bar | Preview, Export), the tool rail, the canvas
// area, the side panel, the timeline area and the status bar. Opens maximized; the normal geometry and the splitters
// are remembered.
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    // The rail's tools in order: id, icon, the tooltip's name and the key.
    struct ToolInfo {
        QString id, icon, name;
        QKeySequence key;
    };
    static QList<ToolInfo> tools();

    void selectTool(const QString& id);
    QString activeTool() const;
    QStringList topBarButtons() const;

    // MainWindowActions.cpp — the video (spec §3.9): opened by its content, released with everything of it when
    // another is opened.
    void openFiles(const QStringList& paths);
    void openVideo(const QString& path);
    void closeVideo();
    media::FrameCache* frameCache() const { return cache_.get(); }
    QString videoState() const { return videoState_; }

signals:
    void firstShown();

protected:
    void showEvent(QShowEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    QWidget* buildTopBar();
    QWidget* buildRail();
    QWidget* buildTimelineArea();
    void buildStatusBar();
    void buildMenu(); // MainMenu.cpp
    void registerProbes();
    void restoreLayout();
    void saveLayout() const;
    void setThemeFromMenu(const QString& name);
    // MainWindowActions.cpp
    void chooseFile();
    void probed(const QString& path, const std::optional<media::MediaInfo>& info, const QString& error);
    void showFrame(const media::Frame& frame);
    void updateStatus();
    void registerMediaProbes();

    QToolButton* menuButton_ = nullptr;
    QMenu* menu_ = nullptr;
    QActionGroup* themeGroup_ = nullptr;
    QPushButton* open_ = nullptr;
    QPushButton* preview_ = nullptr;
    QPushButton* export_ = nullptr;
    QStackedWidget* options_ = nullptr;
    QButtonGroup* toolGroup_ = nullptr;
    QStackedWidget* canvasStack_ = nullptr;
    DropZone* dropZone_ = nullptr;
    SidePanel* side_ = nullptr;
    Transport* transport_ = nullptr;
    QSplitter* mainSplit_ = nullptr;
    QSplitter* centreSplit_ = nullptr;
    QLabel* sourceLabel_ = nullptr;
    QLabel* gpuLabel_ = nullptr;
    QLabel* memoryLabel_ = nullptr;
    QWidget* exportArea_ = nullptr;
    QLabel* framePreview_ = nullptr; // the frame until the canvas (AV-003)
    bool shownOnce_ = false;

    std::unique_ptr<media::DiskCache> disk_;
    std::unique_ptr<media::FrameCache> cache_;
    std::unique_ptr<media::DecodeWorker> worker_;
    std::optional<media::MediaInfo> video_;
    QString videoState_ = QStringLiteral("none"); // none, opening, ready, error
    QString lastError_;
    QSize shownFrame_;
    bool pressureToastShown_ = false;
};

} // namespace avloop
