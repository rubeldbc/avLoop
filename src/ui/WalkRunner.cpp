#include "ui/WalkRunner.h"

#include "ui/theme/Theme.h"

#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QJsonDocument>
#include <QMap>
#include <QTimer>
#include <QWidget>

#include <windows.h>

#include <climits>
#include <cstdio>

namespace avloop {

namespace {

QMap<QString, WalkProbes::Probe>& probes() {
    static QMap<QString, WalkProbes::Probe> map;
    return map;
}

QHash<QString, WalkProbes::Mapper>& mappers() {
    static QHash<QString, WalkProbes::Mapper> map;
    return map;
}

QHash<QString, WalkStep>& extraSteps() {
    static QHash<QString, WalkStep> map;
    return map;
}

} // namespace

void WalkProbes::add(const QString& name, Probe probe) {
    probes().insert(name, std::move(probe));
}

bool WalkProbes::has(const QString& name) {
    return probes().contains(name);
}

QVariant WalkProbes::read(const QString& name) {
    auto it = probes().find(name);
    return it == probes().end() ? QVariant() : (*it)();
}

QStringList WalkProbes::names() {
    return probes().keys();
}

void WalkProbes::addMapper(const QString& widget, Mapper mapper) {
    mappers().insert(widget, std::move(mapper));
}

WalkProbes::Mapper WalkProbes::mapper(const QString& widget) {
    return mappers().value(widget);
}

WalkRunner::WalkRunner(QWidget* window, QString script, QString outDir, QObject* parent)
    : QObject(parent), window_(window), script_(std::move(script)), out_(std::move(outDir)) {}

void WalkRunner::addStep(const QString& name, WalkStep step) {
    extraSteps().insert(name, std::move(step));
}

void WalkRunner::start() {
    QFile f(script_);
    if (!f.open(QIODevice::ReadOnly)) {
        check(false, QStringLiteral("script"), QStringLiteral("cannot read %1").arg(script_));
        emit finished(failures_);
        return;
    }
    QJsonParseError error{};
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        check(false, QStringLiteral("script"), error.errorString());
        emit finished(failures_);
        return;
    }
    const QJsonObject root = doc.object();
    steps_ = root.value(QStringLiteral("steps")).toArray();
    delay_ = root.value(QStringLiteral("delay")).toInt(60);
    if (out_.isEmpty()) out_ = QFileInfo(script_).absolutePath();
    QDir().mkpath(out_);
    keepLog_ = root.value(QStringLiteral("log")).toBool(true);
    QFile::remove(logFile());
    log(QStringLiteral("walk %1: %2 steps").arg(QFileInfo(script_).fileName()).arg(steps_.size()));
    QTimer::singleShot(root.value(QStringLiteral("settle")).toInt(400), this, &WalkRunner::next);
}

void WalkRunner::next() {
    if (!window_ || at_ >= steps_.size()) {
        // The walk ran on the owner's desktop without taking the focus from the owner's work.
        if (window_) {
            const bool ours = GetForegroundWindow() == HWND(window_->winId());
            check(!ours, QStringLiteral("the owner's focus untouched"),
                  ours ? QStringLiteral("AvLoop is the foreground window")
                       : QStringLiteral("another program holds the focus"));
        }
        log(failures_ ? QStringLiteral("WALK FAILED (%1)").arg(failures_) : QStringLiteral("WALK PASSED"));
        emit finished(failures_);
        return;
    }
    const QJsonObject step = steps_.at(at_++).toObject();
    const QString label = step.value(QStringLiteral("label")).toString();
    // A label carries the time since the app started (the --mem-log clock), so a memory log can be read by phase.
    const qint64 origin = qApp->property("avloop.clockOrigin").toLongLong();
    if (!label.isEmpty())
        log(origin > 0 ? QStringLiteral("-- %1 @%2 ms").arg(label).arg(QDateTime::currentMSecsSinceEpoch() - origin)
                       : QStringLiteral("-- %1").arg(label));
    const int wait = run(step);
    if (wait < 0) {
        log(QStringLiteral("WALK FAILED (%1) at step %2").arg(failures_).arg(at_));
        emit finished(failures_);
        return;
    }
    if (wait == INT_MAX) return; // the step continues by itself (menu, waitFor)
    QTimer::singleShot(wait, this, &WalkRunner::next);
}

int WalkRunner::run(const QJsonObject& step) {
    const QString what = step.value(QStringLiteral("do")).toString();
    if (what.isEmpty() && step.contains(QStringLiteral("label"))) return 0; // a heading only
    if (what == QLatin1String("wait")) return step.value(QStringLiteral("ms")).toInt(200);
    if (what == QLatin1String("click")) return stepClick(step, false);
    if (what == QLatin1String("dclick")) return stepClick(step, true);
    if (what == QLatin1String("drag")) return stepDrag(step);
    if (what == QLatin1String("wheel")) return stepWheel(step);
    if (what == QLatin1String("key")) return stepKey(step);
    if (what == QLatin1String("type")) return stepType(step);
    if (what == QLatin1String("choose")) return stepChoose(step);
    if (what == QLatin1String("shot")) return stepShot(step);
    if (what == QLatin1String("expect")) return stepExpect(step);
    if (what == QLatin1String("waitFor")) return stepWaitFor(step);
    if (what == QLatin1String("menu")) return stepMenu(step);
    if (what == QLatin1String("theme")) {
        Theme::instance().apply(Theme::idFromName(step.value(QStringLiteral("name")).toString()));
        return delay_ + 200;
    }
    if (what == QLatin1String("resize")) {
        placeQuietly(window_,
                     QSize(step.value(QStringLiteral("w")).toInt(1280), step.value(QStringLiteral("h")).toInt(720)));
        return delay_ + 200;
    }
    if (what == QLatin1String("maximize")) {
        placeQuietly(window_, {});
        return delay_ + 300;
    }
    if (auto it = extraSteps().find(what); it != extraSteps().end()) return (*it)(*this, step);
    check(false, QStringLiteral("step"), QStringLiteral("unknown step \"%1\"").arg(what));
    return -1;
}

QWidget* WalkRunner::target(const QJsonObject& step) const {
    const QString name = step.value(QStringLiteral("on")).toString();
    if (name.isEmpty()) return window_;
    if (window_ && window_->objectName() == name) return window_;
    for (QWidget* top : QApplication::topLevelWidgets()) {
        if (top->objectName() == name && top->isVisible()) return top;
        if (auto* w = top->findChild<QWidget*>(name); w && w->isVisible()) return w;
    }
    return nullptr;
}

void WalkRunner::check(bool ok, const QString& what, const QString& observed) {
    if (!ok) ++failures_;
    log(QStringLiteral("%1 %2: %3").arg(ok ? QStringLiteral("PASS") : QStringLiteral("FAIL"), what, observed));
}

QString WalkRunner::logFile() const {
    return QDir(out_.isEmpty() ? QDir::tempPath() : out_)
        .filePath(QFileInfo(script_).completeBaseName() + QStringLiteral(".walk.txt"));
}

void WalkRunner::log(const QString& line) {
    const QByteArray bytes = (line + QLatin1Char('\n')).toUtf8();
    std::fputs(bytes.constData(), stdout);
    std::fflush(stdout);
    if (!keepLog_) return;
    QFile file(logFile());
    if (file.open(QIODevice::Append)) file.write(bytes);
}

} // namespace avloop
