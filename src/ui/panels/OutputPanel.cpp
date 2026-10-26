// The side panel's Output section (spec §10.2): Format (WebM VP9 or VP8; the others at AV-013), Size (Crop size, Long
// side 1920 … 256, Custom W×H with its ratio lock), Alpha (Auto — only when there are transparent parts —, On, Off),
// Background (behind the transparent parts when there is no alpha) and the frame rate (§8.3: Source — the stream's
// rate, at most 60 —, the common rates or Custom 1–60). Sizes come out even, ≥ 16, the long side ≤ 1920; a note says
// when that limit took the size down. A new rate keeps In and Out in seconds on its grid.
#include "core/Geometry.h"
#include "core/Project.h"
#include "core/Timecode.h"
#include "export/FormatProfiles.h"
#include "ui/WalkRunner.h"
#include "ui/panels/Section.h"
#include "ui/panels/SidePanel.h"
#include "ui/theme/IconFactory.h"
#include "ui/widgets/StepSpinBox.h"

#include <QColorDialog>
#include <QComboBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QPixmap>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QToolButton>

#include <cmath>
#include <memory>

namespace avloop {

namespace {

QWidget* rowOf(QWidget* parent, std::initializer_list<QWidget*> parts) {
    auto* w = new QWidget(parent);
    auto* row = new QHBoxLayout(w);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(4);
    for (QWidget* p : parts) {
        p->setParent(w);
        row->addWidget(p, qobject_cast<QToolButton*>(p) || qobject_cast<QLabel*>(p) ? 0 : 1);
    }
    return w;
}

// The other side of a locked W×H: the export frame's ratio, even.
int locked(double side, double ratio) {
    return std::max(16, int(std::lround(side * ratio / 2.0)) * 2);
}

} // namespace

void SidePanel::bindOutput(Project* project) {
    Section* s = output_;
    QWidget* body = s->body();
    auto* hint = new QLabel(tr("Open a video to set its output."), body);
    hint->setProperty("quiet", true);
    hint->setWordWrap(true);
    s->addWide(hint);

    auto* format = new QComboBox(body);
    format->setObjectName(QStringLiteral("panel.output.format"));
    for (const exporting::FormatProfile& p : exporting::formatProfiles())
        format->addItem(p.name, formatId(p.format));
    s->addRow(tr("Format"), format);

    auto* size = new QComboBox(body);
    size->setObjectName(QStringLiteral("panel.output.size"));
    size->addItem(tr("Crop size"), QStringLiteral("crop"));
    for (int n : OutputSettings::longSides())
        size->addItem(tr("Long side %1").arg(n), QStringLiteral("long-%1").arg(n));
    size->addItem(tr("Custom W×H"), QStringLiteral("custom"));
    s->addRow(tr("Size"), size);
    auto* customW = new StepSpinBox(QStringLiteral("panel.output.w"), 16, 9999, 2, 0, body);
    auto* customH = new StepSpinBox(QStringLiteral("panel.output.h"), 16, 9999, 2, 0, body);
    for (StepSpinBox* f : {customW, customH})
        f->setMinimumWidth(44);
    auto* by = new QLabel(QStringLiteral("×"), body);
    by->setProperty("quiet", true);
    auto* lock = new QToolButton(body);
    lock->setObjectName(QStringLiteral("panel.output.lock"));
    lock->setCheckable(true);
    lock->setAutoRaise(true);
    lock->setToolTip(tr("Keep the crop's ratio"));
    QWidget* custom = rowOf(body, {customW, by, customH, lock});
    s->addRow(QString(), custom);
    auto* note = new QLabel(body);
    note->setObjectName(QStringLiteral("panel.output.note"));
    note->setProperty("quiet", true);
    note->setWordWrap(true);
    s->addWide(note);

    auto* alpha = new QComboBox(body);
    alpha->setObjectName(QStringLiteral("panel.output.alpha"));
    alpha->addItem(tr("Auto"), QStringLiteral("auto"));
    alpha->addItem(tr("On"), QStringLiteral("on"));
    alpha->addItem(tr("Off"), QStringLiteral("off"));
    alpha->setToolTip(tr("Auto keeps alpha only when the output has transparent parts"));
    s->addRow(tr("Alpha"), alpha);
    auto* colour = new QLineEdit(body);
    colour->setObjectName(QStringLiteral("panel.output.background"));
    colour->setValidator(
        new QRegularExpressionValidator(QRegularExpression(QStringLiteral("#[0-9A-Fa-f]{6}")), colour));
    colour->setProperty("mono", true);
    auto* pick = new QToolButton(body);
    pick->setObjectName(QStringLiteral("panel.output.backgroundPick"));
    pick->setToolTip(tr("Choose the colour behind the transparent parts"));
    QWidget* background = rowOf(body, {colour, pick});
    s->addRow(tr("Background"), background);

    auto* fps = new QComboBox(body);
    fps->setObjectName(QStringLiteral("panel.output.fps"));
    fps->addItem(tr("Source"), QStringLiteral("source"));
    for (const Rational& r : OutputSettings::commonRates())
        fps->addItem(timecode::rateText(r), QStringLiteral("%1/%2").arg(r.num).arg(r.den));
    fps->addItem(tr("Custom…"), QStringLiteral("custom"));
    s->addRow(tr("FPS"), fps);
    auto* fpsCustom = new StepSpinBox(QStringLiteral("panel.output.fpsCustom"), 1, 60, 1, 3, body);
    s->addRow(QString(), fpsCustom);

    const QList<QWidget*> customRow = s->row(custom);
    // Custom… stays chosen while its field is in use, even on a rate the list holds.
    auto customChosen = std::make_shared<bool>(false);
    auto refresh = [=] {
        const OutputSettings& o = project->output();
        const bool open = project->source().isValid();
        hint->setVisible(!open);
        for (QWidget* w : {static_cast<QWidget*>(format), static_cast<QWidget*>(size), custom,
                           static_cast<QWidget*>(alpha), background, static_cast<QWidget*>(fps)})
            w->setEnabled(open);
        const QSignalBlocker b1(format), b2(size), b3(alpha), b4(fps), b5(lock);
        format->setCurrentIndex(std::max(0, format->findData(formatId(o.format))));
        const QSize crop = geometry::outputSize(project->exportFrame().size());
        size->setItemText(0, open ? tr("Crop size (%1×%2)").arg(crop.width()).arg(crop.height()) : tr("Crop size"));
        size->setCurrentIndex(std::max(0, size->findData(o.sizeId())));
        for (QWidget* w : customRow)
            w->setVisible(o.size == OutputSettings::Size::Custom);
        customW->setValueQuietly(o.custom.width());
        customH->setValueQuietly(o.custom.height());
        lock->setChecked(o.lockRatio);
        lock->setIcon(IconFactory::icon(o.lockRatio ? QStringLiteral("lock") : QStringLiteral("unlock")));
        bool limited = false;
        const QSize out = project->outputSize(&limited);
        note->setText(limited ? tr("Limited to 1920 px on the long side: %1×%2").arg(out.width()).arg(out.height())
                              : QString());
        note->setVisible(open && limited);
        const bool needed = geometry::needsAlpha(project->crop(), project->transform(), project->source().size(),
                                                 project->source().hasAlpha);
        alpha->setItemText(0, open ? (needed ? tr("Auto (on)") : tr("Auto (off)")) : tr("Auto"));
        alpha->setCurrentIndex(std::max(0, alpha->findData(OutputSettings::alphaId(o.alpha))));
        if (!colour->hasFocus()) colour->setText(o.background.toUpper());
        QPixmap swatch(14, 14);
        swatch.fill(QColor(o.background));
        pick->setIcon(QIcon(swatch));
        background->setEnabled(open && !project->outputAlpha());
        // "Source (29.97)": what Source means for this video.
        fps->setItemText(0, open ? tr("Source (%1)").arg(timecode::rateText(project->outputFps())) : tr("Source"));
        int i = fps->findData(o.fpsId());
        const bool isCustom = !o.sourceFps && (i < 0 || *customChosen);
        if (isCustom) i = fps->findData(QStringLiteral("custom"));
        fps->setCurrentIndex(i);
        fpsCustom->setVisible(isCustom && open);
        fpsCustom->setValueQuietly(o.fps.value());
    };
    connect(project, &Project::changed, this, [refresh](Project::Aspect) { refresh(); });

    connect(format, &QComboBox::activated, this, [=] {
        OutputSettings o = project->output();
        o.format = formatFromId(format->currentData().toString());
        project->changeOutput(o);
    });
    connect(size, &QComboBox::activated, this, [=] {
        OutputSettings o = project->output();
        const bool wasCustom = o.size == OutputSettings::Size::Custom;
        o.setSizeId(size->currentData().toString());
        if (o.size == OutputSettings::Size::Custom && !wasCustom) o.custom = project->outputSize(); // from what it is
        project->changeOutput(o);
    });
    auto ratio = [project] {
        const QSizeF f = project->exportFrame().size();
        return f.width() > 0 && f.height() > 0 ? f.height() / f.width() : 1.0;
    };
    connect(customW, &StepSpinBox::valueChanged, this, [=](double v) {
        OutputSettings o = project->output();
        o.custom.setWidth(int(v));
        if (o.lockRatio) o.custom.setHeight(locked(v, ratio()));
        project->changeOutput(o, QStringLiteral("field:output.size"));
    });
    connect(customH, &StepSpinBox::valueChanged, this, [=](double v) {
        OutputSettings o = project->output();
        o.custom.setHeight(int(v));
        if (o.lockRatio) o.custom.setWidth(locked(v, 1.0 / ratio()));
        project->changeOutput(o, QStringLiteral("field:output.size"));
    });
    connect(lock, &QToolButton::clicked, this, [=](bool on) {
        OutputSettings o = project->output();
        o.lockRatio = on;
        if (on) o.custom.setHeight(locked(o.custom.width(), ratio()));
        project->changeOutput(o);
    });
    connect(alpha, &QComboBox::activated, this, [=] {
        OutputSettings o = project->output();
        o.alpha = OutputSettings::alphaFromId(alpha->currentData().toString());
        project->changeOutput(o);
    });
    auto setBackground = [project](const QColor& c) {
        if (!c.isValid() || c.name() == project->output().background) return;
        OutputSettings o = project->output();
        o.background = c.name();
        project->changeOutput(o);
    };
    connect(colour, &QLineEdit::editingFinished, this, [=] { setBackground(QColor(colour->text())); });
    connect(pick, &QToolButton::clicked, this, [=] {
        setBackground(QColorDialog::getColor(QColor(project->output().background), this, tr("Background")));
    });
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
    connect(fpsCustom, &StepSpinBox::valueChanged, this, [=](double v) {
        OutputSettings o = project->output();
        o.sourceFps = false;
        o.fps = OutputSettings::customRate(v);
        project->changeOutput(o, QStringLiteral("field:output.fps"));
    });
    WalkProbes::add(QStringLiteral("output.fps"), [project] { return timecode::rateText(project->outputFps()); });
    WalkProbes::add(QStringLiteral("output.size"), [project] {
        const QSize out = project->outputSize();
        return QStringLiteral("%1x%2").arg(out.width()).arg(out.height());
    });
    WalkProbes::add(QStringLiteral("output.sizeMode"), [project] { return project->output().sizeId(); });
    WalkProbes::add(QStringLiteral("output.alpha"),
                    [project] { return project->outputAlpha() ? QStringLiteral("on") : QStringLiteral("off"); });
    WalkProbes::add(QStringLiteral("output.format"), [project] { return formatId(project->output().format); });
    WalkProbes::add(QStringLiteral("output.background"), [project] { return project->output().background; });
    WalkProbes::add(QStringLiteral("output.note"), [note] { return note->text(); });
    refresh();
}

} // namespace avloop
