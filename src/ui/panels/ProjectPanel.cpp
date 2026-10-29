#include "ui/panels/ProjectPanel.h"

#include "core/Project.h"
#include "core/SequenceEdit.h"
#include "media/MediaProbe.h"

#include <QComboBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

namespace avloop {

namespace {

QString lengthText(double seconds) {
    return QStringLiteral("%1 s").arg(seconds, 0, 'f', 1);
}

} // namespace

ProjectPanel::ProjectPanel(Project* project, QWidget* parent) : QWidget(parent), project_(project) {
    setObjectName(QStringLiteral("ProjectPanel"));
    setAttribute(Qt::WA_StyledBackground);
    auto* column = new QVBoxLayout(this);
    column->setContentsMargins(8, 8, 8, 8);
    column->setSpacing(6);
    auto* title = new QLabel(tr("Project"), this);
    title->setObjectName(QStringLiteral("PanelTitle"));
    column->addWidget(title);
    list_ = new QListWidget(this);
    list_->setObjectName(QStringLiteral("project.media"));
    list_->setToolTip(tr("The media of this project: Import… adds files; Insert or Overwrite puts the selected one "
                         "at the playhead"));
    column->addWidget(list_, 1);
    auto* import = new QPushButton(tr("Import…"), this);
    import->setObjectName(QStringLiteral("project.import"));
    column->addWidget(import);
    auto* targetRow = new QHBoxLayout;
    auto* targetLabel = new QLabel(tr("Track"), this);
    track_ = new QComboBox(this);
    track_->setObjectName(QStringLiteral("project.track"));
    for (int i = 1; i <= Sequence::kMaxTracks; ++i)
        track_->addItem(QStringLiteral("V%1").arg(i));
    track_->setToolTip(tr("The track Insert and Overwrite put the media on"));
    targetRow->addWidget(targetLabel);
    targetRow->addWidget(track_, 1);
    column->addLayout(targetRow);
    auto* placeRow = new QHBoxLayout;
    insert_ = new QPushButton(tr("Insert"), this);
    insert_->setObjectName(QStringLiteral("project.insert"));
    insert_->setToolTip(tr("Insert at the playhead: what is there is cut and the rest of the track moves right"));
    overwrite_ = new QPushButton(tr("Overwrite"), this);
    overwrite_->setObjectName(QStringLiteral("project.overwrite"));
    overwrite_->setToolTip(tr("Overwrite at the playhead: what it covers is replaced"));
    placeRow->addWidget(insert_);
    placeRow->addWidget(overwrite_);
    column->addLayout(placeRow);

    connect(import, &QPushButton::clicked, this, [this] {
        const QStringList paths =
            QFileDialog::getOpenFileNames(this, tr("Import media"), QString(), tr("All media (*.*)"));
        if (!paths.isEmpty()) importFiles(paths);
    });
    connect(insert_, &QPushButton::clicked, this, [this] { place(false); });
    connect(overwrite_, &QPushButton::clicked, this, [this] { place(true); });
    connect(list_, &QListWidget::itemSelectionChanged, this, [this] {
        insert_->setEnabled(list_->currentRow() >= 0);
        overwrite_->setEnabled(list_->currentRow() >= 0);
    });
    connect(project_, &Project::changed, this, [this](Project::Aspect a) {
        if (a == Project::Aspect::Media || a == Project::Aspect::Source) refresh();
    });
    refresh();
}

void ProjectPanel::refresh() {
    const int was = list_->currentRow();
    list_->clear();
    for (const SourceInfo& m : project_->media()) {
        if (m.path.isEmpty()) continue;
        auto* item = new QListWidgetItem(QStringLiteral("%1\n%2 × %3 · %4")
                                             .arg(QFileInfo(m.path).fileName())
                                             .arg(m.width)
                                             .arg(m.height)
                                             .arg(lengthText(m.duration)),
                                         list_);
        item->setData(Qt::UserRole, m.path);
        item->setToolTip(QDir::toNativeSeparators(m.path));
    }
    list_->setCurrentRow(std::min(was, list_->count() - 1));
    insert_->setEnabled(list_->currentRow() >= 0);
    overwrite_->setEnabled(list_->currentRow() >= 0);
}

void ProjectPanel::importFiles(const QStringList& paths) {
    QString last;
    for (const QString& path : paths) {
        const media::MediaProbe::Result probed = media::MediaProbe::probe(path);
        if (!probed.info) {
            emit warn(tr("%1: %2").arg(QFileInfo(path).fileName(), probed.error));
            continue;
        }
        const media::MediaInfo& i = *probed.info;
        SourceInfo s;
        s.path = QDir::fromNativeSeparators(QFileInfo(path).absoluteFilePath());
        s.width = i.width;
        s.height = i.height;
        s.fps = i.avgFps.isValid() ? i.avgFps : i.realFps;
        s.vfr = i.vfr;
        s.duration = i.duration;
        s.frameCount = i.frameCount;
        s.hasAlpha = i.hasAlpha;
        s.stream = i.streamIndex;
        project_->addMedia(s);
        last = s.path;
    }
    for (int row = 0; row < list_->count(); ++row)
        if (list_->item(row)->data(Qt::UserRole).toString() == last) list_->setCurrentRow(row);
}

bool ProjectPanel::place(bool overwrite) {
    const QListWidgetItem* item = list_->currentItem();
    if (!item) return false;
    const QString path = item->data(Qt::UserRole).toString();
    SourceInfo media;
    for (const SourceInfo& m : project_->media())
        if (m.path == path) media = m;
    Sequence next = project_->sequence();
    const int track = track_->currentIndex();
    while (next.tracks.size() <= track)
        next.tracks.append({QStringLiteral("V%1").arg(next.tracks.size() + 1), true, false, {}});
    Clip c;
    c.name = QFileInfo(path).completeBaseName();
    c.path = path;
    c.size = media.size();
    c.duration = media.duration;
    c.range = {0, media.duration};
    c.transform = VideoTransform::initial(c.size);
    c.transform.x = next.frame.center().x(); // centred in the frame at 100 % (Premiere's)
    c.transform.y = next.frame.center().y();
    const double at = project_->time();
    const bool ok = overwrite ? sequenceedit::overwrite(next, track, at, c) : sequenceedit::insert(next, track, at, c);
    if (!ok) {
        emit warn(tr("V%1 is locked or full.").arg(track + 1));
        return false;
    }
    project_->changeSequence(next, overwrite ? tr("Overwrite %1").arg(c.name) : tr("Insert %1").arg(c.name));
    return true;
}

} // namespace avloop
