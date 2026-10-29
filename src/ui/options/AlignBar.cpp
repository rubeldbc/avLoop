// The Shape bar's Align (D-026), as Illustrator's: To — Selection (the selected shapes' box) or Frame (the crop's, else
// the video's); align left, centre, right, top, middle, bottom (two or more shapes, or one to the frame); distribute
// horizontally and vertically (three or more). Shapes move, never resize; one Undo step a press.
#include "core/Project.h"
#include "core/ShapeAlign.h"
#include "ui/WalkRunner.h"
#include "ui/options/OptionsBar.h"

#include <QComboBox>
#include <QToolButton>

namespace avloop {

void addAlignTools(OptionsRow* row, Project* project) {
    row->addSeparator();
    auto* to = new QComboBox(row);
    to->setObjectName(QStringLiteral("options.shape.alignTo"));
    to->addItem(QObject::tr("Selection"), 0);
    to->addItem(QObject::tr("Frame"), 1);
    to->setToolTip(QObject::tr("Align to the selected shapes, or to the crop's frame"));
    row->add(to, QObject::tr("Align to"));
    auto frame = [project, to] { return to->currentData().toInt() == 1 ? project->exportFrame() : QRectF(); };
    struct Button {
        const char* name;
        const char* icon;
        const char* tip;
        shapealign::Align how;
    };
    using A = shapealign::Align;
    const Button aligns[] = {{"left", "align-left", QT_TR_NOOP("Align left edges"), A::Left},
                             {"centre", "align-centre", QT_TR_NOOP("Align horizontal centres"), A::Centre},
                             {"right", "align-right", QT_TR_NOOP("Align right edges"), A::Right},
                             {"top", "align-top", QT_TR_NOOP("Align top edges"), A::Top},
                             {"middle", "align-middle", QT_TR_NOOP("Align vertical centres"), A::Middle},
                             {"bottom", "align-bottom", QT_TR_NOOP("Align bottom edges"), A::Bottom}};
    QList<QToolButton*> alignButtons;
    for (const Button& b : aligns) {
        QToolButton* button = row->addIcon(QStringLiteral("shape.align.%1").arg(QLatin1String(b.name)),
                                           QLatin1String(b.icon), QObject::tr(b.tip));
        QObject::connect(button, &QToolButton::clicked, row, [=, how = b.how] {
            CropShape c = project->crop();
            c.shapes = shapealign::aligned(c.shapes, project->selectedShapes(), how, frame());
            if (!(c == project->crop())) project->changeCrop(c);
        });
        alignButtons << button;
    }
    QList<QToolButton*> spreadButtons;
    for (auto [name, icon, tip, how] :
         {std::tuple{"spreadH", "distribute-h", QT_TR_NOOP("Distribute horizontally"), shapealign::Spread::Horizontal},
          std::tuple{"spreadV", "distribute-v", QT_TR_NOOP("Distribute vertically"), shapealign::Spread::Vertical}}) {
        QToolButton* button =
            row->addIcon(QStringLiteral("shape.%1").arg(QLatin1String(name)), QLatin1String(icon), QObject::tr(tip));
        QObject::connect(button, &QToolButton::clicked, row, [=, how = how] {
            CropShape c = project->crop();
            c.shapes = shapealign::distributed(c.shapes, project->selectedShapes(), how, frame());
            if (!(c == project->crop())) project->changeCrop(c);
        });
        spreadButtons << button;
    }
    auto refresh = [=] {
        const qsizetype n = project->selectedShapes().size();
        const bool toFrame = to->currentData().toInt() == 1;
        for (QToolButton* b : alignButtons)
            b->setEnabled(n >= 2 || (n == 1 && toFrame));
        for (QToolButton* b : spreadButtons)
            b->setEnabled(n >= 3);
    };
    QObject::connect(project, &Project::changed, row, [refresh](Project::Aspect a) {
        if (a == Project::Aspect::Shapes || a == Project::Aspect::Crop || a == Project::Aspect::Source) refresh();
    });
    QObject::connect(to, &QComboBox::activated, row, refresh);
    refresh();
}

} // namespace avloop
