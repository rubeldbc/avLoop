#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QPixmap>
#include <QPointF>
#include <QPointer>
#include <QStringList>
#include <QVariant>

#include <functional>

class QWidget;

namespace avloop {

// Named readings a walk's `expect` step compares (spec §2.6, control/04 §3). Widgets register what they can tell.
class WalkProbes {
public:
    using Probe = std::function<QVariant()>;
    using Mapper = std::function<QPointF(const QPointF&)>;

    static void add(const QString& name, Probe probe);
    static bool has(const QString& name);
    static QVariant read(const QString& name);
    static QStringList names();
    // Maps a point in a widget's own model space (the canvas's scene px, the timeline's seconds) to widget px.
    static void addMapper(const QString& widget, Mapper mapper);
    static Mapper mapper(const QString& widget);
};

class WalkRunner;
// A step another module adds (`open`, `project` …): returns the delay before the next step, or -1 on failure.
using WalkStep = std::function<int(WalkRunner&, const QJsonObject&)>;

// Replays a walk script on the real widgets (spec §2.8.6): mouse and keys as real events through the widgets'
// own handlers, pictures by QWidget::grab, `expect` against probes. Prints PASS/FAIL lines and WALK PASSED/FAILED.
class WalkRunner : public QObject {
    Q_OBJECT
public:
    WalkRunner(QWidget* window, QString script, QString outDir, QObject* parent = nullptr);

    static void addStep(const QString& name, WalkStep step);
    // A picture of a widget as it looks: QWidget::grab() leaves out what QPainter drew on a QOpenGLWidget, so each
    // visible one is drawn in again from its own framebuffer.
    static QPixmap grabWidget(QWidget* widget);
    // Sizes a window without ever activating it — the owner works on this desktop while walks run: the client size
    // in logical px, or the whole work area when empty (the look of a maximized window).
    static void placeQuietly(QWidget* window, const QSize& client);

    void start();
    int failures() const { return failures_; }
    QWidget* window() const { return window_; }
    QString outDir() const { return out_; }
    // A PASS or FAIL line; a FAIL counts.
    void check(bool ok, const QString& what, const QString& observed);
    void log(const QString& line);
    // The widget a step names by `on` (an objectName), else the window.
    QWidget* target(const QJsonObject& step) const;

signals:
    void finished(int failures);

private:
    void next();
    int run(const QJsonObject& step);
    QString logFile() const;
    // WalkSteps.cpp
    int stepClick(const QJsonObject& step, bool twice);
    int stepDrag(const QJsonObject& step);
    int stepWheel(const QJsonObject& step);
    int stepKey(const QJsonObject& step);
    int stepType(const QJsonObject& step);
    int stepShot(const QJsonObject& step);
    int stepExpect(const QJsonObject& step);
    int stepWaitFor(const QJsonObject& step);
    int stepMenu(const QJsonObject& step);
    void menuNext();
    QPoint point(QWidget* widget, const QJsonObject& step, const QString& key) const;
    void ensureActive();

    QPointer<QWidget> window_;
    QString script_;
    QString out_;
    QJsonArray steps_;
    int at_ = 0;
    int failures_ = 0;
    int delay_ = 60;
    QStringList menuPath_;
    int waitTicks_ = 0;
    bool keepLog_ = true; // the script's "log": a copy of the lines in <out>/<script>.walk.txt
};

} // namespace avloop
