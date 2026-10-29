// The Shape tool's options (D-022): Draw — Rectangle, Rounded, Ellipse, Polygon, Freehand (what a drag or clicks make
// next); Role — Select, Cut, Keep only, Exclude (D-023): what a new shape does, and the selected shapes' role (one Undo
// step); X, Y, W, H and Radius of the one selected standard shape; Feather and Strength of every selected shape
// (D-024); Delete; Combine ▸ Unite, Minus front, Intersect, Exclude for two or more selected. The same shapes as the
// Crop section's list.
#include "core/Project.h"
#include "core/ShapeCombine.h"
#include "ui/WalkRunner.h"
#include "ui/options/OptionsBar.h"
#include "ui/theme/IconFactory.h"
#include "ui/tools/ShapeEdit.h"
#include "ui/widgets/ResetLabel.h"
#include "ui/widgets/StepSpinBox.h"

#include <QComboBox>
#include <QLabel>
#include <QMenu>
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
    box->addItem(IconFactory::icon(QStringLiteral("role-keep")), QObject::tr("Keep only"),
                 int(MaskShape::Role::KeepOnly));
    box->setItemData(2, QObject::tr("Keep only where it and the shapes below meet"), Qt::ToolTipRole);
    box->addItem(IconFactory::icon(QStringLiteral("role-exclude")), QObject::tr("Exclude"),
                 int(MaskShape::Role::Exclude));
    box->setItemData(3, QObject::tr("Keep where it or the shapes below are, not both"), Qt::ToolTipRole);
    box->setItemIcon(0, IconFactory::icon(QStringLiteral("role-add")));
    return box;
}

void setShapesRole(Project* project, int roleValue) {
    const auto role = MaskShape::Role(roleValue);
    CropShape c = project->shapeHost();
    bool changed = false;
    for (int i : project->selectedShapes())
        if (c.shapes[i].role != role) {
            c.shapes[i].role = role;
            changed = true;
        }
    if (changed) project->changeShapeHost(c);
}

void setShapesSoftness(Project* project, double feather, double strength, const QString& mergeKey) {
    CropShape c = project->shapeHost();
    bool changed = false;
    for (int i : project->selectedShapes()) {
        MaskShape& s = c.shapes[i];
        const double f = feather >= 0 ? feather : s.feather;
        const double p = strength >= 0 ? strength : s.strength;
        if (f != s.feather || p != s.strength) {
            s.feather = f;
            s.strength = p;
            s = s.clamped();
            changed = true;
        }
    }
    if (changed) project->changeShapeHost(c, mergeKey);
}

bool combineSelected(Project* project, int op) {
    int at = -1;
    CropShape c = project->shapeHost();
    c.shapes = shapecombine::combined(c.shapes, project->selectedShapes(), shapecombine::Op(op), &at);
    if (at < 0) return false;
    project->setSelectedShapes({at});
    project->changeShapeHost(c);
    return true;
}

QMenu* createCombineMenu(Project* project, QWidget* parent) {
    auto* menu = new QMenu(parent);
    const QList<std::pair<QString, shapecombine::Op>> ops = {{QObject::tr("Unite"), shapecombine::Op::Unite},
                                                             {QObject::tr("Minus front"), shapecombine::Op::MinusFront},
                                                             {QObject::tr("Intersect"), shapecombine::Op::Intersect},
                                                             {QObject::tr("Exclude"), shapecombine::Op::Exclude}};
    for (const auto& [name, op] : ops)
        QObject::connect(menu->addAction(name), &QAction::triggered, menu,
                         [project, op] { combineSelected(project, int(op)); });
    return menu;
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
    StepSpinBox* feather = row->addNumber(QStringLiteral("shape.feather"), QObject::tr("Feather"), 0,
                                          MaskShape::kMaxFeather, 1, 0, QStringLiteral(" px"));
    feather->setToolTip(QObject::tr("Soften the shape's edge inward, in output pixels"));
    StepSpinBox* strength =
        row->addNumber(QStringLiteral("shape.strength"), QObject::tr("Strength"), 0, 100, 1, 0, QStringLiteral(" %"));
    strength->setToolTip(QObject::tr("How strongly the shape applies: a Cut at 40 % leaves 60 %"));
    QToolButton* remove =
        row->addIcon(QStringLiteral("shape.delete"), QStringLiteral("close"), QObject::tr("Delete the shape (Delete)"));
    QToolButton* combine = row->addIcon(QStringLiteral("shape.combine"), QStringLiteral("role-add"),
                                        QObject::tr("Combine the selected shapes into one"));
    combine->setMenu(createCombineMenu(project, combine));
    combine->setPopupMode(QToolButton::InstantPopup);
    addAlignTools(row, project);

    // The one selected shape, standard: its rectangle and radius are typed here.
    auto single = [project]() -> int {
        const QList<int> s = project->selectedShapes();
        return s.size() == 1 ? s.first() : -1;
    };
    auto refresh = [=] {
        const QSignalBlocker b1(kind), b2(role);
        kind->setCurrentIndex(kind->findData(int(draft->kind)));
        const QList<int> selected = project->selectedShapes();
        const QList<MaskShape>& shapes = project->shapeHost().shapes;
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
        feather->setEnabled(!selected.isEmpty());
        strength->setEnabled(!selected.isEmpty());
        if (!selected.isEmpty()) {
            feather->setValueQuietly(shapes[selected.first()].feather);
            strength->setValueQuietly(shapes[selected.first()].strength);
        }
        remove->setEnabled(!selected.isEmpty());
        combine->setEnabled(selected.size() >= 2);
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
        CropShape c = project->shapeHost();
        change(c.shapes[i]);
        c.shapes[i] = c.shapes[i].clamped();
        project->changeShapeHost(c, QStringLiteral("field:shape.%1").arg(field));
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
        CropShape c = project->shapeHost();
        c.shapes = shapeedit::removed(c.shapes, project->selectedShapes());
        project->setSelectedShapes({});
        project->changeShapeHost(c);
    });
    QObject::connect(feather, &StepSpinBox::valueChanged, row,
                     [=](double v) { setShapesSoftness(project, v, -1, QStringLiteral("field:shape.feather")); });
    QObject::connect(strength, &StepSpinBox::valueChanged, row,
                     [=](double v) { setShapesSoftness(project, -1, v, QStringLiteral("field:shape.strength")); });
    for (auto [name, box] : {std::pair{"x", x},
                             {"y", y},
                             {"w", w},
                             {"h", h},
                             {"radius", radius},
                             {"feather", feather},
                             {"strength", strength}})
        probeValue(QStringLiteral("options.shape.%1").arg(QLatin1String(name)), box);
    // The labels' resets and the bar's (D-025): what is drawn next, and the selected shapes' role, feather, strength.
    reset::setDefault(kind, int(ShapeDraft::Kind::Rect));
    reset::setDefault(role, int(MaskShape::Role::Add));
    reset::setDefault(feather, 0.0);
    reset::setDefault(strength, 100.0);
    row->setReset(QObject::tr("Reset Shape"), [=] {
        draft->kind = ShapeDraft::Kind::Rect;
        draft->role = MaskShape::Role::Add;
        CropShape c = project->shapeHost();
        for (int i : project->selectedShapes()) {
            c.shapes[i].role = MaskShape::Role::Add;
            c.shapes[i].feather = 0;
            c.shapes[i].strength = 100;
        }
        if (!(c == project->shapeHost())) project->changeShapeHost(c);
        refresh();
    });
    refresh();
    return row;
}

QWidget* createPenOptions(Project* project, ShapeDraft* draft, QWidget* parent) {
    auto* row = new OptionsRow(QStringLiteral("pen"), QObject::tr("Pen"), parent);
    QComboBox* role = createRoleBox(row);
    role->setObjectName(QStringLiteral("options.pen.role"));
    row->add(role, QObject::tr("Role"));
    QLabel* how = row->addLabel(QObject::tr("Drag: smooth · Alt: convert, break · Ctrl: select"));
    how->setToolTip(QObject::tr("Click a segment to add an anchor; Delete removes the selected one"));
    auto refresh = [=] {
        const QSignalBlocker block(role);
        const QList<int> selected = project->selectedShapes();
        const MaskShape::Role shown =
            selected.isEmpty() ? draft->role : project->shapeHost().shapes[selected.first()].role;
        role->setCurrentIndex(std::max(0, role->findData(int(shown))));
        row->setEnabled(project->source().isValid());
    };
    QObject::connect(project, &Project::changed, row, [refresh](Project::Aspect a) {
        if (a == Project::Aspect::Crop || a == Project::Aspect::Shapes || a == Project::Aspect::Source) refresh();
    });
    QObject::connect(role, &QComboBox::activated, row, [=] {
        draft->role = MaskShape::Role(role->currentData().toInt());
        setShapesRole(project, int(draft->role));
    });
    reset::setDefault(role, int(MaskShape::Role::Add));
    refresh();
    return row;
}

} // namespace avloop
