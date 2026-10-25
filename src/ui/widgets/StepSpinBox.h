#pragma once

#include <QDoubleSpinBox>

namespace avloop {

// Every number of the UI (spec §5.1): the wheel and ↑/↓ step it, Shift steps ten times; typed values commit on Enter
// or when the focus leaves; Consolas digits.
class StepSpinBox : public QDoubleSpinBox {
    Q_OBJECT
public:
    explicit StepSpinBox(QWidget* parent = nullptr);
    StepSpinBox(const QString& name, double min, double max, double step, int decimals, QWidget* parent = nullptr);

    // Sets the value without emitting valueChanged (a change that came from the model).
    void setValueQuietly(double value);
    void stepBy(int steps) override;

protected:
    void wheelEvent(QWheelEvent* event) override;
};

} // namespace avloop
