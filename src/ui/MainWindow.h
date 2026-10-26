#pragma once

#include "core/ProjectJson.h"
#include "media/MediaInfo.h"

#include <QElapsedTimer>
#include <QMainWindow>
#include <QPointer>
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
class QTimer;
class QToolButton;

namespace avloop {

namespace media {
class DecodeWorker;
class WorkerThread;
class DiskCache;
class FrameCache;
} // namespace media

namespace exporting {
class ExportJob;
class SizeEstimator;
} // namespace exporting

class Canvas;
class DropZone;
class ExportBar;
class PreviewWindow;
namespace render {
class RenderService;
}
class PlaybackClock;
class Timeline;
class Project;
class ToolManager;
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
    // A project file (spec §11.1): its source opened, then the rest restored (the file's UX is AV-010's).
    void openProject(const QString& path);
    Project* project() const { return project_; }
    Canvas* canvas() const { return canvas_; }
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
    // MainWindowCanvas.cpp
    void buildCanvasTools();
    // MainWindowTimeline.cpp — the playhead, what the canvas shows, playback.
    void buildTimeline();
    void seek(double t, bool fast = false);
    void stepFrames(int n);
    void togglePlay();
    void startPlayback(double from);
    void toggleLoop();
    void refreshTransport();
    double fileEnd() const;
    // MainWindowPreview.cpp — the one Preview window.
    void buildPreview();
    void openPreview();
    render::RenderService* renderService();
    // MainWindowExport.cpp — the export (spec §10.1), its Save dialog, its bar and toasts, the size estimate.
    void buildExport();
    void chooseExport();
    void exportTo(const QString& path);
    void exportFinished();
    void refreshExportState();
    void requestEstimate();
    QString exportBlocker() const; // why Export (and Preview) cannot run now; empty when they can
    bool exportRunning() const;
    bool mayCloseDuringExport();
    // A yes/no question; a lab run answers it from the walk ("answer" step) and never opens a dialog.
    bool confirm(const QString& text);
    QObject* exportContext(); // made at first use, as the estimator
    exporting::SizeEstimator* estimator();

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
    Timeline* timeline_ = nullptr;
    PlaybackClock* clock_ = nullptr;
    QLabel* droppedLabel_ = nullptr;
    bool loopOn_ = true;
    double shownSeconds_ = 0;
    QElapsedTimer rasterClock_; // playback's timeline repaints, at most 30 a second
    QPointer<PreviewWindow> previewWindow_;
    QTimer* previewDue_ = nullptr;
    QString rangeText_;
    QSplitter* mainSplit_ = nullptr;
    QSplitter* centreSplit_ = nullptr;
    QLabel* sourceLabel_ = nullptr;
    QLabel* gpuLabel_ = nullptr;
    QLabel* memoryLabel_ = nullptr;
    ExportBar* exportBar_ = nullptr;
    QString lastQuestion_;
    bool labAnswer_ = true;
    bool obsTipShown_ = false;
    Project* project_ = nullptr;
    Canvas* canvas_ = nullptr;
    ToolManager* tools_ = nullptr;
    std::optional<ProjectData> pendingProject_; // read, waiting for its source to open
    bool shownOnce_ = false;

    std::unique_ptr<media::DiskCache> disk_;
    std::unique_ptr<media::FrameCache> cache_;
    std::unique_ptr<render::RenderService> renderService_; // outlives every Preview
    std::unique_ptr<media::WorkerThread> decodeThread_;    // outlives every worker: declared first, gone last
    std::unique_ptr<media::DecodeWorker> worker_;
    std::unique_ptr<QObject> exportContext_;            // on the export thread; deleted once that thread is gone
    std::unique_ptr<media::WorkerThread> exportThread_; // the export's encoder and the estimate's (spec §10.1)
    std::unique_ptr<exporting::SizeEstimator> estimator_;
    std::unique_ptr<exporting::ExportJob> exportJob_; // the running or the last export: goes first
    std::optional<media::MediaInfo> video_;
    QString videoState_ = QStringLiteral("none"); // none, opening, ready, error
    QString lastError_;
    QSize shownFrame_;
    bool pressureToastShown_ = false;
};

} // namespace avloop
