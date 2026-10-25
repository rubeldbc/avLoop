// The steps of a walk (control/04 §3): real mouse, wheel and key events sent through the widgets' own handlers,
// pictures by QWidget::grab, comparisons against probes, the ☰ menu pressed item by item.
#include "ui/WalkRunner.h"

#include <QApplication>
#include <QDir>
#include <QJsonArray>
#include <QMenu>
#include <QMouseEvent>
#include <QPixmap>
#include <QTest>
#include <QTimer>
#include <QToolButton>
#include <QWheelEvent>
#include <QWidget>

#include <climits>
#include <cmath>

namespace avloop {

namespace {

Qt::KeyboardModifiers modifiersOf(const QJsonObject& step) {
    const QString text = step.value(QStringLiteral("mods")).toString();
    Qt::KeyboardModifiers mods;
    if (text.contains(QLatin1String("Shift"), Qt::CaseInsensitive)) mods |= Qt::ShiftModifier;
    if (text.contains(QLatin1String("Ctrl"), Qt::CaseInsensitive)) mods |= Qt::ControlModifier;
    if (text.contains(QLatin1String("Alt"), Qt::CaseInsensitive)) mods |= Qt::AltModifier;
    return mods;
}

Qt::MouseButton buttonOf(const QJsonObject& step) {
    const QString b = step.value(QStringLiteral("button")).toString();
    if (b == QLatin1String("right")) return Qt::RightButton;
    if (b == QLatin1String("middle")) return Qt::MiddleButton;
    return Qt::LeftButton;
}

// Sends one mouse event to the deepest child under `pos`, as the window system would deliver it.
void sendMouse(QWidget* widget, QEvent::Type type, QPoint pos, Qt::MouseButton button, Qt::MouseButtons buttons,
               Qt::KeyboardModifiers mods) {
    QWidget* hit = widget->childAt(pos);
    if (hit && type != QEvent::MouseMove) {
        pos = hit->mapFrom(widget, pos);
        widget = hit;
    }
    const QPointF local(pos);
    const QPointF inWindow = widget->mapTo(widget->window(), local);
    const QPointF global = widget->mapToGlobal(local);
    QMouseEvent event(type, local, inWindow, global, button, buttons, mods);
    QApplication::sendEvent(widget, &event);
}

QString text(const QVariant& v) {
    if (v.typeId() == QMetaType::Bool) return v.toBool() ? QStringLiteral("true") : QStringLiteral("false");
    if (v.typeId() == QMetaType::Double) return QString::number(v.toDouble(), 'g', 10);
    return v.toString();
}

} // namespace

QPoint WalkRunner::point(QWidget* widget, const QJsonObject& step, const QString& key) const {
    const QJsonValue v = step.value(key);
    if (v.isArray()) {
        const QJsonArray a = v.toArray();
        QPointF p(a.at(0).toDouble(), a.at(1).toDouble());
        if (step.value(QStringLiteral("space")).toString() == QLatin1String("frac"))
            p = QPointF(p.x() * widget->width(), p.y() * widget->height());
        else if (auto map = WalkProbes::mapper(step.value(QStringLiteral("space")).toString()))
            p = map(p);
        return p.toPoint();
    }
    return widget->rect().center();
}

int WalkRunner::stepClick(const QJsonObject& step, bool twice) {
    QWidget* w = target(step);
    if (!w) {
        check(false, QStringLiteral("click"), QStringLiteral("no widget \"%1\"").arg(step.value("on").toString()));
        return -1;
    }
    const QPoint at = point(w, step, QStringLiteral("at"));
    const Qt::MouseButton b = buttonOf(step);
    const Qt::KeyboardModifiers mods = modifiersOf(step);
    sendMouse(w, QEvent::MouseButtonPress, at, b, b, mods);
    sendMouse(w, QEvent::MouseButtonRelease, at, b, Qt::NoButton, mods);
    if (twice) {
        sendMouse(w, QEvent::MouseButtonDblClick, at, b, b, mods);
        sendMouse(w, QEvent::MouseButtonRelease, at, b, Qt::NoButton, mods);
    }
    return delay_;
}

int WalkRunner::stepDrag(const QJsonObject& step) {
    QWidget* w = target(step);
    if (!w) {
        check(false, QStringLiteral("drag"), QStringLiteral("no widget \"%1\"").arg(step.value("on").toString()));
        return -1;
    }
    const QPoint from = point(w, step, QStringLiteral("from"));
    const QPoint to = point(w, step, QStringLiteral("to"));
    const int steps = qMax(1, step.value(QStringLiteral("steps")).toInt(12));
    const Qt::MouseButton b = buttonOf(step);
    const Qt::KeyboardModifiers mods = modifiersOf(step);
    sendMouse(w, QEvent::MouseButtonPress, from, b, b, mods);
    for (int i = 1; i <= steps; ++i) {
        const QPointF p = QPointF(from) + (QPointF(to) - QPointF(from)) * (double(i) / steps);
        sendMouse(w, QEvent::MouseMove, p.toPoint(), Qt::NoButton, b, mods);
        QApplication::processEvents();
    }
    sendMouse(w, QEvent::MouseButtonRelease, to, b, Qt::NoButton, mods);
    return delay_;
}

int WalkRunner::stepWheel(const QJsonObject& step) {
    QWidget* w = target(step);
    if (!w) {
        check(false, QStringLiteral("wheel"), QStringLiteral("no widget \"%1\"").arg(step.value("on").toString()));
        return -1;
    }
    const QPoint at = point(w, step, QStringLiteral("at"));
    const int delta = step.value(QStringLiteral("delta")).toInt(120);
    const bool horizontal = step.value(QStringLiteral("horizontal")).toBool();
    const QPoint angle = horizontal ? QPoint(delta, 0) : QPoint(0, delta);
    QWheelEvent event(QPointF(at), QPointF(w->mapToGlobal(at)), QPoint(), angle, Qt::NoButton, modifiersOf(step),
                      Qt::NoScrollPhase, false);
    QApplication::sendEvent(w, &event);
    return delay_;
}

int WalkRunner::stepKey(const QJsonObject& step) {
    const QKeySequence seq = QKeySequence::fromString(step.value(QStringLiteral("keys")).toString());
    if (seq.isEmpty()) {
        check(false, QStringLiteral("key"), QStringLiteral("cannot read \"%1\"").arg(step.value("keys").toString()));
        return -1;
    }
    QWidget* w = step.contains(QStringLiteral("on")) ? target(step) : QApplication::focusWidget();
    if (!w) w = window_;
    const QKeyCombination combo = seq[0];
    QTest::keyClick(w, combo.key(), combo.keyboardModifiers());
    return delay_;
}

int WalkRunner::stepType(const QJsonObject& step) {
    QWidget* w = target(step);
    if (!w) {
        check(false, QStringLiteral("type"), QStringLiteral("no widget \"%1\"").arg(step.value("on").toString()));
        return -1;
    }
    w->setFocus(Qt::OtherFocusReason);
    QTest::keyClick(w, Qt::Key_A, Qt::ControlModifier);
    QTest::keyClicks(w, step.value(QStringLiteral("text")).toString());
    if (step.value(QStringLiteral("enter")).toBool(true)) QTest::keyClick(w, Qt::Key_Return);
    return delay_;
}

int WalkRunner::stepShot(const QJsonObject& step) {
    QWidget* w = target(step);
    if (!w) {
        check(false, QStringLiteral("shot"), QStringLiteral("no widget \"%1\"").arg(step.value("on").toString()));
        return -1;
    }
    QPixmap pm = w->grab();
    const int maxWidth = step.value(QStringLiteral("maxWidth")).toInt(0);
    if (maxWidth > 0 && pm.width() > maxWidth) pm = pm.scaledToWidth(maxWidth, Qt::SmoothTransformation);
    const QString file = QDir(out_).filePath(step.value(QStringLiteral("file")).toString());
    QDir().mkpath(QFileInfo(file).absolutePath());
    const bool saved = pm.save(file, "PNG");
    check(saved, QStringLiteral("shot %1").arg(QFileInfo(file).fileName()),
          QStringLiteral("%1x%2 px").arg(pm.width()).arg(pm.height()));
    return delay_;
}

int WalkRunner::stepExpect(const QJsonObject& step) {
    const QString probe = step.value(QStringLiteral("probe")).toString();
    if (!WalkProbes::has(probe)) {
        check(false, QStringLiteral("expect %1").arg(probe), QStringLiteral("no such probe"));
        return delay_;
    }
    const QVariant value = WalkProbes::read(probe);
    const QString observed = text(value);
    bool ok = true;
    QString wanted;
    if (step.contains(QStringLiteral("eq"))) {
        const QJsonValue eq = step.value(QStringLiteral("eq"));
        if (eq.isDouble())
            ok = std::abs(value.toDouble() - eq.toDouble()) < 1e-9;
        else if (eq.isBool())
            ok = value.toBool() == eq.toBool();
        else
            ok = observed == eq.toString();
        wanted = QStringLiteral("= %1").arg(eq.isString() ? eq.toString() : text(eq.toVariant()));
    }
    if (step.contains(QStringLiteral("near"))) {
        const QJsonArray near = step.value(QStringLiteral("near")).toArray();
        ok = ok && std::abs(value.toDouble() - near.at(0).toDouble()) <= near.at(1).toDouble();
        wanted = QStringLiteral("≈ %1 ± %2").arg(near.at(0).toDouble()).arg(near.at(1).toDouble());
    }
    if (step.contains(QStringLiteral("min")))
        ok = ok && value.toDouble() >= step.value(QStringLiteral("min")).toDouble();
    if (step.contains(QStringLiteral("max")))
        ok = ok && value.toDouble() <= step.value(QStringLiteral("max")).toDouble();
    if (step.contains(QStringLiteral("contains")))
        ok = ok && observed.contains(step.value(QStringLiteral("contains")).toString());
    check(ok, QStringLiteral("expect %1").arg(probe),
          ok || wanted.isEmpty() ? observed : observed + QStringLiteral(" (wanted ") + wanted + QLatin1Char(')'));
    return delay_;
}

int WalkRunner::stepWaitFor(const QJsonObject& step) {
    const QString probe = step.value(QStringLiteral("probe")).toString();
    const int timeout = step.value(QStringLiteral("timeout")).toInt(10000);
    waitTicks_ = 0;
    auto* timer = new QTimer(this);
    timer->setInterval(50);
    connect(timer, &QTimer::timeout, this, [this, timer, step, probe, timeout] {
        const QString observed = text(WalkProbes::read(probe));
        const bool done = observed == step.value(QStringLiteral("eq")).toVariant().toString();
        if (done || ++waitTicks_ * 50 >= timeout) {
            timer->deleteLater();
            check(done, QStringLiteral("waitFor %1").arg(probe),
                  QStringLiteral("%1 after %2 ms").arg(observed).arg(waitTicks_ * 50));
            QTimer::singleShot(delay_, this, &WalkRunner::next);
        }
    });
    timer->start();
    return INT_MAX;
}

int WalkRunner::stepMenu(const QJsonObject& step) {
    menuPath_.clear();
    for (const QJsonValue& v : step.value(QStringLiteral("path")).toArray())
        menuPath_ << v.toString();
    auto* button = window_ ? window_->findChild<QToolButton*>(QStringLiteral("menu.button")) : nullptr;
    if (!button || menuPath_.isEmpty()) {
        check(false, QStringLiteral("menu"), QStringLiteral("no menu button or empty path"));
        return -1;
    }
    // The menu runs its own event loop once opened: the next press is armed before the button is pressed.
    QTimer::singleShot(250, this, &WalkRunner::menuNext);
    const QPoint c = button->rect().center();
    sendMouse(button, QEvent::MouseButtonPress, c, Qt::LeftButton, Qt::LeftButton, {});
    return INT_MAX;
}

void WalkRunner::menuNext() {
    auto* menu = qobject_cast<QMenu*>(QApplication::activePopupWidget());
    const QString wanted = menuPath_.isEmpty() ? QString() : menuPath_.takeFirst();
    QAction* found = nullptr;
    if (menu)
        for (QAction* a : menu->actions())
            if (a->text().remove(QLatin1Char('&')) == wanted) found = a;
    if (!menu || !found || !found->isEnabled()) {
        check(false, QStringLiteral("menu"), QStringLiteral("\"%1\" not found or disabled").arg(wanted));
        while (QWidget* popup = QApplication::activePopupWidget())
            popup->close();
        QTimer::singleShot(delay_, this, &WalkRunner::next);
        return;
    }
    const QPoint at = menu->actionGeometry(found).center();
    sendMouse(menu, QEvent::MouseMove, at, Qt::NoButton, Qt::NoButton, {});
    sendMouse(menu, QEvent::MouseButtonPress, at, Qt::LeftButton, Qt::LeftButton, {});
    sendMouse(menu, QEvent::MouseButtonRelease, at, Qt::LeftButton, Qt::NoButton, {});
    log(QStringLiteral("     menu: %1").arg(wanted));
    if (!menuPath_.isEmpty())
        QTimer::singleShot(250, this, &WalkRunner::menuNext);
    else
        QTimer::singleShot(delay_ + 150, this, &WalkRunner::next);
}

} // namespace avloop
