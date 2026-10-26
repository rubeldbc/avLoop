#include "export/ExportQueue.h"

#include "core/TimeRange.h"
#include "export/ExportJob.h"
#include "media/MediaProbe.h"
#include "render/RenderJob.h"

#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>
#include <QTimer>
#include <QUuid>

namespace avloop::exporting {

namespace {

QString tr(const char* text) {
    return QCoreApplication::translate("ExportQueue", text);
}

ExportQueue::State stateFromId(const QString& id) {
    using S = ExportQueue::State;
    if (id == QLatin1String("done")) return S::Done;
    if (id == QLatin1String("failed")) return S::Failed;
    return S::Pending; // "running" too: an entry under way when the app closed starts over
}

} // namespace

QString ExportQueue::stateId(State s) {
    switch (s) {
    case State::Running:
        return QStringLiteral("running");
    case State::Done:
        return QStringLiteral("done");
    case State::Failed:
        return QStringLiteral("failed");
    case State::Pending:
        break;
    }
    return QStringLiteral("pending");
}

ExportQueue::ExportQueue(const QString& file, JobMaker makeJob, QObject* parent)
    : QObject(parent), file_(file), makeJob_(std::move(makeJob)) {
    if (!file_.isEmpty()) {
        lock_ = std::make_unique<QLockFile>(file_ + QStringLiteral(".lock"));
        if (lock_->tryLock(0)) load();
    }
}

ExportQueue::~ExportQueue() {
    if (job_) job_->cancel();
    job_.reset();
}

int ExportQueue::indexOf(const QString& id) const {
    for (int i = 0; i < entries_.size(); ++i)
        if (entries_[i].id == id) return i;
    return -1;
}

void ExportQueue::add(const ProjectData& snapshot, QSize outputSize, const QString& output) {
    Entry e;
    e.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    e.project = projectjson::toJson(snapshot);
    e.output = output;
    e.format = formatId(snapshot.output.format);
    e.size = outputSize;
    entries_ << e;
    save();
    emit changed();
}

void ExportQueue::remove(const QString& id) {
    const int i = indexOf(id);
    if (i < 0) return;
    if (id == running_) {
        running_.clear(); // its end is not this entry's any more
        job_->cancel();
    }
    entries_.removeAt(i);
    save();
    emit changed();
}

void ExportQueue::reorder(const QStringList& ids) {
    QList<Entry> sorted;
    for (const QString& id : ids)
        if (const int i = indexOf(id); i >= 0) sorted << entries_.takeAt(i);
    entries_ = sorted + entries_;
    save();
    emit changed();
}

void ExportQueue::clearDone() {
    entries_.removeIf([](const Entry& e) { return e.state == State::Done; });
    save();
    emit changed();
}

void ExportQueue::start() {
    if (started_) return;
    started_ = true;
    emit startedChanged();
    if (!job_) next();
}

void ExportQueue::pause() {
    if (!started_) return;
    started_ = false;
    emit startedChanged();
    if (job_) job_->cancel(); // its finish puts the entry back to Pending
}

void ExportQueue::next() {
    if (job_ || !started_) return;
    int i = 0;
    while (i < entries_.size() && entries_[i].state != State::Pending)
        ++i;
    if (i == entries_.size() || (mayStart && !mayStart())) {
        started_ = false; // all done, or the main window's export runs: Start again later
        emit startedChanged();
        return;
    }
    Entry& e = entries_[i];
    auto fail = [&](const QString& why) {
        e.state = State::Failed;
        e.reason = why;
        save();
        emit changed();
        QTimer::singleShot(0, this, &ExportQueue::next); // the next one
    };
    ProjectData data;
    QString error;
    if (!projectjson::fromJson(e.project, data, &error)) return fail(error);
    const media::MediaProbe::Result probed = media::MediaProbe::probe(data.source.path);
    if (!probed.info) return fail(tr("The video cannot be opened: %1").arg(probed.error));
    const media::MediaInfo& info = *probed.info;
    data.range = timerange::valid(data.range, info.duration, data.output.effectiveFps(info.avgFps, info.vfr));
    const render::RenderJob job = render::RenderJob::of(data, info);
    if (job.plan.length < 2) return fail(tr("The loop is shorter than 2 frames"));

    e.state = State::Running;
    e.progress = 0;
    e.reason.clear();
    running_ = e.id;
    job_ = makeJob_(job, e.output);
    connect(job_.get(), &ExportJob::progressed, this, [this] {
        const int at = indexOf(running_);
        if (at < 0 || !job_) return;
        entries_[at].progress = double(job_->done()) / std::max(1, job_->total());
        emit progressed(at);
    });
    // The job is let go after its signal has returned.
    connect(job_.get(), &ExportJob::finished, this, [this] { QTimer::singleShot(0, this, &ExportQueue::finishEntry); });
    save();
    emit changed();
    job_->start();
}

void ExportQueue::finishEntry() {
    if (!job_) return;
    const std::unique_ptr<ExportJob> job = std::move(job_);
    const int i = indexOf(running_); // -1: removed while it ran
    running_.clear();
    if (i >= 0) {
        Entry& e = entries_[i];
        const ExportJob::Result& r = job->result();
        switch (job->state()) {
        case ExportJob::State::Done:
            e.state = r.verifyProblem.isEmpty() ? State::Done : State::Failed;
            e.reason = r.verifyProblem.isEmpty() ? QString() : tr("The check found: %1").arg(r.verifyProblem);
            e.output = r.path; // `name (1)` when the file was held open
            e.bytes = r.bytes;
            e.progress = 1;
            break;
        case ExportJob::State::Failed:
            e.state = State::Failed;
            e.reason = r.error;
            break;
        default: // cancelled by Pause: it starts over
            e.state = State::Pending;
            e.progress = 0;
            break;
        }
        save();
    }
    emit changed();
    next();
}

void ExportQueue::load() {
    QFile f(file_);
    if (!f.open(QIODevice::ReadOnly)) return;
    const QJsonObject root = QJsonDocument::fromJson(f.readAll()).object();
    if (root.value(QStringLiteral("format")).toString() != QLatin1String("avloop-queue")) return;
    for (const QJsonValue& v : root.value(QStringLiteral("entries")).toArray()) {
        const QJsonObject o = v.toObject();
        Entry e;
        e.id = o.value(QStringLiteral("id")).toString();
        e.project = o.value(QStringLiteral("project")).toObject();
        e.output = o.value(QStringLiteral("output")).toString();
        e.state = stateFromId(o.value(QStringLiteral("state")).toString());
        e.reason = o.value(QStringLiteral("reason")).toString();
        e.format = o.value(QStringLiteral("format")).toString();
        const QJsonArray size = o.value(QStringLiteral("size")).toArray();
        e.size = QSize(size.at(0).toInt(), size.at(1).toInt());
        e.bytes = o.value(QStringLiteral("bytes")).toInteger();
        e.progress = e.state == State::Done ? 1 : 0;
        if (!e.id.isEmpty() && !e.output.isEmpty()) entries_ << e;
    }
}

void ExportQueue::save() const {
    if (!isSaved()) return;
    QJsonArray list;
    for (const Entry& e : entries_)
        list.append(QJsonObject{{QStringLiteral("id"), e.id},
                                {QStringLiteral("project"), e.project},
                                {QStringLiteral("output"), e.output},
                                {QStringLiteral("state"), stateId(e.state)},
                                {QStringLiteral("reason"), e.reason},
                                {QStringLiteral("format"), e.format},
                                {QStringLiteral("size"), QJsonArray{e.size.width(), e.size.height()}},
                                {QStringLiteral("bytes"), qint64(e.bytes)}});
    const QJsonObject root{{QStringLiteral("format"), QStringLiteral("avloop-queue")},
                           {QStringLiteral("version"), 1},
                           {QStringLiteral("entries"), list}};
    QSaveFile f(file_);
    if (!f.open(QIODevice::WriteOnly)) return;
    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    f.commit();
}

} // namespace avloop::exporting
