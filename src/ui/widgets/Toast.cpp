#include "ui/widgets/Toast.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStyle>
#include <QTimer>

namespace avloop {

namespace {

const char* kindName(Toast::Kind kind) {
    switch (kind) {
    case Toast::Kind::Success:
        return "success";
    case Toast::Kind::Warning:
        return "warning";
    case Toast::Kind::Error:
        return "error";
    case Toast::Kind::Info:
        break;
    }
    return "info";
}

} // namespace

Toast::Toast(QWidget* host, const QString& text, Kind kind) : QFrame(host), kind_(kind) {
    setObjectName(QStringLiteral("Toast"));
    setProperty("kind", QString::fromLatin1(kindName(kind)));
    setAttribute(Qt::WA_StyledBackground);
    auto* row = new QHBoxLayout(this);
    row->setContentsMargins(12, 8, 10, 8);
    row->setSpacing(10);
    label_ = new QLabel(text, this);
    label_->setWordWrap(true);
    label_->setMaximumWidth(340);
    row->addWidget(label_);
    host->installEventFilter(this);
}

Toast* Toast::post(QWidget* host, const QString& text, Kind kind, int ms, const QString& actionText,
                   std::function<void()> action) {
    if (!host) return nullptr;
    if (Toast* old = current(host)) old->dismiss();
    auto* toast = new Toast(host, text, kind);
    if (!actionText.isEmpty() && action) {
        toast->button_ = new QPushButton(actionText, toast);
        toast->layout()->addWidget(toast->button_);
        connect(toast->button_, &QPushButton::clicked, toast, [toast, action] {
            action();
            toast->dismiss();
        });
    }
    toast->adjustSize();
    toast->place();
    toast->show();
    toast->raise();
    if (ms > 0) QTimer::singleShot(ms, toast, &Toast::dismiss);
    return toast;
}

Toast* Toast::current(QWidget* host) {
    if (!host) return nullptr;
    const auto toasts = host->findChildren<Toast*>(Qt::FindDirectChildrenOnly);
    for (Toast* t : toasts)
        if (!t->property("avloopClosing").toBool()) return t;
    return nullptr;
}

QString Toast::text() const {
    return label_->text();
}

void Toast::dismiss() {
    setProperty("avloopClosing", true);
    hide();
    deleteLater();
}

bool Toast::eventFilter(QObject* watched, QEvent* event) {
    if (watched == parentWidget() && event->type() == QEvent::Resize) place();
    return QFrame::eventFilter(watched, event);
}

void Toast::place() {
    QWidget* host = parentWidget();
    const int margin = 16;
    move(host->width() - width() - margin, host->height() - height() - margin);
}

} // namespace avloop
