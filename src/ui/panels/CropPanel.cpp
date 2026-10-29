// The side panel's Crop section (spec §5.1, §6.1): Shape, Inverse (D-021), Ratio, X / Y, W / H, Radius, Feather and the
// output's size with whether it carries alpha. The same crop as the canvas and the options bar; every edit one Undo
// step.
#include "core/AspectRatio.h"
#include "core/Project.h"
#include "ui/WalkRunner.h"
#include "ui/options/OptionsBar.h"
#include "ui/panels/Section.h"
#include "ui/panels/SidePanel.h"
#include "ui/widgets/ResetLabel.h"
#include "ui/widgets/StepSpinBox.h"

#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>

namespace avloop {

namespace {

QWidget* pair(QWidget* parent, StepSpinBox* a, StepSpinBox* b) {
    auto* w = new QWidget(parent);
    auto* row = new QHBoxLayout(w);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(4);
    a->setParent(w);
    b->setParent(w);
    a->setMinimumWidth(44);
    b->setMinimumWidth(44);
    row->addWidget(a);
    row->addWidget(b);
    return w;
}

} // namespace

void SidePanel::bindCrop(Project* project) {
    Section* s = crop_;
    QWidget* body = s->body();
    auto* hint = new QLabel(tr("No crop: the whole video is exported."), body);
    hint->setProperty("quiet", true);
    hint->setWordWrap(true);
    s->addWide(hint);
    auto* shape = new QComboBox(body);
    shape->setObjectName(QStringLiteral("panel.crop.shape"));
    shape->addItem(tr("Rectangle"), int(CropShape::Kind::Rect));
    shape->addItem(tr("Rounded"), int(CropShape::Kind::Rounded));
    shape->addItem(tr("Ellipse"), int(CropShape::Kind::Ellipse));
    s->addRow(tr("Shape"), shape);
    auto* invert = new QCheckBox(tr("Inverse"), body);
    invert->setObjectName(QStringLiteral("panel.crop.invert"));
    invert->setToolTip(tr("The chosen area is cut out; the rest is kept"));
    invert->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    s->addRow(QString(), invert);
    QComboBox* ratio = createRatioBox(body);
    ratio->setObjectName(QStringLiteral("panel.crop.ratio"));
    s->addRow(tr("Ratio"), ratio);
    auto* x = new StepSpinBox(QStringLiteral("panel.crop.x"), -100000, 100000, 1, 1);
    auto* y = new StepSpinBox(QStringLiteral("panel.crop.y"), -100000, 100000, 1, 1);
    s->addRow(tr("X / Y"), pair(body, x, y));
    auto* w = new StepSpinBox(QStringLiteral("panel.crop.w"), 16, 100000, 1, 1);
    auto* h = new StepSpinBox(QStringLiteral("panel.crop.h"), 16, 100000, 1, 1);
    s->addRow(tr("W / H"), pair(body, w, h));
    auto* radius = new StepSpinBox(QStringLiteral("panel.crop.radius"), 0, 100000, 1, 1, body);
    s->addRow(tr("Radius"), radius);
    auto* feather = new StepSpinBox(QStringLiteral("panel.crop.feather"), 0, CropShape::kMaxFeather, 1, 0, body);
    feather->setSuffix(QStringLiteral(" px"));
    s->addRow(tr("Feather"), feather);
    auto* output = new QLabel(body);
    output->setObjectName(QStringLiteral("panel.crop.output"));
    output->setProperty("mono", true);
    s->addRow(tr("Output"), output);

    auto refresh = [=] {
        const CropShape& c = project->crop();
        hint->setVisible(c.isEmpty());
        {
            const QSignalBlocker b1(shape);
            const QSignalBlocker b2(ratio);
            shape->setCurrentIndex(shape->findData(int(c.kind)));
            const QSignalBlocker b3(invert);
            invert->setChecked(c.invert);
            const int r = ratio->findData(c.ratio);
            ratio->setCurrentIndex(r >= 0 ? r : ratio->findData(QStringLiteral("custom")));
        }
        x->setValueQuietly(c.rect.x());
        y->setValueQuietly(c.rect.y());
        w->setValueQuietly(c.rect.width());
        h->setValueQuietly(c.rect.height());
        radius->setMaximum(std::max(0.0, c.maxRadius()));
        radius->setValueQuietly(c.radius);
        feather->setValueQuietly(c.feather);
        for (QWidget* f : {static_cast<QWidget*>(x), static_cast<QWidget*>(y), static_cast<QWidget*>(w),
                           static_cast<QWidget*>(h), static_cast<QWidget*>(radius)})
            f->setEnabled(!c.isEmpty());
        radius->setEnabled(!c.isEmpty() && c.kind == CropShape::Kind::Rounded);
        invert->setEnabled(!c.isEmpty());
        // What the export makes of it: the Output section's size and alpha (spec §10.2).
        const QSize out = project->outputSize();
        const bool alpha = project->outputAlpha();
        output->setText(
            QStringLiteral("%1 × %2%3").arg(out.width()).arg(out.height()).arg(alpha ? tr(" · alpha") : QString()));
        setEnabled(project->source().isValid());
    };
    connect(project, &Project::changed, this, [refresh](Project::Aspect a) {
        if (a != Project::Aspect::Time) refresh(); // the playhead changes nothing here
    });

    auto edit = [project](const QString& field, auto change) {
        CropShape c = project->crop();
        change(c);
        project->changeCrop(c, QStringLiteral("field:panel.%1").arg(field));
    };
    connect(shape, &QComboBox::activated, this, [=] {
        const auto kind = static_cast<CropShape::Kind>(shape->currentData().toInt());
        edit(QStringLiteral("kind"), [kind](CropShape& c) { c.kind = kind; });
    });
    connect(invert, &QCheckBox::clicked, this, [=](bool on) {
        if (project->crop().isEmpty()) return;
        edit(QStringLiteral("invert"), [on](CropShape& c) { c.invert = on; });
    });
    connect(ratio, &QComboBox::activated, this, [=] { applyRatio(project, ratio->currentData().toString()); });
    connect(x, &StepSpinBox::valueChanged, this,
            [=](double v) { edit(QStringLiteral("x"), [v](CropShape& c) { c.rect.moveLeft(v); }); });
    connect(y, &StepSpinBox::valueChanged, this,
            [=](double v) { edit(QStringLiteral("y"), [v](CropShape& c) { c.rect.moveTop(v); }); });
    connect(w, &StepSpinBox::valueChanged, this, [=](double v) {
        const double r = aspect::value(project->crop().ratio, project->source().size());
        edit(QStringLiteral("w"), [v, r](CropShape& c) {
            c.rect.setWidth(v);
            if (r > 0) c.rect.setHeight(v / r);
        });
    });
    connect(h, &StepSpinBox::valueChanged, this, [=](double v) {
        const double r = aspect::value(project->crop().ratio, project->source().size());
        edit(QStringLiteral("h"), [v, r](CropShape& c) {
            c.rect.setHeight(v);
            if (r > 0) c.rect.setWidth(v * r);
        });
    });
    connect(radius, &StepSpinBox::valueChanged, this,
            [=](double v) { edit(QStringLiteral("radius"), [v](CropShape& c) { c.radius = v; }); });
    connect(feather, &StepSpinBox::valueChanged, this,
            [=](double v) { edit(QStringLiteral("feather"), [v](CropShape& c) { c.feather = v; }); });
    WalkProbes::add(QStringLiteral("panel.crop.output"), [output] { return output->text(); });
    WalkProbes::add(QStringLiteral("panel.crop.hint"), [hint] { return !hint->isHidden(); });
    WalkProbes::add(QStringLiteral("panel.crop.w"), [w] { return w->value(); });
    // The labels' resets and the group's (D-025): a position has no default.
    reset::setDefault(shape, int(CropShape::Kind::Rect));
    reset::setDefault(invert, false);
    reset::setDefault(ratio, QStringLiteral("free"));
    reset::setDefault(radius, 0.0);
    reset::setDefault(feather, 0.0);
    s->addResetAction(tr("Reset Crop"), QStringLiteral("crop"), [project] { project->changeCrop(CropShape{}); });
    refresh();
}

} // namespace avloop
