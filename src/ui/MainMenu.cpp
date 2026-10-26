// The ☰ menu of the main window (spec §5.1): every entry of the spec's list; an entry whose phase has not been built
// yet stands disabled. Shortcuts live on the window so they work with the menu closed (spec §11.4).
#include "ui/MainWindow.h"

#include "ui/theme/IconFactory.h"
#include "ui/theme/Theme.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QMenu>
#include <QSettings>
#include <QToolButton>

namespace avloop {

namespace {

QAction* entry(QMenu* menu, const QString& name, const QString& text, const QKeySequence& key = {},
               const QString& icon = {}) {
    QAction* a = menu->addAction(text);
    a->setObjectName(name);
    if (!key.isEmpty()) a->setShortcut(key);
    if (!icon.isEmpty()) a->setIcon(IconFactory::icon(icon));
    return a;
}

} // namespace

void MainWindow::buildMenu() {
    menu_ = new QMenu(this);
    menu_->setObjectName(QStringLiteral("menu.main"));

    // MainWindowProject.cpp connects New, Open Project, Save, Save As, Recent, Settings, the shortcuts and About.
    entry(menu_, QStringLiteral("action.new"), tr("New"));
    entry(menu_, QStringLiteral("action.openProject"), tr("Open Project…"));
    entry(menu_, QStringLiteral("action.save"), tr("Save"), QKeySequence(Qt::CTRL | Qt::Key_S));
    entry(menu_, QStringLiteral("action.saveAs"), tr("Save As…"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S));
    QMenu* recent = menu_->addMenu(tr("Recent"));
    recent->setObjectName(QStringLiteral("menu.recent"));
    menu_->addSeparator();
    QAction* undo = entry(menu_, QStringLiteral("action.undo"), tr("Undo"), QKeySequence(Qt::CTRL | Qt::Key_Z),
                          QStringLiteral("undo"));
    QAction* redo = entry(menu_, QStringLiteral("action.redo"), tr("Redo"), QKeySequence(Qt::CTRL | Qt::Key_Y),
                          QStringLiteral("redo"));
    menu_->addSeparator();
    entry(menu_, QStringLiteral("action.batch"), tr("Batch Queue…"), {},
          QStringLiteral("batch")); // MainWindowBatch.cpp
    entry(menu_, QStringLiteral("action.batchAdd"), tr("Add to Batch Queue"),
          QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_E));
    entry(menu_, QStringLiteral("action.finder"), tr("Loop Finder…"), QKeySequence(Qt::CTRL | Qt::Key_L),
          QStringLiteral("finder")); // MainWindowFinder.cpp

    QMenu* theme = menu_->addMenu(tr("Theme"));
    theme->setObjectName(QStringLiteral("menu.theme"));
    themeGroup_ = new QActionGroup(theme);
    themeGroup_->setExclusive(true);
    const QList<QPair<ThemeId, QString>> themes = {{ThemeId::Navy, tr("Navy")}, {ThemeId::Black, tr("Black")}};
    for (const auto& [id, label] : themes) {
        QAction* a = entry(theme, QStringLiteral("action.theme.%1").arg(Theme::idName(id)), label);
        a->setCheckable(true);
        a->setChecked(Theme::instance().id() == id);
        a->setData(Theme::idName(id));
        themeGroup_->addAction(a);
    }
    connect(themeGroup_, &QActionGroup::triggered, this,
            [this](QAction* a) { setThemeFromMenu(a->data().toString()); });

    entry(menu_, QStringLiteral("action.settings"), tr("Settings…"), QKeySequence(Qt::CTRL | Qt::Key_Comma),
          QStringLiteral("settings"));
    entry(menu_, QStringLiteral("action.shortcuts"), tr("Keyboard shortcuts"), QKeySequence(Qt::Key_F1));
    menu_->addSeparator();
    entry(menu_, QStringLiteral("action.about"), tr("About AvLoop"));
    QAction* exit = entry(menu_, QStringLiteral("action.exit"), tr("Exit"));
    connect(exit, &QAction::triggered, this, &QWidget::close);

    // Undo and Redo follow the stack.
    for (QAction* a : {undo, redo})
        a->setEnabled(false);
    for (QAction* a : menu_->actions())
        addAction(a);

    menuButton_->setMenu(menu_);
}

void MainWindow::setThemeFromMenu(const QString& name) {
    const ThemeId id = Theme::idFromName(name);
    Theme::instance().apply(id);
    QSettings().setValue(QStringLiteral("ui/theme"), Theme::idName(id));
}

} // namespace avloop
