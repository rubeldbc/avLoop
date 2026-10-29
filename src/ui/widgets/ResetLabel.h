#pragma once

#include <QLabel>
#include <QVariant>

namespace avloop {

// Resetting a control to its default (D-025): the default is kept on the control where it is made; resetting goes
// through the control's own change path (a number's valueChanged, a list's activated, a box's click, a colour's
// colourChosen), so the model takes it as one Undo step, as if typed.
namespace reset {

// A StepSpinBox's number, a QComboBox's item data, a QCheckBox's state, a ColourField's QColor.
void setDefault(QWidget* control, const QVariant& value);
// Whether the control, or a control inside it (a pair of numbers), has a default.
bool hasDefault(const QWidget* control);
// Resets the control (and those inside it) to its default; false when there is none.
bool apply(QWidget* control);

} // namespace reset

// The label beside a control (D-025): a click resets the control to its default — the hand cursor and "Click to
// reset" when it has one, nothing for a value without a default (a position) or a read-only row. Looks as before.
class ResetLabel : public QLabel {
    Q_OBJECT
public:
    ResetLabel(const QString& text, QWidget* control, QWidget* parent = nullptr);
    QWidget* control() const { return control_; }

signals:
    void reset();

protected:
    void enterEvent(QEnterEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private:
    QWidget* control_;
};

} // namespace avloop
