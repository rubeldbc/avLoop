// The Crop section's Shapes (D-022): the shapes inside the frame, top first, each "Rectangle 1 · Select"; a row clicked
// selects the shape (Shift or Ctrl add), the same selection as the Shape tool's; the selected shapes' Role and Delete;
// Combine (D-023) with its op — Unite, Minus front, Intersect, Exclude — for two or more; Feather and Strength (D-024);
// the layers (D-026): each row's check box shows or hides it, a drag reorders; Name, Visible, Locked, Up, Down,
// Duplicate for the one selected.
#include "core/Project.h"
#include "core/ShapeAlign.h"
#include "core/ShapeCombine.h"
#include "ui/WalkRunner.h"
#include "ui/options/OptionsBar.h"
#include "ui/panels/Section.h"
#include "ui/panels/SidePanel.h"
#include "ui/theme/IconFactory.h"
#include "ui/tools/ShapeEdit.h"
#include "ui/widgets/ResetLabel.h"
#include "ui/widgets/StepSpinBox.h"

#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>

namespace avloop {

namespace {

QString roleWord(MaskShape::Role r) {
    switch (r) {
    case MaskShape::Role::Cut:
        return QObject::tr("Cut");
    case MaskShape::Role::KeepOnly:
        return QObject::tr("Keep only");
    case MaskShape::Role::Exclude:
        return QObject::tr("Exclude");
    case MaskShape::Role::Add:
        break;
    }
    return QObject::tr("Select");
}

// " · 20 px · 60 %" when a shape is softer or weaker than the defaults (D-024).
QString soft(const MaskShape& m) {
    QString note;
    if (m.feather > 0) note += QStringLiteral(" · %1 px").arg(m.feather);
    if (m.strength < 100) note += QStringLiteral(" · %1 %").arg(m.strength);
    return note;
}

QString kindIcon(MaskShape::Kind k) {
    switch (k) {
    case MaskShape::Kind::Ellipse:
        return QStringLiteral("tool-crop-ellipse");
    case MaskShape::Kind::Rounded:
        return QStringLiteral("tool-crop-round");
    case MaskShape::Kind::Polygon:
        return QStringLiteral("shape-polygon");
    case MaskShape::Kind::Path:
        return QStringLiteral("shape-freehand");
    case MaskShape::Kind::Rect:
        break;
    }
    return QStringLiteral("tool-crop-rect");
}

} // namespace

void SidePanel::bindShapes(Project* project) {
    Section* s = crop_;
    QWidget* body = s->body();
    auto* title = new QLabel(tr("Shapes"), body);
    title->setProperty("subhead", true);
    s->addWide(title);
    auto* hint = new QLabel(tr("Draw shapes with S: each selects or cuts inside the crop."), body);
    hint->setProperty("quiet", true);
    hint->setWordWrap(true);
    s->addWide(hint);
    auto* list = new QListWidget(body);
    list->setObjectName(QStringLiteral("panel.shapes.list"));
    list->setSelectionMode(QAbstractItemView::ExtendedSelection);
    list->setFixedHeight(112);
    list->setIconSize(QSize(16, 16));
    list->setDragDropMode(QAbstractItemView::InternalMove); // a drag reorders the layers (D-026)
    list->setToolTip(tr("Top first. Drag to reorder; the check box shows or hides a shape"));
    s->addWide(list);
    // The layer's own rows (D-026): its name, shown or hidden, locked; Up, Down, Duplicate.
    auto* name = new QLineEdit(body);
    name->setObjectName(QStringLiteral("panel.shapes.name"));
    s->addRow(tr("Name"), name);
    auto* flags = new QWidget(body);
    auto* flagsRow = new QHBoxLayout(flags);
    flagsRow->setContentsMargins(0, 0, 0, 0);
    auto* shown = new QCheckBox(tr("Visible"), flags);
    shown->setObjectName(QStringLiteral("panel.shapes.visible"));
    auto* locked = new QCheckBox(tr("Locked"), flags);
    locked->setObjectName(QStringLiteral("panel.shapes.locked"));
    locked->setToolTip(tr("A locked shape is not picked or moved on the canvas"));
    flagsRow->addWidget(shown);
    flagsRow->addWidget(locked);
    flagsRow->addStretch(1);
    s->addRow(QString(), flags);
    auto* order = new QWidget(body);
    auto* orderRow = new QHBoxLayout(order);
    orderRow->setContentsMargins(0, 0, 0, 0);
    orderRow->setSpacing(4);
    auto* up = new QPushButton(tr("Up"), order);
    up->setObjectName(QStringLiteral("panel.shapes.up"));
    auto* down = new QPushButton(tr("Down"), order);
    down->setObjectName(QStringLiteral("panel.shapes.down"));
    auto* duplicate = new QPushButton(tr("Duplicate"), order);
    duplicate->setObjectName(QStringLiteral("panel.shapes.duplicate"));
    for (QPushButton* b : {up, down, duplicate})
        orderRow->addWidget(b);
    s->addRow(QString(), order);
    QComboBox* role = createRoleBox(body);
    role->setObjectName(QStringLiteral("panel.shapes.role"));
    s->addRow(tr("Role"), role);
    auto* feather = new StepSpinBox(QStringLiteral("panel.shapes.feather"), 0, MaskShape::kMaxFeather, 1, 0, body);
    feather->setSuffix(QStringLiteral(" px"));
    feather->setToolTip(tr("Soften the selected shapes' edges inward"));
    s->addRow(tr("Feather"), feather);
    auto* strength = new StepSpinBox(QStringLiteral("panel.shapes.strength"), 0, 100, 1, 0, body);
    strength->setSuffix(QStringLiteral(" %"));
    strength->setToolTip(tr("A Cut at 40 % leaves 60 % of what it covers"));
    s->addRow(tr("Strength"), strength);
    auto* remove = new QPushButton(tr("Delete"), body);
    remove->setObjectName(QStringLiteral("panel.shapes.delete"));
    remove->setToolTip(tr("Delete the selected shapes"));
    s->addRow(QString(), remove);
    // Combine (D-023): the op, then the button — the selected shapes baked into one path.
    auto* op = new QComboBox(body);
    op->setObjectName(QStringLiteral("panel.shapes.combineOp"));
    op->addItem(tr("Unite"), int(shapecombine::Op::Unite));
    op->addItem(tr("Minus front"), int(shapecombine::Op::MinusFront));
    op->addItem(tr("Intersect"), int(shapecombine::Op::Intersect));
    op->addItem(tr("Exclude"), int(shapecombine::Op::Exclude));
    s->addRow(tr("Combine"), op);
    auto* combine = new QPushButton(tr("Combine"), body);
    combine->setObjectName(QStringLiteral("panel.shapes.combine"));
    combine->setToolTip(tr("Two or more selected shapes into one path"));
    s->addRow(QString(), combine);

    // Rows top first: row r is shape (n − 1 − r).
    auto shapeOf = [list](int row) { return list->count() - 1 - row; };
    auto refresh = [=] {
        const QList<MaskShape>& shapes = project->shapeHost().shapes;
        const QList<int> selected = project->selectedShapes();
        const QSignalBlocker block(list);
        list->clear();
        for (qsizetype i = shapes.size() - 1; i >= 0; --i) {
            const MaskShape& m = shapes[i];
            auto* item = new QListWidgetItem(IconFactory::icon(m.locked ? QStringLiteral("lock") : kindIcon(m.kind)),
                                             QStringLiteral("%1 · %2%3").arg(m.name, roleWord(m.role), soft(m)), list);
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsDragEnabled);
            item->setFlags(item->flags() & ~Qt::ItemIsDropEnabled);
            item->setCheckState(m.visible ? Qt::Checked : Qt::Unchecked);
            item->setData(Qt::UserRole, int(i));
            item->setSelected(selected.contains(int(i)));
        }
        hint->setVisible(shapes.isEmpty());
        list->setVisible(!shapes.isEmpty());
        for (QWidget* w : s->row(role) + s->row(feather) + s->row(strength) + s->row(remove) + s->row(op) +
                              s->row(combine) + s->row(name) + s->row(flags) + s->row(order))
            w->setVisible(!shapes.isEmpty());
        const bool one = selected.size() == 1;
        for (QWidget* w : {static_cast<QWidget*>(name), static_cast<QWidget*>(flags), static_cast<QWidget*>(order)})
            w->setEnabled(one);
        if (one) {
            const MaskShape& m = shapes[selected.first()];
            if (!name->hasFocus()) name->setText(m.name);
            const QSignalBlocker b1(shown), b2(locked);
            shown->setChecked(m.visible);
            locked->setChecked(m.locked);
            up->setEnabled(selected.first() < shapes.size() - 1);
            down->setEnabled(selected.first() > 0);
        }
        role->setEnabled(!selected.isEmpty());
        op->setEnabled(selected.size() >= 2);
        feather->setEnabled(!selected.isEmpty());
        strength->setEnabled(!selected.isEmpty());
        if (!selected.isEmpty()) {
            feather->setValueQuietly(shapes[selected.first()].feather);
            strength->setValueQuietly(shapes[selected.first()].strength);
        }
        combine->setEnabled(selected.size() >= 2);
        remove->setEnabled(!selected.isEmpty());
        if (!selected.isEmpty()) {
            const QSignalBlocker b(role);
            role->setCurrentIndex(std::max(0, role->findData(int(shapes[selected.first()].role))));
        }
    };
    connect(project, &Project::changed, this, [refresh](Project::Aspect a) {
        if (a == Project::Aspect::Crop || a == Project::Aspect::Shapes || a == Project::Aspect::Source) refresh();
    });
    connect(list, &QListWidget::itemSelectionChanged, this, [=] {
        QList<int> chosen;
        for (int r = 0; r < list->count(); ++r)
            if (list->item(r)->isSelected()) chosen.append(shapeOf(r));
        project->setSelectedShapes(chosen);
    });
    connect(role, &QComboBox::activated, this, [=] { setShapesRole(project, role->currentData().toInt()); });
    // The layers (D-026): each change one Undo step.
    auto editOne = [project](auto change) {
        const QList<int> sel = project->selectedShapes();
        if (sel.size() != 1) return;
        CropShape c = project->shapeHost();
        change(c.shapes[sel.first()]);
        if (!(c == project->shapeHost())) project->changeShapeHost(c);
    };
    connect(name, &QLineEdit::editingFinished, this, [=] {
        const QString text = name->text().trimmed();
        if (!text.isEmpty()) editOne([text](MaskShape& m) { m.name = text; });
    });
    connect(shown, &QCheckBox::clicked, this, [=](bool on) { editOne([on](MaskShape& m) { m.visible = on; }); });
    connect(locked, &QCheckBox::clicked, this, [=](bool on) { editOne([on](MaskShape& m) { m.locked = on; }); });
    connect(list, &QListWidget::itemChanged, this, [=](QListWidgetItem* item) {
        const int i = item->data(Qt::UserRole).toInt();
        CropShape c = project->shapeHost();
        if (i < 0 || i >= c.shapes.size()) return;
        c.shapes[i].visible = item->checkState() == Qt::Checked;
        if (!(c == project->shapeHost())) project->changeShapeHost(c);
    });
    auto move = [project](int from, int to) {
        CropShape c = project->shapeHost();
        c.shapes = shapealign::moved(c.shapes, from, to);
        if (c == project->shapeHost()) return;
        project->setSelectedShapes({to});
        project->changeShapeHost(c);
    };
    connect(up, &QPushButton::clicked, this, [=] {
        if (const QList<int> sel = project->selectedShapes(); sel.size() == 1) move(sel.first(), sel.first() + 1);
    });
    connect(down, &QPushButton::clicked, this, [=] {
        if (const QList<int> sel = project->selectedShapes(); sel.size() == 1) move(sel.first(), sel.first() - 1);
    });
    connect(duplicate, &QPushButton::clicked, this, [=] {
        const QList<int> sel = project->selectedShapes();
        if (sel.size() != 1) return;
        CropShape c = project->shapeHost();
        c.shapes = shapealign::duplicated(c.shapes, sel.first());
        if (c == project->shapeHost()) return;
        project->setSelectedShapes({sel.first() + 1});
        project->changeShapeHost(c);
    });
    // A row dragged: the list's new order (top first) is the shapes' order reversed.
    connect(list->model(), &QAbstractItemModel::rowsMoved, this, [=] {
        CropShape c = project->shapeHost();
        QList<MaskShape> next;
        for (int r = list->count() - 1; r >= 0; --r) {
            const int i = list->item(r)->data(Qt::UserRole).toInt();
            if (i >= 0 && i < c.shapes.size()) next.append(c.shapes[i]);
        }
        if (next.size() != c.shapes.size()) return;
        c.shapes = next;
        project->setSelectedShapes({});
        if (!(c == project->shapeHost())) project->changeShapeHost(c);
    });
    connect(feather, &StepSpinBox::valueChanged, this,
            [=](double v) { setShapesSoftness(project, v, -1, QStringLiteral("field:panel.shapes.feather")); });
    connect(strength, &StepSpinBox::valueChanged, this,
            [=](double v) { setShapesSoftness(project, -1, v, QStringLiteral("field:panel.shapes.strength")); });
    connect(combine, &QPushButton::clicked, this, [=] { combineSelected(project, op->currentData().toInt()); });
    connect(remove, &QPushButton::clicked, this, [project] {
        CropShape c = project->shapeHost();
        c.shapes = shapeedit::removed(c.shapes, project->selectedShapes());
        project->setSelectedShapes({});
        project->changeShapeHost(c);
    });

    WalkProbes::add(QStringLiteral("shapes.count"), [project] { return int(project->shapeHost().shapes.size()); });
    WalkProbes::add(QStringLiteral("shapes.selected"), [project] {
        QStringList ids;
        for (int i : project->selectedShapes())
            ids << QString::number(i);
        return ids.join(QLatin1Char(','));
    });
    WalkProbes::add(QStringLiteral("shapes.summary"),
                    [project] { return shapeedit::summary(project->shapeHost().shapes); });
    // Each shape's layer flags (D-026): "v" shown or "h" hidden, then "l" locked or "u", "," between.
    WalkProbes::add(QStringLiteral("shapes.flags"), [project] {
        QStringList parts;
        for (const MaskShape& m : project->shapeHost().shapes)
            parts << QStringLiteral("%1%2").arg(m.visible ? 'v' : 'h').arg(m.locked ? 'l' : 'u');
        return parts.join(QLatin1Char(','));
    });
    // Each shape's feather and strength, "f/s" a shape, ";" between (D-024).
    WalkProbes::add(QStringLiteral("shapes.soft"), [project] {
        QStringList parts;
        for (const MaskShape& m : project->shapeHost().shapes)
            parts << QStringLiteral("%1/%2").arg(m.feather).arg(m.strength);
        return parts.join(QLatin1Char(';'));
    });
    WalkProbes::add(QStringLiteral("panel.shapes.rows"), [list] {
        QStringList rows;
        for (int r = 0; r < list->count(); ++r)
            rows << list->item(r)->text();
        return rows.join(QLatin1Char('|'));
    });
    reset::setDefault(role, int(MaskShape::Role::Add));
    reset::setDefault(feather, 0.0);
    reset::setDefault(strength, 100.0);
    reset::setDefault(op, int(shapecombine::Op::Unite));
    s->addResetAction(tr("Reset shapes"), QStringLiteral("shapes"), [project] {
        CropShape c = project->shapeHost();
        if (c.shapes.isEmpty()) return;
        c.shapes.clear();
        project->setSelectedShapes({});
        project->changeShapeHost(c);
    });
    refresh();
}

} // namespace avloop
