#include "ui/preview/PreviewWindow.h"

#include "core/Timecode.h"
#include "export/FormatProfiles.h"
#include "media/DiskCache.h"
#include "render/RenderService.h"
#include "ui/preview/PreviewPlayer.h"
#include "ui/preview/PreviewWidgets.h"
#include "ui/theme/IconFactory.h"

#include <QAction>
#include <QButtonGroup>
#include <QCloseEvent>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScreen>
#include <QSettings>
#include <QToolButton>
#include <QVBoxLayout>

namespace avloop {

PreviewWindow::PreviewWindow(render::RenderService* service, media::DiskCache* disk, std::int64_t cacheBytes,
                             QWidget* parent)
    : QWidget(parent, Qt::Window), service_(service), disk_(disk) {
    setObjectName(QStringLiteral("PreviewWindow"));
    setWindowTitle(tr("AvLoop — Preview"));
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_StyledBackground);
    player_ = new PreviewPlayer(service, disk, cacheBytes, this);

    auto* column = new QVBoxLayout(this);
    column->setContentsMargins(8, 8, 8, 8);
    column->setSpacing(6);
    auto* top = new QHBoxLayout;
    live_ = new QPushButton(tr("Live"), this);
    live_->setObjectName(QStringLiteral("preview.live"));
    encoded_ = new QPushButton(tr("Encoded"), this);
    encoded_->setObjectName(QStringLiteral("preview.encoded"));
    auto* modes = new QButtonGroup(this);
    for (QPushButton* b : {live_, encoded_}) {
        b->setCheckable(true);
        modes->addButton(b);
        top->addWidget(b);
    }
    live_->setChecked(true);
    top->addSpacing(12);
    top->addWidget(new QLabel(tr("Background"), this));
    background_ = new QComboBox(this);
    background_->setObjectName(QStringLiteral("preview.background"));
    const QList<QPair<QString, int>> backgrounds = {
        {tr("Checker"), int(PreviewView::Background::Checker)}, {tr("Black"), int(PreviewView::Background::Black)},
        {tr("White"), int(PreviewView::Background::White)},     {tr("Green"), int(PreviewView::Background::Green)},
        {tr("Navy"), int(PreviewView::Background::Navy)},       {tr("Image…"), int(PreviewView::Background::Image)}};
    for (const auto& [name, id] : backgrounds)
        background_->addItem(name, id);
    top->addWidget(background_);
    top->addWidget(new QLabel(tr("Zoom"), this));
    zoom_ = new QComboBox(this);
    zoom_->setObjectName(QStringLiteral("preview.zoom"));
    zoom_->addItem(tr("Fit"), 0.0);
    for (double z : {0.5, 1.0, 2.0})
        zoom_->addItem(QStringLiteral("%1%").arg(z * 100), z);
    top->addWidget(zoom_);
    top->addStretch(1);
    status_ = new QLabel(this);
    status_->setObjectName(QStringLiteral("preview.status"));
    status_->setProperty("quiet", true);
    top->addWidget(status_);
    column->addLayout(top);

    view_ = new PreviewView(this);
    column->addWidget(view_, 1);

    auto* bottom = new QHBoxLayout;
    play_ = new QToolButton(this);
    play_->setObjectName(QStringLiteral("preview.play"));
    play_->setAutoRaise(true);
    play_->setToolTip(tr("Play/Pause (Space)"));
    bottom->addWidget(play_);
    slider_ = new PreviewSlider(this);
    bottom->addWidget(slider_, 1);
    time_ = new QLabel(this);
    time_->setObjectName(QStringLiteral("preview.time"));
    time_->setProperty("mono", true);
    bottom->addWidget(time_);
    aroundSeam_ = new QToolButton(this);
    aroundSeam_->setObjectName(QStringLiteral("preview.aroundSeam"));
    aroundSeam_->setText(tr("Around seam"));
    aroundSeam_->setCheckable(true);
    aroundSeam_->setToolTip(tr("Play the last 2 s and the first 2 s, again and again"));
    bottom->addWidget(aroundSeam_);
    column->addLayout(bottom);
    info_ = new QLabel(this);
    info_->setObjectName(QStringLiteral("preview.info"));
    info_->setProperty("quiet", true);
    info_->setProperty("mono", true);
    column->addWidget(info_);

    slider_->setCached([this](int k) { return player_->isCached(k); });
    connect(player_, &PreviewPlayer::frameChanged, this, [this](int k) {
        view_->setFrame(player_->frame());
        slider_->setValue(k);
        refresh();
    });
    connect(player_, &PreviewPlayer::stateChanged, this, &PreviewWindow::refresh);
    connect(slider_, &PreviewSlider::moved, player_, &PreviewPlayer::seek);
    connect(play_, &QToolButton::clicked, player_, &PreviewPlayer::toggle);
    connect(aroundSeam_, &QToolButton::toggled, player_, &PreviewPlayer::setAroundSeam);
    connect(live_, &QPushButton::clicked, this, [this] { player_->setMode(PreviewPlayer::Mode::Live); });
    connect(encoded_, &QPushButton::clicked, this, [this] {
        if (player_->encodeState() != PreviewPlayer::Encode::Running &&
            player_->encodeState() != PreviewPlayer::Encode::Done)
            startEncode();
        player_->setMode(PreviewPlayer::Mode::Encoded);
    });
    connect(background_, &QComboBox::activated, this, [this] {
        const auto b = PreviewView::Background(background_->currentData().toInt());
        QImage image;
        if (b == PreviewView::Background::Image) {
            const QString file = QFileDialog::getOpenFileName(this, tr("An image behind the loop"), {},
                                                              tr("Images (*.png *.jpg *.jpeg *.bmp *.webp)"));
            image.load(file);
        }
        view_->setBackground(b, image);
    });
    connect(zoom_, &QComboBox::activated, this, [this] { view_->setZoom(zoom_->currentData().toDouble()); });
    auto key = [this](QKeySequence seq, auto act) {
        auto* a = new QAction(this);
        a->setShortcut(seq);
        a->setShortcutContext(Qt::WindowShortcut);
        connect(a, &QAction::triggered, this, act);
        addAction(a);
    };
    key(QKeySequence(Qt::Key_Space), [this] { player_->toggle(); });
    key(QKeySequence(Qt::Key_Left), [this] { player_->step(-1); });
    key(QKeySequence(Qt::Key_Right), [this] { player_->step(1); });

    if (!restoreGeometry(QSettings().value(QStringLiteral("preview/geometry")).toByteArray())) resize(720, 560);
    refresh();
}

PreviewWindow::~PreviewWindow() {
    delete player_; // its renderer, its caches and its decoder of the encoded file
    player_ = nullptr;
    service_->trim();
    service_->run([] {}); // what the render thread still held of it is gone, its textures too, the file closed
    if (!tempFile_.isEmpty()) QFile::remove(tempFile_);
}

void PreviewWindow::setJob(const render::RenderJob& job) {
    const bool first = player_->length() == 0;
    player_->setJob(job);
    slider_->setLength(job.plan.length);
    const LoopPlan& p = job.plan;
    const bool fade = job.loop.mode == LoopSettings::Mode::Crossfade;
    const bool pingPong =
        job.loop.mode == LoopSettings::Mode::PingPong || job.loop.mode == LoopSettings::Mode::PingPongSmooth;
    const bool clip = job.loop.mode == LoopSettings::Mode::None;
    slider_->setSeams(fade ? p.segment - 2 * p.fade : -1, pingPong ? QList<int>{0, p.segment - 1} : QList<int>{},
                      !clip);
    // A clip as cut has no seam to watch (D-020): Around seam waits for a loop.
    if (clip && player_->aroundSeam()) player_->setAroundSeam(false);
    aroundSeam_->setEnabled(!clip);
    aroundSeam_->setToolTip(clip ? tr("The clip is not a loop")
                                 : tr("Play the last 2 s and the first 2 s, again and again"));
    if (first && !QSettings().contains(QStringLiteral("preview/geometry"))) {
        // The output's size and the controls, at most 80% of the screen.
        const QSize avail = screen() ? screen()->availableSize() * 0.8 : QSize(1280, 800);
        resize(job.outputSize.boundedTo(avail - QSize(40, 140)).expandedTo(QSize(480, 270)) + QSize(40, 140));
    }
    refresh();
}

void PreviewWindow::startEncode() {
    const QString dir = disk_ ? disk_->sessionDir() : QDir::tempPath();
    if (!tempFile_.isEmpty()) QFile::remove(tempFile_);
    tempFile_ = QDir(dir).filePath(QStringLiteral("preview-%1.%2")
                                       .arg(++encodes_)
                                       .arg(exporting::profileOf(player_->job().output.format).extension));
    player_->encode(tempFile_);
}

QString PreviewWindow::info() const {
    const render::RenderJob& j = player_->job();
    if (j.plan.length <= 0) return {};
    QString text = QStringLiteral("%1×%2 · %3 fps · %4 frames · %5 s · %6")
                       .arg(j.outputSize.width())
                       .arg(j.outputSize.height())
                       .arg(timecode::rateText(j.fps))
                       .arg(j.plan.length)
                       .arg(j.plan.length / j.fps.value(), 0, 'f', 2)
                       .arg(formatName(j.output.format, j.alpha));
    if (player_->encodeState() == PreviewPlayer::Encode::Done)
        text += QStringLiteral(" · %1 MB").arg(player_->encodedBytes() / 1048576.0, 0, 'f', 2);
    return text;
}

QString PreviewWindow::status() const {
    using E = PreviewPlayer::Encode;
    if (player_->mode() == PreviewPlayer::Mode::Encoded) {
        switch (player_->encodeState()) {
        case E::Running:
            return tr("Encoding %1/%2").arg(player_->encodeDone()).arg(player_->length());
        case E::OutOfDate:
            return tr("Out of date");
        case E::Failed:
            return tr("Encoding failed: %1").arg(player_->error());
        case E::Done:
            return tr("Encoded · %1 MB").arg(player_->encodedBytes() / 1048576.0, 0, 'f', 2);
        case E::None:
            break;
        }
    }
    if (player_->caching()) return tr("Caching…");
    return {};
}

void PreviewWindow::refresh() {
    play_->setIcon(IconFactory::icon(player_->playing() ? QStringLiteral("pause") : QStringLiteral("play")));
    const render::RenderJob& j = player_->job();
    const double fps = j.fps.isValid() ? j.fps.value() : 30.0;
    time_->setText(
        QStringLiteral("%1 / %2 s").arg(player_->position() / fps, 0, 'f', 2).arg(player_->length() / fps, 0, 'f', 2));
    status_->setText(status());
    info_->setText(info());
    live_->setChecked(player_->mode() == PreviewPlayer::Mode::Live);
    encoded_->setChecked(player_->mode() == PreviewPlayer::Mode::Encoded);
    {
        const QSignalBlocker block(aroundSeam_);
        aroundSeam_->setChecked(player_->aroundSeam());
    }
    slider_->update();
}

void PreviewWindow::closeEvent(QCloseEvent* event) {
    QSettings().setValue(QStringLiteral("preview/geometry"), saveGeometry());
    QWidget::closeEvent(event);
}

} // namespace avloop
