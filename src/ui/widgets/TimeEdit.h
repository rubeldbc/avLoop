#pragma once

#include "core/Rational.h"

#include <QLineEdit>

namespace avloop {

// A time field (spec §7.3, §6.4): it shows HH:MM:SS.FF at the output frame rate; "12.4", "00:00:12.12" or "372f" and
// Enter jump there (timeEntered) and leave the field, so the keys work the timeline again. Esc or leaving the field
// puts the shown time back.
class TimeEdit : public QLineEdit {
    Q_OBJECT
public:
    explicit TimeEdit(const QString& name, QWidget* parent = nullptr);

    void setFps(Rational fps);
    void setSeconds(double seconds); // shown unless the user is typing
    double seconds() const { return seconds_; }

signals:
    void timeEntered(double seconds);

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;

private:
    void showTime();

    Rational fps_{30, 1};
    double seconds_ = 0;
};

} // namespace avloop
