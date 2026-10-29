#include "ui/widgets/ResetLabel.h"

#include "ui/widgets/ColourField.h"
#include "ui/widgets/StepSpinBox.h"

#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QMouseEvent>

namespace avloop {

namespace reset {

namespace {

constexpr const char* kDefault = "avloop.default";

QList<QWidget*> controlsIn(const QWidget* control) {
    QList<QWidget*> found;
    for (QWidget* w : control->findChildren<QWidget*>())
        if (w->property(kDefault).isValid()) found << w;
    return found;
}

bool applyOne(QWidget* w) {
    const QVariant value = w->property(kDefault);
    if (!value.isValid()) return false;
    if (auto* box = qobject_cast<StepSpinBox*>(w)) {
        box->setValue(value.toDouble()); // valueChanged when it differs: the field's own edit
    } else if (auto* list = qobject_cast<QComboBox*>(w)) {
        const int i = list->findData(value);
        if (i >= 0 && i != list->currentIndex()) {
            list->setCurrentIndex(i);
            emit list->activated(i);
        }
    } else if (auto* check = qobject_cast<QCheckBox*>(w)) {
        if (check->isChecked() != value.toBool()) check->click();
    } else if (auto* colour = qobject_cast<ColourField*>(w)) {
        const QColor c = value.value<QColor>();
        if (c.rgb() != colour->colour().rgb()) colour->choose(c);
    }
    return true;
}

} // namespace

void setDefault(QWidget* control, const QVariant& value) {
    control->setProperty(kDefault, value);
}

bool hasDefault(const QWidget* control) {
    return control && (control->property(kDefault).isValid() || !controlsIn(control).isEmpty());
}

bool apply(QWidget* control) {
    if (!control || !control->isEnabled()) return false;
    if (control->property(kDefault).isValid()) return applyOne(control);
    bool any = false;
    for (QWidget* w : controlsIn(control))
        any = applyOne(w) || any;
    return any;
}

} // namespace reset

ResetLabel::ResetLabel(const QString& text, QWidget* control, QWidget* parent)
    : QLabel(text, parent), control_(control) {
    if (control && !control->objectName().isEmpty()) setObjectName(control->objectName() + QStringLiteral(".label"));
}

void ResetLabel::enterEvent(QEnterEvent* event) {
    // The defaults are given after the row is made: the cursor and the tip are decided when the pointer comes.
    const bool can = reset::hasDefault(control_);
    setCursor(can ? Qt::PointingHandCursor : Qt::ArrowCursor);
    setToolTip(can ? tr("Click to reset") : QString());
    QLabel::enterEvent(event);
}

void ResetLabel::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && reset::apply(control_)) {
        emit reset();
        event->accept();
        return;
    }
    QLabel::mousePressEvent(event);
}

} // namespace avloop
