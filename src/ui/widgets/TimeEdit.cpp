#include "ui/widgets/TimeEdit.h"

#include "core/Timecode.h"

#include <QFontMetrics>
#include <QKeyEvent>

namespace avloop {

TimeEdit::TimeEdit(const QString& name, QWidget* parent) : QLineEdit(parent) {
    setObjectName(name);
    setProperty("mono", true);
    setAlignment(Qt::AlignCenter);
    // Measured in the face the style sheet gives it (Consolas), not the default one.
    QFont mono(QStringLiteral("Consolas"));
    mono.setPointSizeF(font().pointSizeF());
    setFixedWidth(QFontMetrics(mono).horizontalAdvance(QStringLiteral("00:00:00.00")) + 26);
    setToolTip(tr("Type a time and press Enter: 12.4 (seconds), 00:00:12.12 or 372f (a frame)"));
    connect(this, &QLineEdit::returnPressed, this, [this] {
        double t = 0;
        if (timecode::parse(text(), fps_, &t)) {
            seconds_ = t;
            emit timeEntered(t);
        }
        showTime();
        clearFocus(); // the keys are the timeline's again (I, O, Space …)
    });
    showTime();
}

void TimeEdit::setFps(Rational fps) {
    fps_ = fps;
    showTime();
}

void TimeEdit::setSeconds(double seconds) {
    seconds_ = seconds;
    if (!hasFocus() || !isModified()) showTime();
}

void TimeEdit::showTime() {
    setText(timecode::format(seconds_, fps_));
    setModified(false);
}

void TimeEdit::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        showTime();
        clearFocus();
        return;
    }
    QLineEdit::keyPressEvent(event);
}

void TimeEdit::focusOutEvent(QFocusEvent* event) {
    QLineEdit::focusOutEvent(event);
    showTime();
}

} // namespace avloop
