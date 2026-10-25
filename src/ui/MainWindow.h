#pragma once

#include <QMainWindow>
#include <QStringList>

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

signals:
    void firstShown();

protected:
    void showEvent(QShowEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

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
    bool shownOnce_ = false;
};

} // namespace avloop
