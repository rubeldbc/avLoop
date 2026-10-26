// The main window's canvas side (spec §6, §6.3, §11.2): the tools and their options bar, Undo and Redo on the
// project's stack, the view's keys (Ctrl+0 Fit, Ctrl+1 100%, Ctrl+= / Ctrl+- zoom), and what walks read of them.
#include "core/Geometry.h"
#include "core/Project.h"
#include "ui/MainWindow.h"
#include "ui/WalkRunner.h"
#include "ui/canvas/Canvas.h"
#include "ui/panels/SidePanel.h"
#include "ui/tools/Tool.h"
#include "ui/tools/ToolManager.h"

#include <QAction>
#include <QDir>
#include <QJsonObject>
#include <QLabel>
#include <QStackedWidget>
#include <QStatusBar>
#include <QUndoStack>

namespace avloop {

void MainWindow::buildCanvasTools() {
    tools_ = new ToolManager(project_, canvas_, options_, this);
    tools_->add(createSelectionTool(canvas_, project_));
    tools_->add(createHandTool(canvas_, project_));
    tools_->add(createZoomTool(canvas_, project_));
    for (CropShape::Kind kind : {CropShape::Kind::Rect, CropShape::Kind::Rounded, CropShape::Kind::Ellipse})
        tools_->add(createCropTool(int(kind), canvas_, project_));
    tools_->add(createInOutTool(false, canvas_, project_));
    tools_->add(createInOutTool(true, canvas_, project_));
    tools_->add(createKeyframeTool(canvas_, project_, [this](double t) { seek(t); }));
    side_->bindCrop(project_);
    side_->bindLoop(project_);
    side_->bindOutput(project_);
    side_->bindEffects(project_,
                       [this](std::function<void(const QColor&)> done) { canvas_->pickColour(std::move(done)); });

    // "Transparent area in output" / "Output is fully transparent" (spec §6.2, §12.2).
    auto* notice = new QLabel(statusBar());
    notice->setObjectName(QStringLiteral("status.notice"));
    statusBar()->addWidget(notice);
    connect(project_, &Project::changed, notice, [this, notice](Project::Aspect) {
        const auto cover = geometry::coverage(project_->crop(), project_->transform(), project_->source().size());
        notice->setText(cover == geometry::Coverage::None     ? tr("Output is fully transparent")
                        : cover == geometry::Coverage::Partly ? tr("Transparent area in output")
                                                              : QString());
    });

    QUndoStack* stack = project_->undoStack();
    if (auto* undo = findChild<QAction*>(QStringLiteral("action.undo"))) {
        undo->setEnabled(stack->canUndo());
        connect(stack, &QUndoStack::canUndoChanged, undo, &QAction::setEnabled);
        connect(undo, &QAction::triggered, stack, &QUndoStack::undo);
    }
    if (auto* redo = findChild<QAction*>(QStringLiteral("action.redo"))) {
        redo->setShortcuts({QKeySequence(Qt::CTRL | Qt::Key_Y), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Z)});
        redo->setEnabled(stack->canRedo());
        connect(stack, &QUndoStack::canRedoChanged, redo, &QAction::setEnabled);
        connect(redo, &QAction::triggered, stack, &QUndoStack::redo);
    }

    auto key = [this](const QList<QKeySequence>& keys, auto act) {
        auto* a = new QAction(this);
        a->setShortcuts(keys);
        connect(a, &QAction::triggered, this, act);
        addAction(a);
    };
    key({QKeySequence(Qt::CTRL | Qt::Key_0)}, [this] { canvas_->fitView(); });
    key({QKeySequence(Qt::CTRL | Qt::Key_1)}, [this] { canvas_->setZoom(1.0); });
    key({QKeySequence(Qt::CTRL | Qt::Key_Equal), QKeySequence(Qt::CTRL | Qt::Key_Plus)},
        [this] { canvas_->zoomStep(1); });
    key({QKeySequence(Qt::CTRL | Qt::Key_Minus)}, [this] { canvas_->zoomStep(-1); });

    const auto& t = [this]() -> const VideoTransform& { return project_->transform(); };
    WalkProbes::add(QStringLiteral("transform.x"), [t] { return t().x; });
    WalkProbes::add(QStringLiteral("transform.y"), [t] { return t().y; });
    WalkProbes::add(QStringLiteral("transform.scale"), [t] { return t().scale; });
    WalkProbes::add(QStringLiteral("transform.flipH"), [t] { return t().flipH; });
    WalkProbes::add(QStringLiteral("transform.flipV"), [t] { return t().flipV; });
    const auto& c = [this]() -> const CropShape& { return project_->crop(); };
    WalkProbes::add(QStringLiteral("crop.empty"), [c] { return c().isEmpty(); });
    WalkProbes::add(QStringLiteral("crop.kind"), [c] { return CropShape::kindId(c().kind); });
    WalkProbes::add(QStringLiteral("crop.ratio"), [c] { return c().ratio; });
    WalkProbes::add(QStringLiteral("crop.x"), [c] { return c().rect.x(); });
    WalkProbes::add(QStringLiteral("crop.y"), [c] { return c().rect.y(); });
    WalkProbes::add(QStringLiteral("crop.w"), [c] { return c().rect.width(); });
    WalkProbes::add(QStringLiteral("crop.h"), [c] { return c().rect.height(); });
    WalkProbes::add(QStringLiteral("crop.cx"), [c] { return c().rect.center().x(); });
    WalkProbes::add(QStringLiteral("crop.cy"), [c] { return c().rect.center().y(); });
    WalkProbes::add(QStringLiteral("crop.radius"), [c] { return c().radius; });
    WalkProbes::add(QStringLiteral("crop.feather"), [c] { return c().feather; });
    WalkProbes::add(QStringLiteral("crop.output"), [this] {
        const QSize out = geometry::outputSize(
            geometry::exportFrame(project_->crop().rect, project_->transform(), project_->source().size()).size());
        return QStringLiteral("%1x%2").arg(out.width()).arg(out.height());
    });
    WalkProbes::add(QStringLiteral("status.notice"), [this] {
        auto* l = statusBar()->findChild<QLabel*>(QStringLiteral("status.notice"));
        return l ? l->text() : QString();
    });
    WalkProbes::add(QStringLiteral("canvas.zoom"), [this] { return canvas_->zoom(); });
    WalkProbes::add(QStringLiteral("canvas.panX"), [this] { return canvas_->pan().x(); });
    WalkProbes::add(QStringLiteral("canvas.panY"), [this] { return canvas_->pan().y(); });
    WalkProbes::add(QStringLiteral("canvas.gl"), [this] { return canvas_->glReady(); });
    WalkProbes::add(QStringLiteral("undo.count"), [stack] { return stack->count(); });
    WalkProbes::add(QStringLiteral("undo.index"), [stack] { return stack->index(); });
    // Walks give canvas points in scene px: {"space": "canvas", "at": [x, y]}.
    WalkProbes::addMapper(QStringLiteral("canvas"),
                          [this](const QPointF& scene) { return canvas_->sceneToView(scene); });
    // {"do": "project", "file": "<path>"}: opens a project as the command line does.
    WalkRunner::addStep(QStringLiteral("project"), [this](WalkRunner& walk, const QJsonObject& step) {
        const QString file = step.value(QStringLiteral("file")).toString();
        walk.log(QStringLiteral("     project %1").arg(file));
        openProject(QDir::isAbsolutePath(file) ? file : QDir::current().absoluteFilePath(file));
        return 600;
    });
}

} // namespace avloop
