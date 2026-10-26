// The side panel's Quality section (spec §10.3): Low, Medium (default), High or Custom — CRF 4–63 and Speed 0–8 show
// what the preset uses, and typing either makes it Custom from those values; Custom adds Max bitrate (VP8's bitrate),
// Two-pass and Target size (MB; two passes, CRF unused). Under it, the size estimate (`~1.8 MB`). WebM uses all of
// it; MP4 H.264 only the preset's bitrate (Custom: Max bitrate); ProRes its profile; GIF and APNG none (a note says
// so).
#include "core/Project.h"
#include "ui/WalkRunner.h"
#include "ui/panels/Section.h"
#include "ui/panels/SidePanel.h"
#include "ui/widgets/StepSpinBox.h"

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>

namespace avloop {

void SidePanel::bindQuality(Project* project) {
    Section* s = quality_;
    QWidget* body = s->body();
    auto* hint = new QLabel(tr("Open a video to set its quality."), body);
    hint->setProperty("quiet", true);
    hint->setWordWrap(true);
    s->addWide(hint);

    using P = QualitySettings::Preset;
    auto* preset = new QComboBox(body);
    preset->setObjectName(QStringLiteral("panel.quality.preset"));
    preset->addItem(tr("Low"), QualitySettings::presetId(P::Low));
    preset->addItem(tr("Medium"), QualitySettings::presetId(P::Medium));
    preset->addItem(tr("High"), QualitySettings::presetId(P::High));
    preset->addItem(tr("Custom"), QualitySettings::presetId(P::Custom));
    s->addRow(tr("Preset"), preset);
    auto* formatNote = new QLabel(body);
    formatNote->setObjectName(QStringLiteral("panel.quality.formatNote"));
    formatNote->setProperty("quiet", true);
    formatNote->setWordWrap(true);
    s->addWide(formatNote);
    auto* crf = new StepSpinBox(QStringLiteral("panel.quality.crf"), 4, 63, 1, 0, body);
    crf->setToolTip(tr("Lower is better and larger"));
    s->addRow(tr("CRF"), crf);
    auto* speed = new StepSpinBox(QStringLiteral("panel.quality.speed"), 0, 8, 1, 0, body);
    speed->setToolTip(tr("Lower is slower and a little smaller"));
    s->addRow(tr("Speed"), speed);
    auto* maxRate = new StepSpinBox(QStringLiteral("panel.quality.maxBitrate"), 0, 100000, 500, 0, body);
    maxRate->setSpecialValueText(tr("None"));
    maxRate->setSuffix(tr(" kbit/s"));
    s->addRow(tr("Max bitrate"), maxRate);
    auto* twoPass = new QCheckBox(tr("Two-pass"), body);
    twoPass->setObjectName(QStringLiteral("panel.quality.twoPass"));
    s->addRow(QString(), twoPass);
    auto* target = new StepSpinBox(QStringLiteral("panel.quality.target"), 0, 4000, 0.5, 1, body);
    target->setSpecialValueText(tr("None"));
    target->setSuffix(tr(" MB"));
    target->setToolTip(tr("Two passes at the bitrate this size allows"));
    s->addRow(tr("Target size"), target);
    estimate_ = new QLabel(body);
    estimate_->setObjectName(QStringLiteral("panel.quality.estimate"));
    estimate_->setText(QStringLiteral("—"));
    estimate_->setProperty("mono", true);
    estimate_->setToolTip(
        tr("Measured 1 s after a change to the range, the crop, the loop, the output or the quality"));
    s->addRow(tr("Estimate"), estimate_);

    const QList<QWidget*> customRows = s->row(maxRate) + s->row(twoPass) + s->row(target);
    auto refresh = [=] {
        const QualitySettings& stored = project->quality();
        const QualitySettings q = stored.resolved(project->output().format == OutputFormat::WebmVp8);
        const bool open = project->source().isValid();
        const bool custom = q.preset == P::Custom;
        hint->setVisible(!open);
        for (QWidget* w :
             {static_cast<QWidget*>(preset), static_cast<QWidget*>(crf), static_cast<QWidget*>(speed),
              static_cast<QWidget*>(maxRate), static_cast<QWidget*>(twoPass), static_cast<QWidget*>(target)})
            w->setEnabled(open);
        for (QWidget* w : customRows)
            w->setVisible(custom);
        const QSignalBlocker b1(preset), b2(twoPass);
        preset->setCurrentIndex(std::max(0, preset->findData(QualitySettings::presetId(q.preset))));
        crf->setValueQuietly(q.crf);
        const OutputFormat format = project->output().format;
        const bool webm = format == OutputFormat::WebmVp9 || format == OutputFormat::WebmVp8;
        const bool mp4 = format == OutputFormat::Mp4H264;
        formatNote->setText(mp4 ? tr("H.264: the preset sets the bitrate")
                            : format == OutputFormat::MovProRes
                                ? tr("ProRes: its 4444 profile sets the quality")
                                : tr("%1 has no quality settings").arg(formatName(format, false)));
        formatNote->setVisible(open && !webm);
        preset->setEnabled(open && (webm || mp4));
        maxRate->setEnabled(open && (webm || mp4));
        crf->setEnabled(open && webm && q.targetMB <= 0); // Target size sets the bitrate instead
        speed->setEnabled(open && webm);
        target->setEnabled(open && webm);
        speed->setValueQuietly(q.speed);
        maxRate->setValueQuietly(q.maxBitrate);
        twoPass->setChecked(q.twoPass || q.targetMB > 0);
        twoPass->setEnabled(open && webm && q.targetMB <= 0);
        target->setValueQuietly(q.targetMB);
    };
    connect(project, &Project::changed, this, [refresh](Project::Aspect a) {
        if (a == Project::Aspect::Quality || a == Project::Aspect::Source || a == Project::Aspect::Output) refresh();
    });

    // A field typed on a preset makes it Custom, starting from the preset's values.
    auto edit = [project](const QString& field, auto change) {
        QualitySettings q = project->quality().resolved(project->output().format == OutputFormat::WebmVp8);
        q.preset = P::Custom;
        change(q);
        project->changeQuality(q, field.isEmpty() ? QString() : QStringLiteral("field:quality.") + field);
    };
    connect(preset, &QComboBox::activated, this, [=] {
        const P chosen = QualitySettings::presetFromId(preset->currentData().toString());
        QualitySettings q = project->quality();
        if (chosen == P::Custom && q.preset != P::Custom)
            q = q.resolved(project->output().format == OutputFormat::WebmVp8);
        q.preset = chosen;
        project->changeQuality(q);
    });
    connect(crf, &StepSpinBox::valueChanged, this,
            [=](double v) { edit(QStringLiteral("crf"), [v](QualitySettings& q) { q.crf = int(v); }); });
    connect(speed, &StepSpinBox::valueChanged, this,
            [=](double v) { edit(QStringLiteral("speed"), [v](QualitySettings& q) { q.speed = int(v); }); });
    connect(maxRate, &StepSpinBox::valueChanged, this,
            [=](double v) { edit(QStringLiteral("maxBitrate"), [v](QualitySettings& q) { q.maxBitrate = int(v); }); });
    connect(twoPass, &QCheckBox::clicked, this,
            [=](bool on) { edit(QString(), [on](QualitySettings& q) { q.twoPass = on; }); });
    connect(target, &StepSpinBox::valueChanged, this,
            [=](double v) { edit(QStringLiteral("target"), [v](QualitySettings& q) { q.targetMB = v; }); });

    WalkProbes::add(QStringLiteral("quality.preset"),
                    [project] { return QualitySettings::presetId(project->quality().preset); });
    WalkProbes::add(QStringLiteral("quality.crf"), [project] {
        return project->quality().resolved(project->output().format == OutputFormat::WebmVp8).crf;
    });
    WalkProbes::add(QStringLiteral("quality.estimate"), [this] { return estimate_->text(); });
    refresh();
}

void SidePanel::setEstimate(const QString& text) {
    if (estimate_) estimate_->setText(text);
}

} // namespace avloop
