#include "ui/dialogs/ShortcutOverlay.h"

#include <QEvent>
#include <QGridLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QVBoxLayout>

#include <algorithm>

namespace avloop {

ShortcutOverlay::ShortcutOverlay(QWidget* host, const Rows& rows) : QFrame(host) {
    setObjectName(QStringLiteral("ShortcutOverlay"));
    setAttribute(Qt::WA_StyledBackground);
    setAttribute(Qt::WA_DeleteOnClose);
    setFocusPolicy(Qt::StrongFocus);
    auto* column = new QVBoxLayout(this);
    column->setContentsMargins(20, 14, 20, 16);
    column->setSpacing(10);
    auto* title = new QLabel(tr("Keyboard shortcuts"), this);
    title->setObjectName(QStringLiteral("ShortcutOverlayTitle"));
    column->addWidget(title);
    auto* grid = new QGridLayout;
    grid->setHorizontalSpacing(14);
    grid->setVerticalSpacing(5);
    const int perColumn = int((rows.size() + 1) / 2);
    for (int i = 0; i < rows.size(); ++i) {
        const int c = i < perColumn ? 0 : 3;
        const int r = i < perColumn ? i : i - perColumn;
        auto* what = new QLabel(rows[i].first, this);
        what->setProperty("quiet", true);
        auto* keys = new QLabel(rows[i].second, this);
        keys->setProperty("mono", true);
        grid->addWidget(what, r, c);
        grid->addWidget(keys, r, c + 1);
    }
    grid->setColumnMinimumWidth(2, 24);
    column->addLayout(grid);
    auto* hint = new QLabel(tr("F1 or Esc closes this list."), this);
    hint->setProperty("quiet", true);
    column->addWidget(hint);
    host->installEventFilter(this);
    adjustSize();
    place();
    show();
    raise();
    setFocus();
}

ShortcutOverlay* ShortcutOverlay::find(QWidget* host) {
    return host ? host->findChild<ShortcutOverlay*>(QString(), Qt::FindDirectChildrenOnly) : nullptr;
}

void ShortcutOverlay::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape || event->key() == Qt::Key_F1) {
        close();
        return;
    }
    QFrame::keyPressEvent(event);
}

void ShortcutOverlay::mousePressEvent(QMouseEvent*) {
    close();
}

bool ShortcutOverlay::eventFilter(QObject* watched, QEvent* event) {
    if (watched == parentWidget() && event->type() == QEvent::Resize) place();
    return QFrame::eventFilter(watched, event);
}

void ShortcutOverlay::place() {
    const QWidget* host = parentWidget();
    move((host->width() - width()) / 2, std::max(8, (host->height() - height()) / 2));
}

} // namespace avloop
