#include "ui/tools/ToolManager.h"

#include "core/Project.h"
#include "ui/canvas/Canvas.h"
#include "ui/tools/Tool.h"

#include <QStackedWidget>

namespace avloop {

ToolManager::ToolManager(Project* project, Canvas* canvas, QStackedWidget* optionsBar, QObject* parent)
    : QObject(parent), project_(project), canvas_(canvas), options_(optionsBar) {
    emptyOptions_ = new QWidget(options_);
    options_->addWidget(emptyOptions_);
}

void ToolManager::add(Tool* tool) {
    tool->setParent(this);
    tools_.insert(tool->id(), tool);
    if (QWidget* w = tool->createOptions(options_)) {
        options_->addWidget(w);
        optionWidgets_.insert(tool->id(), w);
    }
}

Tool* ToolManager::tool(const QString& id) const {
    return tools_.value(id);
}

void ToolManager::activate(const QString& id) {
    activeId_ = id;
    active_ = tools_.value(id);
    canvas_->setTool(active_);
    options_->setCurrentWidget(optionWidgets_.value(id, emptyOptions_));
    ViewState v = project_->view();
    v.tool = id;
    project_->setView(v);
    emit activeChanged(id);
}

} // namespace avloop
