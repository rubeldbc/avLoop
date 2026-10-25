#include "ui/widgets/StepSpinBox.h"

#include <QApplication>
#include <QLineEdit>
#include <QSignalBlocker>
#include <QWheelEvent>

namespace avloop {

StepSpinBox::StepSpinBox(QWidget* parent) : QDoubleSpinBox(parent) {
    setProperty("mono", true);
    setKeyboardTracking(false);
    setButtonSymbols(QAbstractSpinBox::NoButtons);
    setAccelerated(false);
    setFocusPolicy(Qt::StrongFocus);
    setMinimumWidth(58);
}

StepSpinBox::StepSpinBox(const QString& name, double min, double max, double step, int decimals, QWidget* parent)
    : StepSpinBox(parent) {
    setObjectName(name);
    setDecimals(decimals);
    setRange(min, max);
    setSingleStep(step);
}

void StepSpinBox::setValueQuietly(double value) {
    if (hasFocus() && lineEdit()->isModified()) return; // never overwrite what is being typed
    const QSignalBlocker block(this);
    setValue(value);
}

void StepSpinBox::stepBy(int steps) {
    if (QApplication::keyboardModifiers() & Qt::ShiftModifier) steps *= 10;
    QDoubleSpinBox::stepBy(steps);
}

void StepSpinBox::wheelEvent(QWheelEvent* event) {
    // With Shift, Windows turns the wheel sideways: either axis steps.
    const QPoint d = event->angleDelta();
    const int notch = d.y() != 0 ? d.y() : d.x();
    if (notch == 0 || !isEnabled() || isReadOnly()) return;
    stepBy(notch > 0 ? 1 : -1);
    event->accept();
}

} // namespace avloop
