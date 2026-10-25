#pragma once

#include <QString>
#include <QWidget>

class QHBoxLayout;
class QLabel;
class QPushButton;
class QToolButton;

namespace avloop {

class Canvas;
class Project;
class StepSpinBox;

// A tool's row in the top bar's options slot (spec §5.1, §6's table): the tool's name, then its numbers, toggles and
// buttons, each named for walks ("options.<name>").
class OptionsRow : public QWidget {
    Q_OBJECT
public:
    OptionsRow(const QString& tool, const QString& title, QWidget* parent = nullptr);

    StepSpinBox* addNumber(const QString& name, const QString& label, double min, double max, double step, int decimals,
                           const QString& suffix = {});
    QToolButton* addIcon(const QString& name, const QString& icon, const QString& tip, bool checkable = false);
    QPushButton* addButton(const QString& name, const QString& text);
    QLabel* addLabel(const QString& text);
    void addSeparator();

private:
    QString tool_;
    QHBoxLayout* row_ = nullptr;
};

// SelectionOptions.cpp, ZoomOptions.cpp
QWidget* createSelectionOptions(Project* project, Canvas* canvas, QWidget* parent);
QWidget* createZoomOptions(Canvas* canvas, QWidget* parent);

} // namespace avloop
