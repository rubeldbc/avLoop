// The Shape tool's options (D-022): Draw — Rectangle, Rounded, Ellipse, Polygon, Freehand (what a drag or clicks make
// next); Role — what a new shape does, and the selected shapes' role (one Undo step); X, Y, W, H and Radius of the one
// selected standard shape; Delete. The same shapes as the Crop section's list.
#include "core/Project.h"
#include "ui/WalkRunner.h"
#include "ui/options/OptionsBar.h"
#include "ui/theme/IconFactory.h"
#include "ui/tools/ShapeEdit.h"
#include "ui/widgets/StepSpinBox.h"

#include <QComboBox>
#include <QPointer>
#include <QToolButton>

namespace avloop {

QComboBox* createRoleBox(QWidget* parent) {
    auto* box = new QComboBox(parent);
    box->addItem(QObject::tr("Select"), int(MaskShape::Role::Add));
    box->setItemData(0, QObject::tr("Keep what it covers"), Qt::ToolTipRole);
    box->addItem(QObject::tr("Cut"), int(MaskShape::Role::Cut));
    box->setItemData(1, QObject::tr("Make what it covers transparent"), Qt::ToolTipRole);
    box->setItemIcon(1, IconFactory::icon(QStringLiteral("shape-cut")));
    return box;
}

void setShapesRole(Project* project, int roleValue) {
    const auto role = MaskShape::Role(roleValue);
    CropShape c = project->crop();
    bool changed = false;
    for (int i : project->selectedShapes())
        if (c.shapes[i].role != role) {
            c.shapes[i].role = role;
            changed = true;
        }
    if (changed) project->changeCrop(c);
}

QWidget* createShapeOptions(Project* project, ShapeDraft* draft, QWidget* parent) {
    auto* row = new OptionsRow(QStringLiteral("shape"), QObject::tr("Shape"), parent);
    auto* kind = new QComboBox(row);
    kind->setObjectName(QStringLiteral("options.shape.kind"));
    kind->addItem(QObject::tr("Rectangle"), int(ShapeDraft::Kind::Rect));
    kind->addItem(QObject::tr("Rounded"), int(ShapeDraft::Kind::Rounded));
    kind->addItem(QObject::tr("Ellipse"), int(ShapeDraft::Kind::Ellipse));
    kind->addItem(QObject::tr("Polygon"), int(ShapeDraft::Kind::Polygon));
    kind->addItem(QObject::tr("Freehand"), int(ShapeDraft::Kind::Freehand));
    kind->setToolTip(QObject::tr("What a drag or clicks draw next"));
    row->add(kind, QObject::tr("Draw"));
    QComboBox* role = createRoleBox(row);
    role->setObjectName(QStringLiteral("options.shape.role"));
    row->add(role, QObject::tr("Role"));
    StepSpinBox* x = row->addNumber(QStringLiteral("shape.x"), QStringLiteral("X"), -100000, 100000, 1, 1);
    StepSpinBox* y = row->addNumber(QStringLiteral("shape.y"), QStringLiteral("Y"), -100000, 100000, 1, 1);
    StepSpinBox* w = row->addNumber(QStringLiteral("shape.w"), QStringLiteral("W"), 1, 100000, 1, 1);
    StepSpinBox* h = row->addNumber(QStringLiteral("shape.h"), QStringLiteral("H"), 1, 100000, 1, 1);
    StepSpinBox* radius = row->addNumber(QStringLiteral("shape.radius"), QObject::tr("Radius"), 0, 100000, 1, 1);
    QToolButton* remove =
        row->addIcon(QStringLiteral("shape.delete"), QStringLiteral("close"), QObject::tr("Delete the shape (Delete)"));

    // The one selected shape, standard: its rectangle and radius are typed here.
    auto single = [project]() -> int {
        const QList<int> s = project->selectedShapes();
        return s.size() == 1 ? s.first() : -1;
    };
    auto refresh = [=] {
        const QSignalBlocker b1(kind), b2(role);
        kind->setCurrentIndex(kind->findData(int(draft->kind)));
        const QList<int> selected = project->selectedShapes();
        const QList<MaskShape>& shapes = project->crop().shapes;
        const MaskShape::Role shown = selected.isEmpty() ? draft->role : shapes[selected.first()].role;
        role->setCurrentIndex(std::max(0, role->findData(int(shown))));
        const int i = single();
        const bool standard = i >= 0 && shapes[i].isStandard();
        for (StepSpinBox* box : {x, y, w, h, radius})
            box->setEnabled(standard);
        if (standard) {
            const MaskShape& s = shapes[i];
            x->setValueQuietly(s.rect.x());
            y->setValueQuietly(s.rect.y());
            w->setValueQuietly(s.rect.width());
            h->setValueQuietly(s.rect.height());
            radius->setMaximum(std::max(0.0, std::min(s.rect.width(), s.rect.height()) / 2));
            radius->setValueQuietly(s.radius);
        }
        radius->setEnabled(standard && shapes[i].kind == MaskShape::Kind::Rounded);
        remove->setEnabled(!selected.isEmpty());
        row->setEnabled(project->source().isValid());
    };
    QObject::connect(project, &Project::changed, row, [refresh](Project::Aspect a) {
        if (a == Project::Aspect::Crop || a == Project::Aspect::Shapes || a == Project::Aspect::Source) refresh();
    });
    QObject::connect(kind, &QComboBox::activated, row,
                     [=] { draft->kind = ShapeDraft::Kind(kind->currentData().toInt()); });
    QObject::connect(role, &QComboBox::activated, row, [=] {
        draft->role = MaskShape::Role(role->currentData().toInt());
        setShapesRole(project, int(draft->role));
    });
    auto edit = [=](const QString& field, auto change) {
        const int i = single();
        if (i < 0) return;
        CropShape c = project->crop();
        change(c.shapes[i]);
        c.shapes[i] = c.shapes[i].clamped();
        project->changeCrop(c, QStringLiteral("field:shape.%1").arg(field));
    };
    QObject::connect(x, &StepSpinBox::valueChanged, row,
                     [=](double v) { edit(QStringLiteral("x"), [v](MaskShape& s) { s.rect.moveLeft(v); }); });
    QObject::connect(y, &StepSpinBox::valueChanged, row,
                     [=](double v) { edit(QStringLiteral("y"), [v](MaskShape& s) { s.rect.moveTop(v); }); });
    QObject::connect(w, &StepSpinBox::valueChanged, row,
                     [=](double v) { edit(QStringLiteral("w"), [v](MaskShape& s) { s.rect.setWidth(v); }); });
    QObject::connect(h, &StepSpinBox::valueChanged, row,
                     [=](double v) { edit(QStringLiteral("h"), [v](MaskShape& s) { s.rect.setHeight(v); }); });
    QObject::connect(radius, &StepSpinBox::valueChanged, row,
                     [=](double v) { edit(QStringLiteral("radius"), [v](MaskShape& s) { s.radius = v; }); });
    QObject::connect(remove, &QToolButton::clicked, row, [project] {
        CropShape c = project->crop();
        c.shapes = shapeedit::removed(c.shapes, project->selectedShapes());
        project->setSelectedShapes({});
        project->changeCrop(c);
    });
    for (auto [name, box] : {std::pair{"x", x}, {"y", y}, {"w", w}, {"h", h}, {"radius", radius}})
        probeValue(QStringLiteral("options.shape.%1").arg(QLatin1String(name)), box);
    refresh();
    return row;
}

} // namespace avloop
