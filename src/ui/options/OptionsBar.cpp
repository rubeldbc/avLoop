#include "ui/options/OptionsBar.h"

#include "ui/WalkRunner.h"

#include "ui/theme/IconFactory.h"
#include "ui/widgets/StepSpinBox.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPointer>
#include <QPushButton>
#include <QToolButton>

namespace avloop {

OptionsRow::OptionsRow(const QString& tool, const QString& title, QWidget* parent) : QWidget(parent), tool_(tool) {
    setObjectName(QStringLiteral("options.%1").arg(tool));
    row_ = new QHBoxLayout(this);
    row_->setContentsMargins(0, 0, 0, 0);
    row_->setSpacing(6);
    auto* name = new QLabel(title, this);
    name->setObjectName(QStringLiteral("OptionsTitle"));
    QFont bold = name->font();
    bold.setWeight(QFont::DemiBold);
    name->setFont(bold);
    row_->addWidget(name);
    row_->addSpacing(4);
    row_->addStretch(1);
}

StepSpinBox* OptionsRow::addNumber(const QString& name, const QString& label, double min, double max, double step,
                                   int decimals, const QString& suffix) {
    if (!label.isEmpty()) {
        auto* l = new QLabel(label, this);
        l->setProperty("quiet", true);
        row_->insertWidget(row_->count() - 1, l);
    }
    auto* spin = new StepSpinBox(QStringLiteral("options.%1").arg(name), min, max, step, decimals, this);
    spin->setSuffix(suffix);
    spin->setFixedWidth(70);
    row_->insertWidget(row_->count() - 1, spin);
    return spin;
}

QToolButton* OptionsRow::addIcon(const QString& name, const QString& icon, const QString& tip, bool checkable) {
    auto* b = new QToolButton(this);
    b->setObjectName(QStringLiteral("options.%1").arg(name));
    b->setIcon(IconFactory::icon(icon));
    b->setIconSize(QSize(18, 18));
    b->setFixedSize(26, 26);
    b->setToolTip(tip);
    b->setAutoRaise(true);
    b->setCheckable(checkable);
    row_->insertWidget(row_->count() - 1, b);
    return b;
}

QPushButton* OptionsRow::addButton(const QString& name, const QString& text) {
    auto* b = new QPushButton(text, this);
    b->setObjectName(QStringLiteral("options.%1").arg(name));
    row_->insertWidget(row_->count() - 1, b);
    return b;
}

QLabel* OptionsRow::addLabel(const QString& text) {
    auto* l = new QLabel(text, this);
    l->setProperty("quiet", true);
    row_->insertWidget(row_->count() - 1, l);
    return l;
}

void OptionsRow::add(QWidget* widget, const QString& label) {
    if (!label.isEmpty()) addLabel(label);
    widget->setParent(this);
    row_->insertWidget(row_->count() - 1, widget);
}

void OptionsRow::addSeparator() {
    auto* line = new QFrame(this);
    line->setObjectName(QStringLiteral("TopBarSeparator"));
    line->setAttribute(Qt::WA_StyledBackground);
    line->setFixedSize(1, 18);
    row_->insertWidget(row_->count() - 1, line);
}

void probeValue(const QString& name, StepSpinBox* box) {
    WalkProbes::add(name, [p = QPointer<StepSpinBox>(box)] { return p ? QVariant(p->value()) : QVariant(); });
}

} // namespace avloop
