#pragma once

#include <QFrame>
#include <QList>
#include <QPair>
#include <QString>

namespace avloop {

// F1's list of shortcuts (spec §11.4): a small panel over the window's centre, two columns of "what — keys"; F1, Esc or
// a click closes it. The window gives the rows, from its actions' own shortcuts where it has them.
class ShortcutOverlay : public QFrame {
    Q_OBJECT
public:
    using Rows = QList<QPair<QString, QString>>;
    ShortcutOverlay(QWidget* host, const Rows& rows);

    static ShortcutOverlay* find(QWidget* host);

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void place();
};

} // namespace avloop
