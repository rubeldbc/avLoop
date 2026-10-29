// A crop tool's options (spec §6's table): Rect — Ratio, X, Y, W, H, Inverse, Swap W/H, Clear; Rounded — Ratio, X, Y,
// W, H, Inverse, Radius, Feather; Ellipse — Ratio, X, Y, W, H, Inverse, Feather (D-021: the area cut out of the video).
// The ratio list (Free, Original, 1:1 … 4:5, Custom W:H) is shared with the Crop panel; choosing a ratio keeps the
// crop's centre and its area as close as possible.
#include "core/AspectRatio.h"
#include "core/Project.h"
#include "ui/WalkRunner.h"
#include "ui/options/OptionsBar.h"
#include "ui/widgets/ResetLabel.h"
#include "ui/widgets/StepSpinBox.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPointer>
#include <QPushButton>
#include <QToolButton>

#include <cmath>

namespace avloop {

QComboBox* createRatioBox(QWidget* parent) {
    auto* box = new QComboBox(parent);
    box->addItem(QObject::tr("Free"), QStringLiteral("free"));
    box->addItem(QObject::tr("Original"), QStringLiteral("original"));
    for (const QString& id : aspect::presets().mid(2))
        box->addItem(id, id);
    box->addItem(QObject::tr("Custom…"), QStringLiteral("custom"));
    return box;
}

void applyRatio(Project* project, const QString& id) {
    CropShape c = project->crop();
    // Custom… starts from the crop's own proportions in lowest terms (1:1 with no crop).
    c.ratio = id != QLatin1String("custom") ? id
              : c.isEmpty()                 ? QStringLiteral("1:1")
                                            : aspect::reduced(c.rect.width(), c.rect.height());
    const double r = aspect::value(id, project->source().size());
    if (!c.isEmpty() && r > 0) c.rect = aspect::apply(c.rect, r);
    project->changeCrop(c);
}

namespace {

// The id a box shows for a ratio: a W:H outside the list is Custom.
int indexOf(QComboBox* box, const QString& ratio) {
    const int i = box->findData(ratio);
    return i >= 0 ? i : box->findData(QStringLiteral("custom"));
}

} // namespace

QWidget* createCropOptions(Project* project, Canvas* canvas, int kindValue, QWidget* parent) {
    Q_UNUSED(canvas);
    const auto kind = static_cast<CropShape::Kind>(kindValue);
    const QString k = CropShape::kindId(kind);
    const QString title = kind == CropShape::Kind::Ellipse   ? QObject::tr("Crop: Ellipse")
                          : kind == CropShape::Kind::Rounded ? QObject::tr("Crop: Rounded")
                                                             : QObject::tr("Crop: Rectangle");
    auto* row = new OptionsRow(CropShape::toolId(kind), title, parent);
    QComboBox* ratio = createRatioBox(row);
    ratio->setObjectName(QStringLiteral("options.%1.ratio").arg(k));
    row->add(ratio, QObject::tr("Ratio"));
    // Custom W:H, shown only for a custom ratio.
    auto* custom = new QWidget(row);
    auto* customRow = new QHBoxLayout(custom);
    customRow->setContentsMargins(0, 0, 0, 0);
    customRow->setSpacing(4);
    auto* cw = new StepSpinBox(QStringLiteral("options.%1.customW").arg(k), 0.01, 9999, 1, 2, custom);
    auto* ch = new StepSpinBox(QStringLiteral("options.%1.customH").arg(k), 0.01, 9999, 1, 2, custom);
    cw->setFixedWidth(60);
    ch->setFixedWidth(60);
    auto* colon = new QLabel(QStringLiteral(":"), custom);
    colon->setProperty("quiet", true);
    customRow->addWidget(cw);
    customRow->addWidget(colon);
    customRow->addWidget(ch);
    row->add(custom);
    StepSpinBox* x = row->addNumber(k + QStringLiteral(".x"), QStringLiteral("X"), -100000, 100000, 1, 1);
    StepSpinBox* y = row->addNumber(k + QStringLiteral(".y"), QStringLiteral("Y"), -100000, 100000, 1, 1);
    StepSpinBox* w = row->addNumber(k + QStringLiteral(".w"), QStringLiteral("W"), 16, 100000, 1, 1);
    StepSpinBox* h = row->addNumber(k + QStringLiteral(".h"), QStringLiteral("H"), 16, 100000, 1, 1);
    StepSpinBox* radius = nullptr;
    StepSpinBox* feather = nullptr;
    QPushButton* swap = nullptr;
    QToolButton* clear = nullptr;
    QToolButton* invert = row->addIcon(k + QStringLiteral(".invert"), QStringLiteral("invert"),
                                       QObject::tr("Inverse: cut the area out, keep the rest"), true);
    if (kind == CropShape::Kind::Rect) {
        swap = row->addButton(k + QStringLiteral(".swap"), QObject::tr("Swap W/H"));
        clear =
            row->addIcon(k + QStringLiteral(".clear"), QStringLiteral("close"), QObject::tr("Clear the crop (Delete)"));
    }
    if (kind == CropShape::Kind::Rect) {
        // The Frame tool (D-033): a size for the output, the crop centred where it is (else on the video), not
        // inverted.
        auto* frame = new QComboBox(row);
        frame->setObjectName(QStringLiteral("options.rect.frame"));
        frame->addItem(QObject::tr("Frame size…"), QSize());
        for (const QSize& s : {QSize(1920, 1080), QSize(1080, 1920), QSize(1080, 1080), QSize(1280, 720),
                               QSize(720, 1280), QSize(1080, 1350)})
            frame->addItem(QStringLiteral("%1 × %2").arg(s.width()).arg(s.height()), s);
        frame->setToolTip(QObject::tr("The frame: a size for the output, centred where the crop is"));
        row->add(frame);
        QObject::connect(frame, &QComboBox::activated, row, [project, frame](int index) {
            const QSize size = frame->itemData(index).toSize();
            frame->setCurrentIndex(0);
            if (size.isEmpty()) return;
            CropShape c = project->crop();
            const QPointF centre =
                c.isEmpty() ? project->transform().rect(project->source().size()).center() : c.rect.center();
            c.kind = CropShape::Kind::Rect;
            c.invert = false;
            c.ratio = QStringLiteral("free");
            c.rect = QRectF(centre - QPointF(size.width() / 2.0, size.height() / 2.0), QSizeF(size));
            project->changeCrop(c);
        });
    }
    if (kind == CropShape::Kind::Rounded)
        radius = row->addNumber(k + QStringLiteral(".radius"), QObject::tr("Radius"), 0, 100000, 1, 1);
    if (kind != CropShape::Kind::Rect)
        feather =
            row->addNumber(k + QStringLiteral(".feather"), QObject::tr("Feather"), 0, CropShape::kMaxFeather, 1, 0);

    QWidget* customBox = custom;
    auto refresh = [=] {
        const CropShape& c = project->crop();
        const QSignalBlocker block(ratio);
        ratio->setCurrentIndex(indexOf(ratio, c.ratio));
        const bool isCustom = aspect::isCustom(c.ratio) || c.ratio == QLatin1String("custom");
        customBox->setVisible(isCustom);
        if (aspect::isCustom(c.ratio)) {
            const qsizetype colon = c.ratio.indexOf(QLatin1Char(':'));
            cw->setValueQuietly(c.ratio.left(colon).toDouble());
            ch->setValueQuietly(c.ratio.mid(colon + 1).toDouble());
        }
        x->setValueQuietly(c.rect.x());
        y->setValueQuietly(c.rect.y());
        w->setValueQuietly(c.rect.width());
        h->setValueQuietly(c.rect.height());
        for (StepSpinBox* s : {x, y, w, h})
            s->setEnabled(!c.isEmpty());
        if (radius) {
            radius->setMaximum(std::max(0.0, c.maxRadius()));
            radius->setValueQuietly(c.radius);
            radius->setEnabled(!c.isEmpty());
        }
        if (feather) feather->setValueQuietly(c.feather);
        {
            const QSignalBlocker quiet(invert);
            invert->setChecked(c.invert);
        }
        invert->setEnabled(!c.isEmpty());
        row->setEnabled(project->source().isValid());
    };
    QObject::connect(project, &Project::changed, row, [refresh](Project::Aspect) { refresh(); });

    auto edit = [project](const QString& field, auto change) {
        CropShape c = project->crop();
        change(c);
        project->changeCrop(c, QStringLiteral("field:crop.%1").arg(field));
    };
    // W or H with a ratio set: the other side follows, the top-left stays.
    auto ratioOf = [project] { return aspect::value(project->crop().ratio, project->source().size()); };
    QObject::connect(ratio, &QComboBox::activated, row, [=] { applyRatio(project, ratio->currentData().toString()); });
    auto customChanged = [=] { applyRatio(project, aspect::custom(cw->value(), ch->value())); };
    QObject::connect(cw, &StepSpinBox::valueChanged, row, customChanged);
    QObject::connect(ch, &StepSpinBox::valueChanged, row, customChanged);
    QObject::connect(x, &StepSpinBox::valueChanged, row,
                     [=](double v) { edit(QStringLiteral("x"), [v](CropShape& c) { c.rect.moveLeft(v); }); });
    QObject::connect(y, &StepSpinBox::valueChanged, row,
                     [=](double v) { edit(QStringLiteral("y"), [v](CropShape& c) { c.rect.moveTop(v); }); });
    QObject::connect(w, &StepSpinBox::valueChanged, row, [=](double v) {
        const double r = ratioOf();
        edit(QStringLiteral("w"), [v, r](CropShape& c) {
            c.rect.setWidth(v);
            if (r > 0) c.rect.setHeight(v / r);
        });
    });
    QObject::connect(h, &StepSpinBox::valueChanged, row, [=](double v) {
        const double r = ratioOf();
        edit(QStringLiteral("h"), [v, r](CropShape& c) {
            c.rect.setHeight(v);
            if (r > 0) c.rect.setWidth(v * r);
        });
    });
    if (swap)
        QObject::connect(swap, &QPushButton::clicked, row, [=] {
            CropShape c = project->crop();
            if (c.isEmpty()) return;
            const QPointF centre = c.rect.center();
            c.rect = QRectF(centre.x() - c.rect.height() / 2, centre.y() - c.rect.width() / 2, c.rect.height(),
                            c.rect.width());
            const qsizetype colon = c.ratio.indexOf(QLatin1Char(':'));
            if (colon > 0) c.ratio = c.ratio.mid(colon + 1) + QLatin1Char(':') + c.ratio.left(colon); // 16:9 → 9:16
            project->changeCrop(c);
        });
    QObject::connect(invert, &QToolButton::toggled, row, [=](bool on) {
        CropShape c = project->crop();
        if (c.isEmpty() || c.invert == on) return;
        c.invert = on;
        project->changeCrop(c);
    });
    if (clear)
        QObject::connect(clear, &QToolButton::clicked, row, [=] {
            CropShape c = project->crop();
            c.rect = QRectF();
            c.invert = false;
            project->changeCrop(c);
        });
    if (radius)
        QObject::connect(radius, &StepSpinBox::valueChanged, row,
                         [=](double v) { edit(QStringLiteral("radius"), [v](CropShape& c) { c.radius = v; }); });
    if (feather)
        QObject::connect(feather, &StepSpinBox::valueChanged, row,
                         [=](double v) { edit(QStringLiteral("feather"), [v](CropShape& c) { c.feather = v; }); });
    for (auto [name, box] : {std::pair{"x", x}, {"y", y}, {"w", w}, {"h", h}, {"radius", radius}, {"feather", feather}})
        if (box) probeValue(QStringLiteral("options.%1.%2").arg(k, QLatin1String(name)), box);
    WalkProbes::add(QStringLiteral("options.%1.ratio").arg(k),
                    [p = QPointer<QComboBox>(ratio)] { return p ? p->currentData() : QVariant(); });
    // The labels' resets and the bar's (D-025): the crop's ratio, radius, feather and inverse; its place stays.
    reset::setDefault(ratio, QStringLiteral("free"));
    if (radius) reset::setDefault(radius, 0.0);
    if (feather) reset::setDefault(feather, 0.0);
    row->setReset(QObject::tr("Reset %1").arg(title), [project] {
        CropShape c = project->crop();
        c.ratio = QStringLiteral("free");
        c.radius = 0;
        c.feather = 0;
        c.invert = false;
        if (!(c == project->crop())) project->changeCrop(c);
    });
    refresh();
    return row;
}

} // namespace avloop
