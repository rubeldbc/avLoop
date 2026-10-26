// The side panel's Output section (spec §10.2): its frame rate row (§8.3) — Source (the stream's rate, at most 60;
// a variable rate takes the nearest common one), 15, 23.976, 24, 25, 29.97, 30, 50, 59.94, 60 or Custom (1–60, three
// decimals). Format, Size, Alpha and Background join it at AV-009. A new rate keeps In and Out in seconds on its grid.
#include "core/Project.h"
#include "core/Timecode.h"
#include "ui/WalkRunner.h"
#include "ui/panels/Section.h"
#include "ui/panels/SidePanel.h"
#include "ui/widgets/StepSpinBox.h"

#include <QComboBox>
#include <QLabel>

#include <memory>

namespace avloop {

void SidePanel::bindOutput(Project* project) {
    Section* s = output_;
    QWidget* body = s->body();
    auto* hint = new QLabel(tr("Open a video to set its output."), body);
    hint->setProperty("quiet", true);
    hint->setWordWrap(true);
    s->addWide(hint);
    auto* fps = new QComboBox(body);
    fps->setObjectName(QStringLiteral("panel.output.fps"));
    fps->addItem(tr("Source"), QStringLiteral("source"));
    for (const Rational& r : OutputSettings::commonRates())
        fps->addItem(timecode::rateText(r), QStringLiteral("%1/%2").arg(r.num).arg(r.den));
    fps->addItem(tr("Custom…"), QStringLiteral("custom"));
    s->addRow(tr("FPS"), fps);
    auto* custom = new StepSpinBox(QStringLiteral("panel.output.fpsCustom"), 1, 60, 1, 3, body);
    s->addRow(QString(), custom);

    // Custom… stays chosen while its field is in use, even on a rate the list holds.
    auto customChosen = std::make_shared<bool>(false);
    auto refresh = [=] {
        const OutputSettings& o = project->output();
        const bool open = project->source().isValid();
        hint->setVisible(!open);
        fps->setEnabled(open);
        const QSignalBlocker block(fps);
        // "Source (29.97)": what Source means for this video.
        fps->setItemText(0, open ? tr("Source (%1)").arg(timecode::rateText(project->outputFps())) : tr("Source"));
        int i = fps->findData(o.fpsId());
        const bool isCustom = !o.sourceFps && (i < 0 || *customChosen);
        if (isCustom) i = fps->findData(QStringLiteral("custom"));
        fps->setCurrentIndex(i);
        custom->setVisible(isCustom && open);
        custom->setValueQuietly(o.fps.value());
    };
    connect(project, &Project::changed, this, [refresh](Project::Aspect) { refresh(); });

    connect(fps, &QComboBox::activated, this, [=] {
        const QString id = fps->currentData().toString();
        OutputSettings o = project->output();
        *customChosen = id == QLatin1String("custom");
        if (*customChosen) {
            o.sourceFps = false;
            o.fps = OutputSettings::customRate(project->outputFps().value());
        } else {
            const OutputSettings chosen = OutputSettings::fromFpsId(id);
            o.sourceFps = chosen.sourceFps;
            o.fps = chosen.sourceFps ? o.fps : chosen.fps;
        }
        project->changeOutput(o);
        refresh();
    });
    connect(custom, &StepSpinBox::valueChanged, this, [=](double v) {
        OutputSettings o = project->output();
        o.sourceFps = false;
        o.fps = OutputSettings::customRate(v);
        project->changeOutput(o, QStringLiteral("field:output.fps"));
    });
    WalkProbes::add(QStringLiteral("output.fps"), [project] { return timecode::rateText(project->outputFps()); });
    refresh();
}

} // namespace avloop
