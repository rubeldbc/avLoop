#pragma once

#include <QHash>
#include <QObject>
#include <QString>

class QStackedWidget;

namespace avloop {

class Canvas;
class Project;
class Tool;

// Owns the tools and the one active tool (spec §6): the canvas gets its events, the options bar shows its widget.
// A rail id whose tool a later phase builds selects no tool on the canvas.
class ToolManager : public QObject {
    Q_OBJECT
public:
    ToolManager(Project* project, Canvas* canvas, QStackedWidget* optionsBar, QObject* parent = nullptr);

    void add(Tool* tool); // takes ownership (Qt parent)
    Tool* tool(const QString& id) const;
    Tool* active() const { return active_; }
    QString activeId() const { return activeId_; }
    void activate(const QString& id);

signals:
    void activeChanged(const QString& id);

private:
    Project* project_;
    Canvas* canvas_;
    QStackedWidget* options_;
    QHash<QString, Tool*> tools_;
    QHash<QString, QWidget*> optionWidgets_;
    QWidget* emptyOptions_ = nullptr;
    Tool* active_ = nullptr;
    QString activeId_;
};

} // namespace avloop
