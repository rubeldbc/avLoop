#include "ui/widgets/Toast.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStyle>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>

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
    // The text, then its buttons (if any) in a row at the right.
    auto* column = new QVBoxLayout(this);
    column->setContentsMargins(12, 8, 10, 8);
    column->setSpacing(6);
    label_ = new QLabel(text, this);
    label_->setWordWrap(true);
    column->addWidget(label_);
    host->installEventFilter(this);
}

Toast* Toast::post(QWidget* host, const QString& text, Kind kind, int ms, const QString& actionText,
                   std::function<void()> action) {
    if (!host) return nullptr;
    if (Toast* old = current(host)) old->dismiss();
    auto* toast = new Toast(host, text, kind);
    if (!actionText.isEmpty() && action) toast->addButton(actionText, std::move(action));
    toast->fit();
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

void Toast::addButton(const QString& text, std::function<void()> action) {
    auto* button = new QPushButton(text, this);
    button->setObjectName(buttons_++ == 0 ? QStringLiteral("toast.action") : QStringLiteral("toast.action2"));
    button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    if (!buttonRow_) {
        buttonRow_ = new QHBoxLayout;
        buttonRow_->setSpacing(8);
        buttonRow_->addStretch(1);
        static_cast<QVBoxLayout*>(layout())->addLayout(buttonRow_);
    }
    buttonRow_->addWidget(button);
    connect(button, &QPushButton::clicked, this, [this, action] {
        action();
        dismiss();
    });
    if (isVisible()) fit();
}

void Toast::fit() {
    // A wrapped label asks for little width: give it its longest line, up to its maximum.
    int widest = 0;
    for (const QString& line : label_->text().split(QLatin1Char('\n')))
        widest = std::max(widest, label_->fontMetrics().horizontalAdvance(line) + 4);
    label_->setFixedWidth(std::min(widest, 340)); // its height then follows the wrapped lines
    adjustSize();
    place();
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
