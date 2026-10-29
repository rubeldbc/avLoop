// The Crop section's Shapes (D-022): the shapes inside the frame, top first, each "Rectangle 1 · Select"; a row clicked
// selects the shape (Shift or Ctrl add), the same selection as the Shape tool's; the selected shapes' Role and Delete;
// Combine (D-023) with its op — Unite, Minus front, Intersect, Exclude — for two or more.
#include "core/Project.h"
#include "core/ShapeCombine.h"
#include "ui/WalkRunner.h"
#include "ui/options/OptionsBar.h"
#include "ui/panels/Section.h"
#include "ui/panels/SidePanel.h"
#include "ui/theme/IconFactory.h"
#include "ui/tools/ShapeEdit.h"

#include <QComboBox>
#include <QLabel>
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
    s->addWide(list);
    QComboBox* role = createRoleBox(body);
    role->setObjectName(QStringLiteral("panel.shapes.role"));
    s->addRow(tr("Role"), role);
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
        const QList<MaskShape>& shapes = project->crop().shapes;
        const QList<int> selected = project->selectedShapes();
        const QSignalBlocker block(list);
        list->clear();
        for (qsizetype i = shapes.size() - 1; i >= 0; --i) {
            const MaskShape& m = shapes[i];
            auto* item = new QListWidgetItem(IconFactory::icon(kindIcon(m.kind)),
                                             QStringLiteral("%1 · %2").arg(m.name, roleWord(m.role)), list);
            item->setSelected(selected.contains(int(i)));
        }
        hint->setVisible(shapes.isEmpty());
        list->setVisible(!shapes.isEmpty());
        for (QWidget* w : s->row(role) + s->row(remove) + s->row(op) + s->row(combine))
            w->setVisible(!shapes.isEmpty());
        role->setEnabled(!selected.isEmpty());
        op->setEnabled(selected.size() >= 2);
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
    connect(combine, &QPushButton::clicked, this, [=] { combineSelected(project, op->currentData().toInt()); });
    connect(remove, &QPushButton::clicked, this, [project] {
        CropShape c = project->crop();
        c.shapes = shapeedit::removed(c.shapes, project->selectedShapes());
        project->setSelectedShapes({});
        project->changeCrop(c);
    });

    WalkProbes::add(QStringLiteral("shapes.count"), [project] { return int(project->crop().shapes.size()); });
    WalkProbes::add(QStringLiteral("shapes.selected"), [project] {
        QStringList ids;
        for (int i : project->selectedShapes())
            ids << QString::number(i);
        return ids.join(QLatin1Char(','));
    });
    WalkProbes::add(QStringLiteral("shapes.summary"), [project] { return shapeedit::summary(project->crop().shapes); });
    WalkProbes::add(QStringLiteral("panel.shapes.rows"), [list] {
        QStringList rows;
        for (int r = 0; r < list->count(); ++r)
            rows << list->item(r)->text();
        return rows.join(QLatin1Char('|'));
    });
    refresh();
}

} // namespace avloop
