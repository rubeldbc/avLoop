// The Zoom tool's options (spec §6's table): Fit, 100%, 200% and the zoom in percent. The view only — never in Undo.
#include "ui/canvas/Canvas.h"
#include "ui/options/OptionsBar.h"
#include "ui/widgets/StepSpinBox.h"

#include <QPushButton>
#include <QToolButton>

namespace avloop {

QWidget* createZoomOptions(Canvas* canvas, QWidget* parent) {
    auto* row = new OptionsRow(QStringLiteral("zoom"), QObject::tr("Zoom"), parent);
    QToolButton* fit = row->addIcon(QStringLiteral("zoomFit"), QStringLiteral("zoom-fit"), QObject::tr("Fit (Ctrl+0)"));
    QToolButton* actual =
        row->addIcon(QStringLiteral("zoom100"), QStringLiteral("zoom-100"), QObject::tr("100% (Ctrl+1)"));
    QPushButton* twice = row->addButton(QStringLiteral("zoom200"), QStringLiteral("200%"));
    StepSpinBox* percent =
        row->addNumber(QStringLiteral("zoomPercent"), QObject::tr("Zoom"), 5, 800, 1, 0, QStringLiteral("%"));
    QObject::connect(fit, &QToolButton::clicked, canvas, &Canvas::fitView);
    QObject::connect(actual, &QToolButton::clicked, canvas, [canvas] { canvas->setZoom(1.0); });
    QObject::connect(twice, &QPushButton::clicked, canvas, [canvas] { canvas->setZoom(2.0); });
    QObject::connect(percent, &StepSpinBox::valueChanged, canvas, [canvas](double v) { canvas->setZoom(v / 100.0); });
    QObject::connect(canvas, &Canvas::viewChanged, percent,
                     [canvas, percent] { percent->setValueQuietly(canvas->zoom() * 100.0); });
    percent->setValueQuietly(canvas->zoom() * 100.0);
    return row;
}

} // namespace avloop
