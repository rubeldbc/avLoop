// The main window's project (spec §11.1–§11.4): New, Open Project, Save (Ctrl+S), Save As (Ctrl+Shift+S) and Recent
// in the ☰ menu; the title `AvLoop — <name>.avloop*`; Save / Don't save / Cancel before the work is replaced or the
// window closes; autosave every N minutes to the data folder, offered back after a crash; Settings (Ctrl+,), About and
// the F1 list. A lab run answers the questions from its walk and never opens a dialog.
#include "core/AppInfo.h"
#include "core/AppSettings.h"
#include "core/Project.h"
#include "core/ProjectJson.h"
#include "media/DiskCache.h"
#include "media/FrameCache.h"
#include "ui/MainWindow.h"
#include "ui/WalkRunner.h"
#include "ui/dialogs/AboutDialog.h"
#include "ui/dialogs/SettingsDialog.h"
#include "ui/dialogs/ShortcutOverlay.h"
#include "ui/panels/SidePanel.h"
#include "ui/theme/Theme.h"
#include "ui/widgets/Toast.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QTimer>

#include <windows.h>

namespace avloop {

namespace {

bool processAlive(qint64 pid) {
    const HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, DWORD(pid));
    if (!h) return false;
    DWORD code = 0;
    const bool alive = GetExitCodeProcess(h, &code) && code == STILL_ACTIVE;
    CloseHandle(h);
    return alive;
}

bool lab() {
    return qApp->property("avloop.lab").toBool();
}

} // namespace

void MainWindow::buildProject() {
    auto action = [this](const char* name) { return findChild<QAction*>(QLatin1String(name)); };
    connect(action("action.new"), &QAction::triggered, this, &MainWindow::newProject);
    connect(action("action.openProject"), &QAction::triggered, this, &MainWindow::chooseProject);
    connect(action("action.save"), &QAction::triggered, this, [this] { saveProject(); });
    connect(action("action.saveAs"), &QAction::triggered, this, [this] { saveProjectAs(); });
    connect(action("action.settings"), &QAction::triggered, this, &MainWindow::openSettings);
    connect(action("action.about"), &QAction::triggered, this, &MainWindow::openAbout);
    connect(action("action.shortcuts"), &QAction::triggered, this, &MainWindow::toggleShortcuts);
    if (auto* recent = findChild<QMenu*>(QStringLiteral("menu.recent")))
        connect(recent, &QMenu::aboutToShow, this, [this, recent] { fillRecent(recent); });
    side_->bindPresets(
        project_,
        [this] {
            if (lab()) return labText_;
            bool ok = false;
            const QString name =
                QInputDialog::getText(this, tr("Save preset"), tr("Name"), QLineEdit::Normal, QString(), &ok);
            return ok ? name : QString();
        },
        [this](const QString& text) { return confirm(text); });
    connect(project_->undoStack(), &QUndoStack::cleanChanged, this, &MainWindow::refreshTitle);
    connect(project_, &Project::changed, this, [this](Project::Aspect a) {
        if (a == Project::Aspect::Source) refreshTitle();
    });

    autosaveTimer_ = new QTimer(this);
    autosaveTimer_->setInterval(appsettings::autosaveMinutes() * 60000);
    connect(autosaveTimer_, &QTimer::timeout, this, &MainWindow::autosave);
    autosaveTimer_->start();
    autosaveFile_ = QDir(appsettings::autosaveDir())
                        .filePath(QStringLiteral("session-%1.avloop").arg(QApplication::applicationPid()));

    WalkProbes::add(QStringLiteral("project.path"), [this] { return QDir::toNativeSeparators(projectPath_); });
    WalkProbes::add(QStringLiteral("project.dirty"), [this] { return projectDirty(); });
    WalkProbes::add(QStringLiteral("window.title"), [this] {
        return windowTitle().replace(QStringLiteral("[*]"), isWindowModified() ? QStringLiteral("*") : QString());
    });
    WalkProbes::add(QStringLiteral("autosave.exists"), [this] { return QFileInfo::exists(autosaveFile_); });
    WalkProbes::add(QStringLiteral("recent.projects"),
                    [] { return appsettings::recent(appsettings::Recent::Projects).join(QLatin1Char('|')); });
    WalkProbes::add(QStringLiteral("settings.open"), [this] { return !settings_.isNull(); });
    WalkProbes::add(QStringLiteral("shortcuts.open"),
                    [this] { return ShortcutOverlay::find(centralWidget()) != nullptr; });
    // The About box's labels as written (rich text, the links' targets with them), joined by "|"; empty when closed.
    WalkProbes::add(QStringLiteral("about.text"), [] {
        QStringList texts;
        for (QWidget* top : QApplication::topLevelWidgets())
            if (top->objectName() == QLatin1String("AboutDialog") && top->isVisible())
                for (const QLabel* label : top->findChildren<QLabel*>())
                    texts << label->text();
        return texts.join(QLatin1Char('|'));
    });
    WalkProbes::add(QStringLiteral("theme"), [] { return Theme::idName(Theme::instance().id()); });
    // {"do": "saveAs", "file": "x.avloop"}: Save As with its dialog answered (in the walk's folder).
    WalkRunner::addStep(QStringLiteral("saveAs"), [this](WalkRunner& walk, const QJsonObject& step) {
        const QString file = QDir(walk.outDir()).absoluteFilePath(step.value(QStringLiteral("file")).toString());
        walk.log(QStringLiteral("     saveAs %1").arg(file));
        saveProjectAs(file);
        return 300;
    });
    WalkProbes::add(QStringLiteral("source.file"),
                    [this] { return video_ ? QDir::toNativeSeparators(video_->path) : QString(); });
    WalkProbes::add(QStringLiteral("cache.budgetMB"), [this] { return double(cache_->budget() >> 20); });
    WalkProbes::add(QStringLiteral("autosave.minutes"), [this] { return autosaveTimer_->interval() / 60000; });
    // {"do": "recover"}: the start's recovery offer, now (a lab run makes it only when its walk asks).
    WalkRunner::addStep(QStringLiteral("recover"), [this](WalkRunner& walk, const QJsonObject&) {
        walk.log(QStringLiteral("     recover"));
        offerRecovery();
        return 400;
    });
    // {"do": "autosaveNow"}: the autosave timer's turn, now.
    WalkRunner::addStep(QStringLiteral("autosaveNow"), [this](WalkRunner& walk, const QJsonObject&) {
        walk.log(QStringLiteral("     autosaveNow"));
        autosave();
        return 200;
    });
    // {"do": "crash"}: an access violation, as a bug would make one — the minidump and the autosave stay behind.
    WalkRunner::addStep(QStringLiteral("crash"), [](WalkRunner& walk, const QJsonObject&) {
        walk.log(QStringLiteral("     crash"));
        qCritical("[lab] a crash on purpose");
        *static_cast<volatile int*>(nullptr) = 1;
        return 0;
    });
    refreshTitle();
}

bool MainWindow::projectDirty() const {
    return video_ && (!project_->undoStack()->isClean() || recoveredDirty_);
}

void MainWindow::refreshTitle() {
    QString name;
    if (!projectPath_.isEmpty())
        name = QFileInfo(projectPath_).fileName();
    else if (video_)
        name = QFileInfo(video_->path).fileName();
    setWindowTitle(name.isEmpty() ? AppInfo::displayName()
                                  : QStringLiteral("%1 — %2[*]").arg(AppInfo::displayName(), name));
    setWindowModified(projectDirty());
}

int MainWindow::ask(const QString& text, const QStringList& choices) {
    lastQuestion_ = text;
    if (lab()) return std::max(0, int(choices.indexOf(labChoice_)));
    QMessageBox box(QMessageBox::Question, AppInfo::displayName(), text, QMessageBox::NoButton, this);
    QList<QPushButton*> buttons;
    for (const QString& c : choices)
        buttons << box.addButton(c, buttons.isEmpty() ? QMessageBox::AcceptRole : QMessageBox::RejectRole);
    box.setDefaultButton(buttons.front());
    box.exec();
    return std::max(0, int(buttons.indexOf(qobject_cast<QPushButton*>(box.clickedButton()))));
}

bool MainWindow::maySaveChanges() {
    if (!projectDirty()) return true;
    const QString name = projectPath_.isEmpty() ? tr("this project") : QFileInfo(projectPath_).fileName();
    const QStringList choices = {tr("Save"), tr("Don't save"), tr("Cancel")};
    const int choice = ask(tr("Save the changes to %1?").arg(name), choices);
    if (choice == 0) return saveProject();
    return choice == 1;
}

void MainWindow::projectOpened(const QString& projectFile, const ProjectData& read) {
    recoveredDirty_ = false;
    projectPath_ = projectFile;
    QJsonObject extra = project_->extra();
    if (extra.contains(QStringLiteral("autosave"))) {
        // A recovered autosave: the work goes back to its own file (if it had one), still to be saved.
        projectPath_ = extra.value(QStringLiteral("autosave")).toObject().value(QStringLiteral("project")).toString();
        extra.remove(QStringLiteral("autosave"));
        project_->setExtra(extra);
        recoveredDirty_ = true;
        QFile::remove(projectFile);
    } else if (!projectFile.isEmpty()) {
        appsettings::addRecent(appsettings::Recent::Projects, projectFile);
    }
    if (video_ && read.source.fileSize > 0) {
        const QFileInfo now(video_->path);
        if (now.size() != read.source.fileSize ||
            std::abs(now.lastModified().secsTo(read.source.modified)) > 2) // spec §11.1: a warning, still opened
            Toast::post(centralWidget(), tr("The video has changed since this project was saved."),
                        Toast::Kind::Warning, 8000);
    }
    refreshTitle();
}

void MainWindow::videoOpened(const QString& path) {
    recoveredDirty_ = false;
    projectPath_.clear();
    appsettings::addRecent(appsettings::Recent::Videos, path);
    refreshTitle();
}

void MainWindow::newProject() {
    if (!maySaveChanges()) return;
    closeVideo();
    project_->restore(ProjectData{});
    projectPath_.clear();
    recoveredDirty_ = false;
    QFile::remove(autosaveFile_);
    refreshTitle();
}

void MainWindow::chooseProject() {
    if (!maySaveChanges()) return;
    const QString dir = appsettings::value("ui/lastOpenDir", QDir::homePath()).toString();
    const QString path =
        QFileDialog::getOpenFileName(this, tr("Open a project"), dir, tr("AvLoop projects (*.avloop)"));
    if (path.isEmpty()) return;
    appsettings::setValue("ui/lastOpenDir", QFileInfo(path).absolutePath());
    openProject(path);
}

bool MainWindow::saveProject() {
    if (!video_) return false;
    if (projectPath_.isEmpty()) return saveProjectAs();
    QString error;
    if (!projectjson::save(project_->snapshot(), projectPath_, &error)) {
        Toast::post(centralWidget(), tr("Cannot save %1: %2").arg(QFileInfo(projectPath_).fileName(), error),
                    Toast::Kind::Error, 8000);
        return false;
    }
    project_->undoStack()->setClean();
    recoveredDirty_ = false;
    QFile::remove(autosaveFile_);
    appsettings::addRecent(appsettings::Recent::Projects, projectPath_);
    refreshTitle();
    return true;
}

bool MainWindow::saveProjectAs(const QString& given) {
    if (!video_) return false;
    QString path = given;
    if (path.isEmpty() && lab()) path = labText_; // a lab run: the walk's answer
    if (path.isEmpty() && lab()) return false;
    if (path.isEmpty()) {
        const QString suggested =
            !projectPath_.isEmpty()
                ? projectPath_
                : QDir(appsettings::value("ui/lastOpenDir", QDir::homePath()).toString())
                      .filePath(QFileInfo(video_->path).completeBaseName() + QStringLiteral(".avloop"));
        path = QFileDialog::getSaveFileName(this, tr("Save the project"), suggested, tr("AvLoop projects (*.avloop)"));
        if (path.isEmpty()) return false;
    }
    if (!path.endsWith(QLatin1String(".avloop"), Qt::CaseInsensitive)) path += QStringLiteral(".avloop");
    const QString before = projectPath_;
    projectPath_ = path;
    if (saveProject()) return true;
    projectPath_ = before;
    return false;
}

void MainWindow::fillRecent(QMenu* menu) {
    menu->clear();
    auto add = [&](appsettings::Recent kind) {
        for (const QString& p : appsettings::recent(kind)) {
            QAction* a = menu->addAction(QFileInfo(p).fileName());
            a->setToolTip(p);
            connect(a, &QAction::triggered, this, [this, p] {
                if (!QFileInfo::exists(p)) {
                    Toast::post(centralWidget(), tr("%1 is no longer there.").arg(p), Toast::Kind::Warning);
                    return;
                }
                if (maySaveChanges()) openFiles({p});
            });
        }
    };
    add(appsettings::Recent::Projects);
    if (!menu->isEmpty()) menu->addSeparator();
    add(appsettings::Recent::Videos);
    if (menu->isEmpty()) {
        menu->addAction(tr("Nothing yet"))->setEnabled(false);
        return;
    }
    menu->addSeparator();
    connect(menu->addAction(tr("Clear the list")), &QAction::triggered, this, [] { appsettings::clearRecent(); });
}

void MainWindow::autosave() {
    if (!projectDirty()) return;
    ProjectData d = project_->snapshot();
    d.original.insert(QStringLiteral("autosave"),
                      QJsonObject{{QStringLiteral("pid"), double(QApplication::applicationPid())},
                                  {QStringLiteral("project"), projectPath_},
                                  {QStringLiteral("saved"), QDateTime::currentDateTime().toString(Qt::ISODate)}});
    QString error;
    if (!projectjson::save(d, autosaveFile_, &error)) qWarning().noquote() << "[project] autosave failed:" << error;
}

void MainWindow::removeAutosave() {
    QFile::remove(autosaveFile_);
}

// Spec §11.1: an autosave left by a process that is gone (a crash) is offered back at the next start.
void MainWindow::offerRecovery() {
    QFileInfo newest;
    for (const QFileInfo& f :
         QDir(appsettings::autosaveDir()).entryInfoList({QStringLiteral("*.avloop")}, QDir::Files, QDir::Time)) {
        QFile file(f.absoluteFilePath());
        if (!file.open(QIODevice::ReadOnly)) continue;
        const qint64 pid = qint64(QJsonDocument::fromJson(file.readAll())
                                      .object()
                                      .value(QStringLiteral("autosave"))
                                      .toObject()
                                      .value(QStringLiteral("pid"))
                                      .toDouble());
        if (pid == QApplication::applicationPid() || (pid > 0 && processAlive(pid))) continue; // another AvLoop's
        if (!newest.exists()) newest = f;
    }
    if (!newest.exists()) return;
    const QString when = newest.lastModified().toString(QStringLiteral("HH:mm"));
    if (ask(tr("Recover unsaved project? (autosaved at %1)").arg(when), {tr("Recover"), tr("Discard")}) == 0)
        openProject(newest.absoluteFilePath());
    else
        QFile::remove(newest.absoluteFilePath());
}

void MainWindow::openSettings() {
    if (settings_) {
        if (!lab()) settings_->raise();
        return;
    }
    settings_ = new SettingsDialog(this);
    connect(settings_, &SettingsDialog::themeChanged, this, [this](const QString& id) { setThemeFromMenu(id); });
    connect(settings_, &SettingsDialog::ramCacheChanged, this,
            [this](int mb) { cache_->setBudget(std::int64_t(mb) << 20); });
    connect(settings_, &SettingsDialog::autosaveChanged, this,
            [this](int minutes) { autosaveTimer_->setInterval(minutes * 60000); });
    connect(settings_, &SettingsDialog::clearCacheRequested, this, [this] {
        cache_->clear();
        disk_->clear();
        Toast::post(centralWidget(), tr("The cache is cleared."), Toast::Kind::Info);
    });
    showQuietly(settings_);
}

void MainWindow::openAbout() {
    showQuietly(new AboutDialog(this));
}

void MainWindow::showQuietly(QWidget* window) {
    if (!lab()) {
        window->show();
        return;
    }
    window->setAttribute(Qt::WA_ShowWithoutActivating); // a lab run never takes the focus
    window->show();
    WalkRunner::placeQuietly(window, window->size());
}

void MainWindow::toggleShortcuts() {
    if (ShortcutOverlay* open = ShortcutOverlay::find(centralWidget())) {
        open->close();
        return;
    }
    ShortcutOverlay::Rows rows;
    QStringList seen;
    // The actions' own shortcuts first (the menu's, Preview's, Export's), so the list follows them.
    for (QAction* a : findChildren<QAction*>()) {
        QString text = a->text().remove(QLatin1Char('&')).remove(QStringLiteral("…"));
        const bool stack =
            a->objectName() == QLatin1String("action.undo") || a->objectName() == QLatin1String("action.redo");
        if (text.isEmpty() || a->shortcut().isEmpty() || seen.contains(text) || (!a->isEnabled() && !stack)) continue;
        seen << text;
        rows << qMakePair(text, a->shortcut().toString(QKeySequence::NativeText));
    }
    for (const ToolInfo& t : tools())
        rows << qMakePair(tr("Tool: %1").arg(t.name), t.key.toString(QKeySequence::NativeText));
    // The keys the canvas and the timeline read themselves (spec §11.4).
    rows << qMakePair(tr("Play / Pause"), QStringLiteral("Space"))
         << qMakePair(tr("Loop playback"), QStringLiteral("L"))
         << qMakePair(tr("Previous / next frame"), QStringLiteral("← / →"))
         << qMakePair(tr("Ten frames"), QStringLiteral("Shift+← / →"))
         << qMakePair(tr("Go to In / Out"), QStringLiteral("Home / End"))
         << qMakePair(tr("Start / end of the file"), QStringLiteral("Ctrl+Home / End"))
         << qMakePair(tr("Clear In / Out"), QStringLiteral("Alt+I / Alt+O"))
         << qMakePair(tr("Zoom: Fit · 100% · in · out"), QStringLiteral("Ctrl+0 · 1 · = · -"))
         << qMakePair(tr("Fit the timeline · In–Out"), QStringLiteral("\\ / Shift+\\"))
         << qMakePair(tr("Shortcut list"), QStringLiteral("F1"));
    new ShortcutOverlay(centralWidget(), rows);
}

} // namespace avloop
