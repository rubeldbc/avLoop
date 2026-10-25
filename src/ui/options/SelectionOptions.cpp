// The Selection's options (spec §6.2): X, Y (the video's centre, scene px), Scale % (5–1000, step 0.1), W, H (locked:
// one sets the scale; unlocked: shown only, the video never distorted — R-2), Lock, Fit, Fill, Center, Flip H, Flip V,
// Reset, and the upscale warning. Every edit is one Undo step (typing or the wheel in one field merges).
#include "core/Geometry.h"
#include "core/Project.h"
#include "ui/WalkRunner.h"
#include "ui/canvas/Canvas.h"
#include "ui/options/OptionsBar.h"
#include "ui/theme/IconFactory.h"
#include "ui/widgets/StepSpinBox.h"

#include <QBoxLayout>
#include <QLabel>
#include <QToolButton>

namespace avloop {

namespace {

// What Fit, Fill and Center act on: the crop (AV-004), else the video's own frame at 100%.
QRectF frameOf(const Project* p) {
    return QRectF(QPointF(0, 0), p->source().size());
}

} // namespace

QWidget* createSelectionOptions(Project* project, Canvas* canvas, QWidget* parent) {
    auto* row = new OptionsRow(QStringLiteral("select"), QObject::tr("Selection"), parent);
    StepSpinBox* x = row->addNumber(QStringLiteral("x"), QStringLiteral("X"), -100000, 100000, 1, 1);
    StepSpinBox* y = row->addNumber(QStringLiteral("y"), QStringLiteral("Y"), -100000, 100000, 1, 1);
    StepSpinBox* scale =
        row->addNumber(QStringLiteral("scale"), QObject::tr("Scale"), 5, 1000, 0.1, 1, QStringLiteral("%"));
    auto* warning = new QLabel(row);
    warning->setObjectName(QStringLiteral("options.upscale"));
    warning->setPixmap(IconFactory::icon(QStringLiteral("warning"), IconTone::Warning).pixmap(QSize(16, 16)));
    warning->setToolTip(QObject::tr("Upscaled: output may look soft"));
    warning->hide();
    if (auto* box = qobject_cast<QBoxLayout*>(row->layout())) box->insertWidget(box->indexOf(scale) + 1, warning);
    StepSpinBox* w = row->addNumber(QStringLiteral("w"), QStringLiteral("W"), 1, 100000, 1, 1);
    StepSpinBox* h = row->addNumber(QStringLiteral("h"), QStringLiteral("H"), 1, 100000, 1, 1);
    QToolButton* lock = row->addIcon(QStringLiteral("lock"), QStringLiteral("lock"), QObject::tr("Lock W and H"), true);
    lock->setChecked(true);
    row->addSeparator();
    QToolButton* fit = row->addIcon(QStringLiteral("fit"), QStringLiteral("fit"), QObject::tr("Fit inside the crop"));
    QToolButton* fill = row->addIcon(QStringLiteral("fill"), QStringLiteral("fill"), QObject::tr("Fill the crop"));
    QToolButton* center =
        row->addIcon(QStringLiteral("center"), QStringLiteral("center"), QObject::tr("Center on the crop"));
    row->addSeparator();
    QToolButton* flipH =
        row->addIcon(QStringLiteral("flipH"), QStringLiteral("flip-h"), QObject::tr("Flip horizontal"), true);
    QToolButton* flipV =
        row->addIcon(QStringLiteral("flipV"), QStringLiteral("flip-v"), QObject::tr("Flip vertical"), true);
    row->addSeparator();
    QToolButton* reset =
        row->addIcon(QStringLiteral("reset"), QStringLiteral("reset"), QObject::tr("Reset (100%, no flips)"));

    auto refresh = [=] {
        const VideoTransform& t = project->transform();
        const QSizeF video = project->source().size();
        x->setValueQuietly(t.x);
        y->setValueQuietly(t.y);
        scale->setValueQuietly(t.scale * 100.0);
        w->setValueQuietly(video.width() * t.scale);
        h->setValueQuietly(video.height() * t.scale);
        w->setReadOnly(!lock->isChecked());
        h->setReadOnly(!lock->isChecked());
        lock->setIcon(IconFactory::icon(lock->isChecked() ? QStringLiteral("lock") : QStringLiteral("unlock")));
        flipH->setChecked(t.flipH);
        flipV->setChecked(t.flipV);
        const QRectF frame = geometry::exportFrame(QRectF(), t, video);
        warning->setVisible(project->source().isValid() && geometry::effectiveScale(t, frame) > 1.0 + 1e-9);
        row->setEnabled(project->source().isValid());
    };
    QObject::connect(project, &Project::changed, row, [refresh](Project::Aspect) { refresh(); });
    QObject::connect(lock, &QToolButton::toggled, row, refresh);

    auto edit = [project](const QString& field, auto change) {
        VideoTransform t = project->transform();
        change(t);
        project->changeTransform(t, QStringLiteral("field:%1").arg(field));
    };
    QObject::connect(x, &StepSpinBox::valueChanged, row,
                     [=](double v) { edit(QStringLiteral("x"), [v](VideoTransform& t) { t.x = v; }); });
    QObject::connect(y, &StepSpinBox::valueChanged, row,
                     [=](double v) { edit(QStringLiteral("y"), [v](VideoTransform& t) { t.y = v; }); });
    QObject::connect(scale, &StepSpinBox::valueChanged, row,
                     [=](double v) { edit(QStringLiteral("scale"), [v](VideoTransform& t) { t.scale = v / 100.0; }); });
    QObject::connect(w, &StepSpinBox::valueChanged, row, [=](double v) {
        if (lock->isChecked() && project->source().width > 0)
            edit(QStringLiteral("scale"), [=](VideoTransform& t) { t.scale = v / project->source().width; });
    });
    QObject::connect(h, &StepSpinBox::valueChanged, row, [=](double v) {
        if (lock->isChecked() && project->source().height > 0)
            edit(QStringLiteral("scale"), [=](VideoTransform& t) { t.scale = v / project->source().height; });
    });
    QObject::connect(fit, &QToolButton::clicked, row, [=] {
        project->changeTransform(project->transform().fittedTo(frameOf(project), project->source().size()));
    });
    QObject::connect(fill, &QToolButton::clicked, row, [=] {
        project->changeTransform(project->transform().filling(frameOf(project), project->source().size()));
    });
    QObject::connect(center, &QToolButton::clicked, row,
                     [=] { project->changeTransform(project->transform().centredOn(frameOf(project))); });
    QObject::connect(flipH, &QToolButton::clicked, row, [=] {
        VideoTransform t = project->transform();
        t.flipH = !t.flipH;
        project->changeTransform(t);
    });
    QObject::connect(flipV, &QToolButton::clicked, row, [=] {
        VideoTransform t = project->transform();
        t.flipV = !t.flipV;
        project->changeTransform(t);
    });
    QObject::connect(reset, &QToolButton::clicked, row,
                     [=] { project->changeTransform(VideoTransform::initial(project->source().size())); });

    WalkProbes::add(QStringLiteral("options.scale"), [scale] { return scale->value(); });
    WalkProbes::add(QStringLiteral("options.x"), [x] { return x->value(); });
    WalkProbes::add(QStringLiteral("options.y"), [y] { return y->value(); });
    WalkProbes::add(QStringLiteral("options.w"), [w] { return w->value(); });
    WalkProbes::add(QStringLiteral("options.upscaleWarning"), [warning] { return !warning->isHidden(); });
    Q_UNUSED(canvas);
    refresh();
    return row;
}

} // namespace avloop
