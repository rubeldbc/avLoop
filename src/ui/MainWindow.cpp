#include "ui/MainWindow.h"

#include "core/AppInfo.h"
#include "core/AppSettings.h"
#include "core/Project.h"
#include "export/ExportJob.h"
#include "export/ExportQueue.h"
#include "export/SizeEstimator.h"
#include "media/DecodeWorker.h"
#include "media/DiskCache.h"
#include "media/FrameCache.h"
#include "media/WorkerThread.h"
#include "render/RenderService.h"
#include "ui/WalkRunner.h"
#include "ui/canvas/Canvas.h"
#include "ui/canvas/DropZone.h"
#include "ui/export/ExportBar.h"
#include "ui/panels/ProjectPanel.h"
#include "ui/panels/SidePanel.h"
#include "ui/preview/PreviewWindow.h"
#include "ui/theme/IconFactory.h"
#include "ui/theme/Theme.h"
#include "ui/timeline/Timeline.h"
#include "ui/timeline/Transport.h"
#include "ui/tools/ToolManager.h"

#include <QAction>
#include <QButtonGroup>
#include <QCloseEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScreen>
#include <QSettings>
#include <QSplitter>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

namespace avloop {

namespace {

QFrame* separator(const QString& name, Qt::Orientation orientation, QWidget* parent) {
    auto* line = new QFrame(parent);
    line->setObjectName(name);
    line->setAttribute(Qt::WA_StyledBackground);
    if (orientation == Qt::Vertical)
        line->setFixedSize(1, 20);
    else
        line->setFixedSize(24, 1);
    return line;
}

} // namespace

QList<MainWindow::ToolInfo> MainWindow::tools() {
    return {
        {QStringLiteral("select"), QStringLiteral("tool-select"), tr("Selection"), QKeySequence(Qt::Key_V)},
        {QStringLiteral("crop-rect"), QStringLiteral("tool-crop-rect"), tr("Crop: Rectangle"), QKeySequence(Qt::Key_M)},
        {QStringLiteral("crop-round"), QStringLiteral("tool-crop-round"), tr("Crop: Rounded"), QKeySequence(Qt::Key_U)},
        {QStringLiteral("crop-ellipse"), QStringLiteral("tool-crop-ellipse"), tr("Crop: Ellipse"),
         QKeySequence(Qt::Key_E)},
        {QStringLiteral("shape"), QStringLiteral("tool-shape"), tr("Shape"), QKeySequence(Qt::Key_S)},
        {QStringLiteral("pen"), QStringLiteral("tool-pen"), tr("Pen"), QKeySequence(Qt::Key_P)},
        {QStringLiteral("hand"), QStringLiteral("tool-hand"), tr("Hand"), QKeySequence(Qt::Key_H)},
        {QStringLiteral("zoom"), QStringLiteral("tool-zoom"), tr("Zoom"), QKeySequence(Qt::Key_Z)},
        {QStringLiteral("in"), QStringLiteral("tool-in"), tr("In"), QKeySequence(Qt::Key_I)},
        {QStringLiteral("out"), QStringLiteral("tool-out"), tr("Out"), QKeySequence(Qt::Key_O)},
        {QStringLiteral("keyframe"), QStringLiteral("tool-keyframe"), tr("Keyframe"), QKeySequence(Qt::Key_K)},
    };
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setObjectName(QStringLiteral("MainWindow"));
    setWindowTitle(AppInfo::displayName());
    setMinimumSize(1280, 720);
    setAcceptDrops(true);
    // Settings → Performance: the disk cache's folder and size, the RAM cache's (spec §3.7.1, §11.3).
    const QString diskRoot = appsettings::diskCacheFolder();
    disk_ = std::make_unique<media::DiskCache>(diskRoot.isEmpty() ? media::DiskCache::defaultRoot() : diskRoot,
                                               std::int64_t(appsettings::diskCacheGB()) << 30);
    cache_ = std::make_unique<media::FrameCache>(std::int64_t(appsettings::ramCacheMB()) << 20);
    cache_->setDiskCache(disk_.get());

    auto* central = new QWidget(this);
    auto* column = new QVBoxLayout(central);
    column->setContentsMargins(0, 0, 0, 0);
    column->setSpacing(0);
    column->addWidget(buildTopBar());

    auto* canvasRow = new QWidget(central);
    auto* canvasLayout = new QHBoxLayout(canvasRow);
    canvasLayout->setContentsMargins(0, 0, 0, 0);
    canvasLayout->setSpacing(0);
    canvasLayout->addWidget(buildRail());
    canvasStack_ = new QStackedWidget(canvasRow);
    canvasStack_->setObjectName(QStringLiteral("CanvasArea"));
    canvasStack_->setAttribute(Qt::WA_StyledBackground);
    dropZone_ = new DropZone(canvasStack_);
    canvasStack_->addWidget(dropZone_);
    project_ = new Project(this);
    canvas_ = new Canvas(project_, canvasStack_);
    canvasStack_->addWidget(canvas_);
    connect(dropZone_, &DropZone::openRequested, this, &MainWindow::chooseFile);
    connect(dropZone_, &DropZone::filesDropped, this, &MainWindow::openFiles);
    canvasLayout->addWidget(canvasStack_, 1);

    centreSplit_ = new QSplitter(Qt::Vertical, central);
    centreSplit_->setObjectName(QStringLiteral("split.centre"));
    centreSplit_->setChildrenCollapsible(false);
    centreSplit_->setHandleWidth(1);
    centreSplit_->addWidget(canvasRow);
    centreSplit_->addWidget(buildTimelineArea());
    centreSplit_->setStretchFactor(0, 1);

    side_ = new SidePanel(central);
    side_->setMinimumWidth(220);
    mainSplit_ = new QSplitter(Qt::Horizontal, central);
    mainSplit_->setObjectName(QStringLiteral("split.main"));
    mainSplit_->setChildrenCollapsible(false);
    mainSplit_->setHandleWidth(1);
    projectPanel_ = new ProjectPanel(project_, central);
    projectPanel_->setMinimumWidth(170);
    leftSplit_ = new QSplitter(Qt::Horizontal, central);
    leftSplit_->setObjectName(QStringLiteral("split.left"));
    leftSplit_->setChildrenCollapsible(false);
    leftSplit_->setHandleWidth(1);
    leftSplit_->addWidget(projectPanel_);
    leftSplit_->addWidget(centreSplit_);
    leftSplit_->setStretchFactor(1, 1);
    mainSplit_->addWidget(leftSplit_);
    mainSplit_->addWidget(side_);
    mainSplit_->setStretchFactor(0, 1);
    column->addWidget(mainSplit_, 1);
    setCentralWidget(central);

    buildStatusBar();
    buildMenu();
    buildCanvasTools();
    buildTimeline();
    buildPreview();
    buildExport();
    buildProject();
    buildFinder();
    buildLayers();
    buildMonitor();
    buildBatch();
    registerProbes();
    registerMediaProbes();
    restoreLayout();
    selectTool(QStringLiteral("select"));

    auto* status = new QTimer(this);
    status->setInterval(1000);
    connect(status, &QTimer::timeout, this, &MainWindow::updateStatus);
    status->start();
    updateStatus();
}

MainWindow::~MainWindow() {
    // A Preview still open goes first: it works on the render service, which the members' end takes away before
    // QWidget's would delete it (quitting with the Preview open crashed on the way out — AV-019).
    delete previewWindow_.data();
    removeAutosave(); // a clean exit leaves no autosave; a crash does (spec §11.1)
    // The size estimate hands frames between the render thread and the export thread. With the batch, the export and
    // the estimate gone, what each thread still holds runs out — render, export, render — before the export thread
    // and its context go: a task left on the render thread would post to a deleted context.
    queue_.reset();
    exportJob_.reset();
    estimator_.reset();
    if (renderService_) {
        renderService_->run([] {});
        if (exportContext_) QMetaObject::invokeMethod(exportContext_.get(), [] {}, Qt::BlockingQueuedConnection);
        renderService_->run([] {});
    }
}

QWidget* MainWindow::buildTopBar() {
    auto* bar = new QWidget(this);
    bar->setObjectName(QStringLiteral("TopBar"));
    bar->setAttribute(Qt::WA_StyledBackground);
    bar->setFixedHeight(36);
    auto* row = new QHBoxLayout(bar);
    row->setContentsMargins(8, 0, 8, 0);
    row->setSpacing(8);

    menuButton_ = new QToolButton(bar);
    menuButton_->setObjectName(QStringLiteral("menu.button"));
    menuButton_->setIcon(IconFactory::icon(QStringLiteral("menu")));
    menuButton_->setIconSize(QSize(18, 18));
    menuButton_->setToolTip(tr("Menu"));
    menuButton_->setAutoRaise(true);
    menuButton_->setPopupMode(QToolButton::InstantPopup);
    row->addWidget(menuButton_);

    auto* logo = new QLabel(bar);
    logo->setPixmap(IconFactory::appIcon().pixmap(QSize(20, 20)));
    auto* title = new QLabel(AppInfo::displayName(), bar);
    title->setObjectName(QStringLiteral("AppTitle"));
    row->addWidget(logo);
    row->addWidget(title);
    row->addSpacing(6);

    open_ = new QPushButton(IconFactory::icon(QStringLiteral("open")), tr("Open"), bar);
    open_->setObjectName(QStringLiteral("topbar.open"));
    open_->setToolTip(tr("Open a video or project (Ctrl+O)"));
    connect(open_, &QPushButton::clicked, this, &MainWindow::chooseFile);
    auto* openKey = new QAction(tr("Open a video or project"), open_);
    openKey->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_O));
    connect(openKey, &QAction::triggered, this, &MainWindow::chooseFile);
    addAction(openKey);
    row->addWidget(open_);
    row->addWidget(separator(QStringLiteral("TopBarSeparator"), Qt::Vertical, bar));

    options_ = new QStackedWidget(bar);
    options_->setObjectName(QStringLiteral("OptionsBar"));
    options_->addWidget(new QWidget(options_));
    row->addWidget(options_, 1);
    row->addWidget(separator(QStringLiteral("TopBarSeparator"), Qt::Vertical, bar));

    preview_ = new QPushButton(IconFactory::icon(QStringLiteral("preview")), tr("Preview"), bar);
    preview_->setObjectName(QStringLiteral("topbar.preview"));
    preview_->setToolTip(tr("Preview (F5)"));
    preview_->setEnabled(false);
    export_ = new QPushButton(IconFactory::icon(QStringLiteral("export"), IconTone::OnAccent), tr("Export"), bar);
    export_->setObjectName(QStringLiteral("topbar.export"));
    export_->setProperty("primary", true);
    export_->setToolTip(tr("Export (Ctrl+E)"));
    export_->setEnabled(false);
    row->addWidget(preview_);
    row->addWidget(export_);
    return bar;
}

QWidget* MainWindow::buildRail() {
    auto* rail = new QWidget(this);
    rail->setObjectName(QStringLiteral("ToolRail"));
    rail->setAttribute(Qt::WA_StyledBackground);
    rail->setFixedWidth(40);
    auto* column = new QVBoxLayout(rail);
    column->setContentsMargins(4, 6, 4, 6);
    column->setSpacing(2);
    column->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    toolGroup_ = new QButtonGroup(rail);
    toolGroup_->setExclusive(true);
    const QStringList groupEnds = {QStringLiteral("select"), QStringLiteral("pen"), QStringLiteral("zoom")};
    for (const ToolInfo& tool : tools()) {
        auto* b = new QToolButton(rail);
        b->setObjectName(QStringLiteral("tool.%1").arg(tool.id));
        b->setIcon(IconFactory::icon(tool.icon));
        b->setIconSize(QSize(20, 20));
        b->setFixedSize(32, 32);
        b->setCheckable(true);
        b->setAutoRaise(true);
        b->setToolTip(QStringLiteral("%1 (%2)").arg(tool.name, tool.key.toString(QKeySequence::NativeText)));
        b->setProperty("toolId", tool.id);
        toolGroup_->addButton(b);
        connect(b, &QToolButton::clicked, this, [this, id = tool.id] { selectTool(id); });
        // I and O set In and Out at the playhead, not the tool (spec §6.4; MainWindowTimeline.cpp).
        if (tool.id == QLatin1String("in") || tool.id == QLatin1String("out")) {
            column->addWidget(b, 0, Qt::AlignHCenter);
            continue;
        }
        // The key switches at once (a button's own shortcut would animate a click for 100 ms first).
        auto* key = new QAction(b);
        key->setShortcut(tool.key);
        connect(key, &QAction::triggered, this, [this, id = tool.id] { selectTool(id); });
        // K also adds a keyframe at the playhead, or updates the one there (spec §6.4).
        if (tool.id == QLatin1String("keyframe"))
            connect(key, &QAction::triggered, this, [this] {
                if (video_) project_->addKeyframe();
            });
        addAction(key);
        column->addWidget(b, 0, Qt::AlignHCenter);
        if (groupEnds.contains(tool.id))
            column->addWidget(separator(QStringLiteral("RailSeparator"), Qt::Horizontal, rail), 0, Qt::AlignHCenter);
    }
    return rail;
}

QWidget* MainWindow::buildTimelineArea() {
    auto* area = new QWidget(this);
    area->setObjectName(QStringLiteral("TimelineArea"));
    area->setAttribute(Qt::WA_StyledBackground);
    area->setMinimumHeight(110);
    area->setMaximumHeight(260);
    auto* column = new QVBoxLayout(area);
    column->setContentsMargins(0, 0, 0, 0);
    column->setSpacing(0);
    transport_ = new Transport(area);
    column->addWidget(transport_);
    timeline_ = new Timeline(project_, area);
    column->addWidget(timeline_, 1);
    return area;
}

void MainWindow::buildStatusBar() {
    auto* bar = statusBar();
    bar->setSizeGripEnabled(false);
    bar->setFixedHeight(22);
    sourceLabel_ = new QLabel(tr("No video open"), bar);
    sourceLabel_->setObjectName(QStringLiteral("status.source"));
    gpuLabel_ = new QLabel(bar);
    gpuLabel_->setObjectName(QStringLiteral("status.gpu"));
    exportBar_ = new ExportBar(bar);
    memoryLabel_ = new QLabel(bar);
    memoryLabel_->setObjectName(QStringLiteral("status.memory"));
    bar->addWidget(sourceLabel_);
    bar->addWidget(gpuLabel_);
    bar->addPermanentWidget(exportBar_);
    bar->addPermanentWidget(memoryLabel_);
}

void MainWindow::registerProbes() {
    // Maximized — or, in a lab run that never takes the focus, covering the work area as maximized would.
    WalkProbes::add(QStringLiteral("window.maximized"), [this] {
        const QRect area = screen() ? screen()->availableGeometry() : QRect();
        return isMaximized() || (!area.isEmpty() && frameGeometry().adjusted(-12, -12, 12, 12).contains(area));
    });
    WalkProbes::add(QStringLiteral("window.frame"), [this] {
        const QRect f = frameGeometry();
        const QRect a = screen() ? screen()->availableGeometry() : QRect();
        return QStringLiteral("frame %1,%2 %3x%4 · work area %5,%6 %7x%8")
            .arg(f.x())
            .arg(f.y())
            .arg(f.width())
            .arg(f.height())
            .arg(a.x())
            .arg(a.y())
            .arg(a.width())
            .arg(a.height());
    });
    WalkProbes::add(QStringLiteral("window.size"),
                    [this] { return QStringLiteral("%1x%2").arg(width()).arg(height()); });
    WalkProbes::add(QStringLiteral("topbar.buttons"), [this] { return topBarButtons().join(QLatin1Char(',')); });
    WalkProbes::add(QStringLiteral("tool.active"), [this] { return activeTool(); });
    WalkProbes::add(QStringLiteral("rail.count"), [this] { return toolGroup_->buttons().size(); });
    WalkProbes::add(QStringLiteral("theme"), [] { return Theme::idName(Theme::instance().id()); });
    WalkProbes::add(QStringLiteral("icons.missing"), [] {
        QStringList missing;
        for (const QString& name : IconFactory::allNames())
            if (!IconFactory::exists(name)) missing << name;
        return missing.isEmpty() ? QStringLiteral("none") : missing.join(QLatin1Char(','));
    });
    WalkProbes::add(QStringLiteral("icons.count"), [] { return IconFactory::allNames().size(); });
    WalkProbes::add(QStringLiteral("status.source"), [this] { return sourceLabel_->text(); });
    WalkProbes::add(QStringLiteral("dropzone.visible"), [this] { return dropZone_->isVisible(); });
}

void MainWindow::selectTool(const QString& id) {
    for (QAbstractButton* b : toolGroup_->buttons())
        if (b->property("toolId").toString() == id) b->setChecked(true);
    if (tools_) tools_->activate(id);
}

QString MainWindow::activeTool() const {
    QAbstractButton* b = toolGroup_->checkedButton();
    return b ? b->property("toolId").toString() : QString();
}

QStringList MainWindow::topBarButtons() const {
    QStringList names;
    for (QPushButton* b : {open_, preview_, export_})
        if (b->isVisible()) names << b->text();
    return names;
}

void MainWindow::showEvent(QShowEvent* event) {
    QMainWindow::showEvent(event);
    if (!shownOnce_) {
        shownOnce_ = true;
        QTimer::singleShot(0, this, &MainWindow::firstShown);
    }
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (!mayCloseDuringExport() || !maySaveChanges()) {
        event->ignore();
        return;
    }
    removeAutosave();
    saveLayout();
    QMainWindow::closeEvent(event);
}

void MainWindow::restoreLayout() {
    QSettings s;
    restoreGeometry(s.value(QStringLiteral("ui/windowGeometry")).toByteArray());
    if (!mainSplit_->restoreState(s.value(QStringLiteral("ui/splitMain")).toByteArray()))
        mainSplit_->setSizes({1000, 260});
    if (!centreSplit_->restoreState(s.value(QStringLiteral("ui/splitCentre")).toByteArray()))
        centreSplit_->setSizes({600, 150});
    if (!leftSplit_->restoreState(s.value(QStringLiteral("ui/splitLeft")).toByteArray()))
        leftSplit_->setSizes({200, 1000});
}

void MainWindow::saveLayout() const {
    QSettings s;
    s.setValue(QStringLiteral("ui/windowGeometry"), saveGeometry());
    s.setValue(QStringLiteral("ui/splitMain"), mainSplit_->saveState());
    s.setValue(QStringLiteral("ui/splitCentre"), centreSplit_->saveState());
    s.setValue(QStringLiteral("ui/splitLeft"), leftSplit_->saveState());
}

} // namespace avloop
