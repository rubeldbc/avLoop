#include "ui/export/ExportBar.h"

#include "export/ExportJob.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QToolButton>

#include <algorithm>
#include <cmath>

namespace avloop {

ExportBar::ExportBar(QWidget* parent) : QWidget(parent) {
    setObjectName(QStringLiteral("status.export"));
    auto* row = new QHBoxLayout(this);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(8);
    label_ = new QLabel(this);
    label_->setObjectName(QStringLiteral("status.exportText"));
    label_->setProperty("mono", true);
    bar_ = new QProgressBar(this);
    bar_->setObjectName(QStringLiteral("status.exportBar"));
    bar_->setTextVisible(false);
    bar_->setFixedSize(90, 6);
    cancel_ = new QToolButton(this); // flat: the status bar is 22 px high
    cancel_->setObjectName(QStringLiteral("status.exportCancel"));
    cancel_->setText(tr("Cancel"));
    cancel_->setAutoRaise(true);
    row->addWidget(label_);
    row->addWidget(bar_);
    row->addWidget(cancel_);
    connect(cancel_, &QToolButton::clicked, this, &ExportBar::cancelRequested);
    hide();
}

void ExportBar::track(exporting::ExportJob* job) {
    if (job_) disconnect(job_, nullptr, this, nullptr);
    job_ = job;
    if (job_) {
        connect(job_, &exporting::ExportJob::progressed, this, &ExportBar::refresh);
        connect(job_, &exporting::ExportJob::finished, this, [this] { track(nullptr); });
        connect(job_, &QObject::destroyed, this, [this] {
            job_ = nullptr;
            hide();
        });
    }
    setVisible(job_ != nullptr);
    refresh();
}

QString ExportBar::text() const {
    return isVisible() || job_ ? label_->text() : QString();
}

QString ExportBar::progressText(int done, int total, int pass, int passes, double fps, double eta) {
    if (done <= 0) return tr("Exporting… preparing");
    const int perPass = std::max(1, total / std::max(1, passes));
    const int inPass = std::clamp(done - (pass - 1) * perPass, 0, perPass);
    QString text = tr("Exporting %1/%2").arg(inPass).arg(perPass);
    if (passes > 1) text += tr(" · pass %1/%2").arg(pass).arg(passes);
    text += QStringLiteral(" · %1%").arg(int(std::floor(100.0 * done / std::max(1, total))));
    if (fps > 0) text += tr(" · %1 fps").arg(int(std::lround(fps)));
    if (eta >= 0) {
        const int s = int(std::ceil(eta));
        text += tr(" · ETA %1:%2").arg(s / 60).arg(s % 60, 2, 10, QLatin1Char('0'));
    }
    return text;
}

void ExportBar::refresh() {
    if (!job_) return;
    label_->setText(progressText(job_->done(), job_->total(), job_->pass(), job_->passes(), job_->fps(),
                                 job_->done() > 0 ? job_->etaSeconds() : -1));
    bar_->setRange(0, std::max(1, job_->total()));
    bar_->setValue(job_->done());
}

} // namespace avloop
