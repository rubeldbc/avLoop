// The Program monitor and the Project panel (D-033). While the project is AvLoop's one video the canvas draws it as
// AvLoop did — the GL video, the layers' underlay. A sequence beyond that (cuts on V1, a clip from later in its media,
// masks) is drawn as the composite at the playhead: the SequenceRenderer over every clip present and the area they
// cover with the frame, their masks applied and the matte left to the canvas's dim, made when the playhead or the
// sequence changes (the last one kept between). The Project panel sits left of the canvas, its splitter remembered.
#include "core/Project.h"
#include "core/SequenceView.h"
#include "render/SequenceRenderer.h"
#include "ui/MainWindow.h"
#include "ui/MonitorComposite.h"
#include "ui/WalkRunner.h"
#include "ui/canvas/Canvas.h"
#include "ui/panels/ProjectPanel.h"
#include "ui/widgets/Toast.h"

#include <QDir>
#include <QPainter>
#include <QTimer>

#include <cmath>
#include <memory>

namespace avloop {

MonitorComposite::MonitorComposite(Project* project, Canvas* canvas, QObject* parent)
    : QObject(parent), project_(project), canvas_(canvas) {
    connect(project_, &Project::changed, this, [this](Project::Aspect a) {
        if (a != Project::Aspect::Sequence && a != Project::Aspect::Source) return;
        made_ = -1;
        renderer_.reset(); // the clips changed: their decoders go with them
        canvas_->setVideoHidden(active());
        canvas_->update();
    });
}

MonitorComposite::~MonitorComposite() = default;

bool MonitorComposite::active() const {
    return project_->source().isValid() && !project_->avloopShaped();
}

void MonitorComposite::paint(QPainter& p) {
    if (project_->time() != made_ && !pending_) {
        pending_ = true;
        QTimer::singleShot(0, this, [this] {
            pending_ = false;
            make();
            canvas_->update();
        });
    }
    if (!image_.isNull()) p.drawImage(rect_, image_);
}

void MonitorComposite::make() {
    const double t = project_->time();
    made_ = t;
    if (!active()) return;
    Sequence s = project_->sequence();
    // The frame and every clip present: what the canvas shows; the matte left to its dim.
    QRectF area = s.frame;
    for (const Track& track : s.tracks)
        for (const Clip& c : track.clips) {
            double own = 0;
            if (c.ownTime(t, &own)) area = area.united(c.rectAt(own));
        }
    if (area.isEmpty()) return;
    const double scale = std::min(1.0, 1920.0 / std::max(area.width(), area.height()));
    const QSize size(std::max(16, int(std::lround(area.width() * scale))),
                     std::max(16, int(std::lround(area.height() * scale))));
    if (!renderer_ || area != rect_ || size != image_.size()) {
        s.matte = CropShape{};
        renderer_ = std::make_unique<render::SequenceRenderer>(s, area, size, LoopSettings::Method::Nearest, true,
                                                               Qt::transparent);
    }
    rect_ = area;
    renderer_->render({{t, 1, 0, 0}}, image_);
}

void MainWindow::buildMonitor() {
    monitor_ = new MonitorComposite(project_, canvas_, this);
    connect(projectPanel_, &ProjectPanel::warn, this,
            [this](const QString& text) { Toast::post(centralWidget(), text, Toast::Kind::Warning); });
    // {"do": "import", "files": ["samples/x.mp4", …]}: Import… with the dialog answered.
    WalkRunner::addStep(QStringLiteral("import"), [this](WalkRunner& walk, const QJsonObject& step) {
        QStringList paths;
        for (const QJsonValue& v : step.value(QStringLiteral("files")).toArray()) {
            const QString file = v.toString();
            paths << (QDir::isAbsolutePath(file) ? file : QDir::current().absoluteFilePath(file));
        }
        walk.log(QStringLiteral("     import %1").arg(paths.size()));
        projectPanel_->importFiles(paths);
        return 300;
    });
    WalkProbes::add(QStringLiteral("project.media"), [this] { return int(project_->media().size()); });
    WalkProbes::add(QStringLiteral("sequence.clips"), [this] { return project_->sequence().clipCount(); });
    WalkProbes::add(QStringLiteral("sequence.v1"), [this] {
        QStringList clips;
        for (const Clip& c : project_->sequence().tracks.value(0).clips)
            clips << QStringLiteral("%1@%2+%3").arg(c.name).arg(c.start, 0, 'f', 2).arg(c.range.in, 0, 'f', 2);
        return clips.join(QLatin1Char(' '));
    });
    WalkProbes::add(QStringLiteral("sequence.tracks"), [this] { return int(project_->sequence().tracks.size()); });
    WalkProbes::add(QStringLiteral("monitor.composite"), [this] { return monitor_->active(); });
}

} // namespace avloop
