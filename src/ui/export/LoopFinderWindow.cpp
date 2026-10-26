#include "ui/export/LoopFinderWindow.h"

#include "core/Project.h"
#include "core/Timecode.h"
#include "ui/widgets/StepSpinBox.h"

#include <QComboBox>
#include <QFutureWatcher>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QListWidget>
#include <QPixmap>
#include <QPointer>
#include <QProgressBar>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>
#include <QtConcurrent>

#include <cstring>

namespace avloop {

namespace {

QLabel* quiet(const QString& text, QWidget* parent) {
    auto* l = new QLabel(text, parent);
    l->setProperty("quiet", true);
    return l;
}

} // namespace

LoopFinderWindow::LoopFinderWindow(Project* project, const media::MediaInfo& source, QWidget* parent)
    : QWidget(parent, Qt::Window), project_(project), source_(source) {
    setObjectName(QStringLiteral("LoopFinderWindow"));
    setWindowTitle(tr("AvLoop — Loop Finder"));
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_StyledBackground);
    auto* column = new QVBoxLayout(this);
    column->setContentsMargins(10, 10, 10, 10);
    column->setSpacing(8);

    auto* inputs = new QHBoxLayout;
    inputs->addWidget(quiet(tr("Search"), this));
    range_ = new QComboBox(this);
    range_->setObjectName(QStringLiteral("loopfinder.range"));
    range_->addItem(tr("In–Out ± 5 s"), QStringLiteral("around"));
    range_->addItem(tr("The whole file"), QStringLiteral("file"));
    inputs->addWidget(range_);
    inputs->addSpacing(8);
    inputs->addWidget(quiet(tr("Length"), this));
    min_ = new StepSpinBox(QStringLiteral("loopfinder.min"), 0.5, 60, 0.5, 2, this);
    max_ = new StepSpinBox(QStringLiteral("loopfinder.max"), 0.5, 60, 0.5, 2, this);
    for (StepSpinBox* s : {min_, max_})
        s->setSuffix(QStringLiteral(" s"));
    min_->setValueQuietly(2);
    max_->setValueQuietly(10);
    inputs->addWidget(min_);
    inputs->addWidget(quiet(QStringLiteral("–"), this));
    inputs->addWidget(max_);
    inputs->addStretch(1);
    find_ = new QPushButton(tr("Find"), this);
    find_->setObjectName(QStringLiteral("loopfinder.find"));
    find_->setProperty("primary", true);
    inputs->addWidget(find_);
    column->addLayout(inputs);

    auto* running = new QHBoxLayout;
    progress_ = new QProgressBar(this);
    progress_->setObjectName(QStringLiteral("loopfinder.progress"));
    progress_->setRange(0, 1000);
    progress_->setTextVisible(false);
    progress_->setFixedHeight(6);
    cancel_ = new QPushButton(tr("Cancel"), this);
    cancel_->setObjectName(QStringLiteral("loopfinder.cancel"));
    running->addWidget(progress_, 1);
    running->addWidget(cancel_);
    column->addLayout(running);
    status_ = quiet(tr("Find looks for the In and Out where the video joins itself best."), this);
    status_->setObjectName(QStringLiteral("loopfinder.status"));
    status_->setWordWrap(true);
    column->addWidget(status_);

    list_ = new QListWidget(this);
    list_->setObjectName(QStringLiteral("loopfinder.list"));
    list_->setIconSize(QSize(120, 68));
    list_->setSpacing(2);
    column->addWidget(list_, 1);
    column->addWidget(quiet(tr("A click sets In and Out; a double-click also opens the Preview."), this));

    connect(find_, &QPushButton::clicked, this, &LoopFinderWindow::find);
    connect(cancel_, &QPushButton::clicked, this, [this] {
        if (cancelFlag_) *cancelFlag_ = true;
    });
    connect(list_, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) { choose(list_->row(item), false); });
    connect(list_, &QListWidget::itemDoubleClicked, this,
            [this](QListWidgetItem* item) { choose(list_->row(item), true); });
    cancel_->setVisible(false);
    progress_->setVisible(false);
    if (!restoreGeometry(QSettings().value(QStringLiteral("finder/geometry")).toByteArray())) resize(560, 520);
}

LoopFinderWindow::~LoopFinderWindow() {
    if (cancelFlag_) *cancelFlag_ = true; // a search under way stops; its fingerprints go with it
    QSettings().setValue(QStringLiteral("finder/geometry"), saveGeometry());
}

void LoopFinderWindow::find() {
    if (state_ == QLatin1String("running")) return;
    exporting::LoopFinder::Input in;
    in.source = source_;
    in.frame = project_->exportFrame();
    in.transform = project_->baseTransform();
    in.keyframes = project_->keyframes();
    in.fps = project_->outputFps();
    const double end = timerange::fileEnd(source_.duration, in.fps);
    const bool whole = range_->currentData().toString() == QLatin1String("file");
    in.from = whole ? 0.0 : std::max(0.0, project_->range().in - 5);
    in.to = whole ? end : std::min(end, project_->range().out + 5);
    in.minLength = std::min(min_->value(), max_->value());
    in.maxLength = std::max(min_->value(), max_->value());

    cancelFlag_ = std::make_shared<std::atomic<bool>>(false);
    auto cancel = cancelFlag_;
    QPointer<LoopFinderWindow> self(this);
    auto progress = [self](int done, int total) {
        QMetaObject::invokeMethod(qApp, [self, done, total] {
            if (self) self->progress_->setValue(done * 1000 / std::max(1, total));
        });
    };
    auto* watcher = new QFutureWatcher<QPair<QList<exporting::LoopFinder::Candidate>, QString>>(this);
    connect(watcher, &QFutureWatcherBase::finished, this, [this, watcher] {
        const auto [found, error] = watcher->result();
        watcher->deleteLater();
        results_ = found;
        const bool cancelled = cancelFlag_ && *cancelFlag_;
        state_ = cancelled         ? QStringLiteral("idle")
                 : found.isEmpty() ? QStringLiteral("failed")
                                   : QStringLiteral("done");
        find_->setEnabled(true);
        cancel_->setVisible(false);
        progress_->setVisible(false);
        status_->setText(cancelled ? tr("Cancelled.")
                         : found.isEmpty()
                             ? error
                             : tr("The best %n loop(s): a click sets In and Out.", "", int(found.size())));
        showResults();
    });
    watcher->setFuture(QtConcurrent::run([in, cancel, progress] {
        QString error;
        QList<exporting::LoopFinder::Candidate> found = exporting::LoopFinder::find(in, *cancel, progress, &error);
        return qMakePair(found, error);
    }));
    state_ = QStringLiteral("running");
    results_.clear();
    list_->clear();
    find_->setEnabled(false);
    cancel_->setVisible(true);
    progress_->setValue(0);
    progress_->setVisible(true);
    status_->setText(tr("Looking at %1 s of video…").arg(in.to - in.from, 0, 'f', 1));
}

void LoopFinderWindow::showResults() {
    list_->clear();
    const Rational fps = project_->outputFps();
    for (const exporting::LoopFinder::Candidate& c : results_) {
        // Both ends side by side: the first frame and the last, which the loop joins.
        QImage pair(c.first.width() * 2 + 6, c.first.height(), QImage::Format_RGBA8888_Premultiplied);
        pair.fill(Qt::transparent);
        for (int y = 0; y < c.first.height(); ++y) {
            std::memcpy(pair.scanLine(y), c.first.constScanLine(y), std::size_t(c.first.width()) * 4);
            std::memcpy(pair.scanLine(y) + std::size_t(c.first.width() + 6) * 4, c.last.constScanLine(y),
                        std::size_t(c.last.width()) * 4);
        }
        auto* item = new QListWidgetItem(QIcon(QPixmap::fromImage(pair)),
                                         tr("In %1 · Out %2 · %3 s · score %4")
                                             .arg(timecode::format(c.in, fps), timecode::format(c.out, fps))
                                             .arg(c.out - c.in, 0, 'f', 2)
                                             .arg(c.score),
                                         list_);
        item->setSizeHint(QSize(0, c.first.height() + 8));
    }
    list_->setIconSize(results_.isEmpty()
                           ? QSize(120, 68)
                           : QSize(results_.front().first.width() * 2 + 6, results_.front().first.height()));
}

void LoopFinderWindow::choose(int row, bool preview) {
    if (row < 0 || row >= results_.size()) return;
    const exporting::LoopFinder::Candidate& c = results_[row];
    project_->changeRange(TimeRange{c.in, c.out}); // one Undo step
    list_->setCurrentRow(row);
    if (preview) emit previewRequested();
}

} // namespace avloop
