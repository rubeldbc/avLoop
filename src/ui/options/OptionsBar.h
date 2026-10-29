#pragma once

#include <QString>
#include <QWidget>

class QComboBox;
class QHBoxLayout;
class QLabel;
class QPushButton;
class QToolButton;

namespace avloop {

class Canvas;
class Project;
class StepSpinBox;
struct ShapeDraft;

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
    // Any other widget (a ratio list), with a quiet label before it.
    void add(QWidget* widget, const QString& label = {});
    void addSeparator();

private:
    QString tool_;
    QHBoxLayout* row_ = nullptr;
};

// SelectionOptions.cpp, ZoomOptions.cpp
QWidget* createSelectionOptions(Project* project, Canvas* canvas, QWidget* parent);
QWidget* createZoomOptions(Canvas* canvas, QWidget* parent);
// CropOptions.cpp: a crop tool's row (`kind`: CropShape::Kind), and the ratio list shared with the Crop panel.
QWidget* createCropOptions(Project* project, Canvas* canvas, int kind, QWidget* parent);
QComboBox* createRatioBox(QWidget* parent);
// ShapeOptions.cpp: the Shape tool's row — what it draws next (`draft`), the selected shapes' role, place and size.
QWidget* createShapeOptions(Project* project, ShapeDraft* draft, QWidget* parent);
// The Pen's row: the role of the next path and of the selected shapes, and how the Pen is used.
QWidget* createPenOptions(Project* project, ShapeDraft* draft, QWidget* parent);
// A shape role list (Select, Cut …) shared by the bar and the Crop section; a role set on the selected shapes (one Undo
// step, nothing when none changes).
QComboBox* createRoleBox(QWidget* parent);
void setShapesRole(Project* project, int role);
// Combine (D-023): the selected shapes baked into one by `op` (shapecombine::Op), one Undo step; false with fewer
// than two. The menu of the four, for a button.
bool combineSelected(Project* project, int op);
// Feather and Strength (D-024) set on every selected shape (a value < 0 keeps each one's own), one Undo step.
void setShapesSoftness(Project* project, double feather, double strength, const QString& mergeKey = {});
class QMenu* createCombineMenu(Project* project, QWidget* parent);
// Applies the ratio chosen in a ratio box to the project's crop (one Undo step); Custom… takes the crop's own.
void applyRatio(Project* project, const QString& id);
// What walks read of a bar's field: its value while its row is shown, nothing once a tool switch has removed it.
void probeValue(const QString& name, StepSpinBox* box);

} // namespace avloop
