// The video layers in the main window (D-027): ☰ → Add as layer… (Ctrl+Shift+O) puts a video on the lowest free track
// from the playhead, at its own size at the scene's top-left (R-18), selected; the canvas draws each shown layer's
// frame at the playhead under the crop's dim — decoded when the playhead rests, the last one kept while playing — and
// its outline (the selected one solid, its name); the Preview and the export make the layers exactly (LayerPass).
#include "core/Project.h"
#include "media/Decoder.h"
#include "media/MediaProbe.h"
#include "ui/MainWindow.h"
#include "ui/WalkRunner.h"
#include "ui/canvas/Canvas.h"
#include "ui/panels/SidePanel.h"
#include "ui/theme/Theme.h"
#include "ui/timeline/PlaybackClock.h"
#include "ui/timeline/Timeline.h"
#include "ui/timeline/TrackLanes.h"
#include "ui/widgets/Toast.h"

#include <QAction>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHash>
#include <QPainter>
#include <QVBoxLayout>

#include <cmath>
#include <memory>

namespace avloop {

namespace {

// Each layer's frame at the playhead for the canvas: one software decoder a file (at most VideoLayer::kMaxLayers), the
// last frame a layer showed.
class LayerPreview : public QObject {
public:
    LayerPreview(Project* project, QObject* parent) : QObject(parent), project_(project) {
        connect(project, &Project::changed, this, [this](Project::Aspect a) {
            if (a == Project::Aspect::Layers || a == Project::Aspect::Source) shown_.clear();
            if (a == Project::Aspect::Source) decoders_.clear();
        });
    }

    void paint(QPainter& p, Canvas* canvas, bool playing) {
        const QList<VideoLayer>& layers = project_->layers();
        const ThemeTokens& t = Theme::instance().tokens();
        const double now = project_->time();
        for (int i : videolayers::drawOrder(layers)) {
            const VideoLayer& l = layers[i];
            const QRectF view = canvas->sceneToView(l.rect());
            double own = 0;
            if (l.visible && l.sourceTime(now, &own)) {
                Shown& s = shown_[i];
                if (!playing && (s.image.isNull() || std::abs(s.own - own) > 1e-3)) {
                    s.image = frameOf(l, own);
                    s.own = own;
                }
                if (!s.image.isNull()) {
                    p.save();
                    p.setOpacity(l.opacity / 100.0);
                    p.drawImage(view, s.image.mirrored(l.transform.flipH, l.transform.flipV));
                    p.restore();
                }
            }
            QPen pen(t.accent, i == project_->selectedLayer() ? 1.5 : 1,
                     i == project_->selectedLayer() ? Qt::SolidLine : Qt::DashLine);
            p.setPen(pen);
            p.setBrush(Qt::NoBrush);
            p.drawRect(view);
            if (i == project_->selectedLayer()) p.drawText(view.topLeft() + QPointF(4, -4), l.name);
        }
    }

    int decoders() const { return int(decoders_.size()); }

private:
    struct Shown {
        QImage image;
        double own = -1;
    };

    QImage frameOf(const VideoLayer& l, double own) {
        auto it = decoders_.find(l.path);
        if (it == decoders_.end()) {
            if (decoders_.size() >= VideoLayer::kMaxLayers) decoders_.clear(); // bounded (AVADR-0002)
            const media::MediaProbe::Result probed = media::MediaProbe::probe(l.path);
            std::shared_ptr<media::Decoder> d;
            if (probed.info) {
                media::Decoder::Options options;
                options.hardware = false;
                options.maxLongSide = 960; // the canvas's look; the Preview and the export are exact
                d = std::make_shared<media::Decoder>(*probed.info, options);
                if (!d->open()) d.reset();
            }
            it = decoders_.insert(l.path, d);
        }
        if (!it.value()) return {};
        const media::Frame f = it.value()->frameAt(own + 1e-4);
        if (f.isNull()) return {};
        return QImage(f.data.get(), f.width, f.height, f.stride, QImage::Format_RGBA8888_Premultiplied).copy();
    }

    Project* project_;
    QHash<QString, std::shared_ptr<media::Decoder>> decoders_;
    QHash<int, Shown> shown_;
};

} // namespace

void MainWindow::addLayer(const QString& path) {
    if (!video_) return;
    QList<VideoLayer> layers = project_->layers();
    if (layers.size() >= VideoLayer::kMaxLayers) {
        Toast::post(centralWidget(), tr("At most %1 layers.").arg(VideoLayer::kMaxLayers), Toast::Kind::Warning);
        return;
    }
    const media::MediaProbe::Result probed = media::MediaProbe::probe(path);
    if (!probed.info) {
        Toast::post(centralWidget(), probed.error, Toast::Kind::Warning);
        return;
    }
    VideoLayer l;
    l.name = QFileInfo(path).completeBaseName();
    l.path = QDir::fromNativeSeparators(QFileInfo(path).absoluteFilePath());
    l.size = QSizeF(probed.info->width, probed.info->height);
    l.duration = probed.info->duration;
    l.range = {0, probed.info->duration};
    l.start = project_->time();
    l.transform = VideoTransform::initial(l.size);
    for (int track = 2; track <= 1 + VideoLayer::kMaxLayers; ++track)
        if (videolayers::fits(layers, track, l.start, l.length())) {
            l.track = track;
            break;
        }
    layers.append(l);
    project_->changeLayers(layers);
    project_->setSelectedLayer(int(layers.size() - 1));
}

void MainWindow::buildLayers() {
    side_->bindLayers(project_);
    // The tracks (AV-031): lanes above the main video's timeline, on its time scale.
    if (auto* column = qobject_cast<QVBoxLayout*>(timeline_->parentWidget()->layout()))
        column->insertWidget(
            column->indexOf(timeline_),
            new TrackLanes(project_, timeline_, [this](double t) { seek(t); }, timeline_->parentWidget()));
    auto* preview = new LayerPreview(project_, this);
    canvas_->setUnderlay([this, preview](QPainter& p) { preview->paint(p, canvas_, clock_ && clock_->running()); });
    connect(project_, &Project::changed, canvas_, [this](Project::Aspect a) {
        if (a == Project::Aspect::Layers || a == Project::Aspect::LayerSelection || a == Project::Aspect::Time)
            canvas_->update();
    });
    if (auto* add = findChild<QAction*>(QStringLiteral("action.addLayer")))
        connect(add, &QAction::triggered, this, [this] {
            if (!video_) return;
            const QString path =
                QFileDialog::getOpenFileName(this, tr("Add a video as a layer"), QString(), tr("All media (*.*)"));
            if (!path.isEmpty()) addLayer(path);
        });
    // {"do": "addLayer", "file": "samples/x.mp4"}: Add as layer… with the dialog answered.
    WalkRunner::addStep(QStringLiteral("addLayer"), [this](WalkRunner& walk, const QJsonObject& step) {
        const QString file = step.value(QStringLiteral("file")).toString();
        walk.log(QStringLiteral("     addLayer %1").arg(file));
        addLayer(QDir::isAbsolutePath(file) ? file : QDir::current().absoluteFilePath(file));
        return 400;
    });
    WalkProbes::add(QStringLiteral("layers.previewDecoders"), [preview] { return preview->decoders(); });
}

} // namespace avloop
