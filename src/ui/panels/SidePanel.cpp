#include "ui/panels/SidePanel.h"

#include "ui/panels/Section.h"
#include "ui/theme/IconFactory.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QToolButton>
#include <QVBoxLayout>

namespace avloop {

namespace {

QToolButton* iconButton(const QString& name, const QString& icon, const QString& tip, QWidget* parent) {
    auto* b = new QToolButton(parent);
    b->setObjectName(name);
    b->setIcon(IconFactory::icon(icon));
    b->setIconSize(QSize(16, 16));
    b->setToolTip(tip);
    b->setAutoRaise(true);
    return b;
}

} // namespace

SidePanel::SidePanel(QWidget* parent) : QWidget(parent) {
    setObjectName(QStringLiteral("SidePanel"));
    setAttribute(Qt::WA_StyledBackground);
    auto* column = new QVBoxLayout(this);
    column->setContentsMargins(0, 0, 0, 0);
    column->setSpacing(0);

    auto* presetRow = new QWidget(this);
    presetRow->setObjectName(QStringLiteral("PresetRow"));
    presetRow->setAttribute(Qt::WA_StyledBackground);
    auto* row = new QHBoxLayout(presetRow);
    row->setContentsMargins(8, 8, 8, 8);
    row->setSpacing(4);
    presets_ = new QComboBox(presetRow);
    presets_->setObjectName(QStringLiteral("preset.box"));
    presets_->addItem(tr("— Preset —"));
    presets_->setEnabled(false);
    row->addWidget(presets_, 1);
    savePreset_ = iconButton(QStringLiteral("preset.save"), QStringLiteral("check"), tr("Save preset"), presetRow);
    deletePreset_ =
        iconButton(QStringLiteral("preset.delete"), QStringLiteral("close"), tr("Delete preset"), presetRow);
    savePreset_->setEnabled(false);
    deletePreset_->setEnabled(false);
    row->addWidget(savePreset_);
    row->addWidget(deletePreset_);
    column->addWidget(presetRow);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto* body = new QWidget(scroll);
    body->setObjectName(QStringLiteral("SidePanelBody"));
    auto* sections = new QVBoxLayout(body);
    sections->setContentsMargins(0, 0, 0, 0);
    sections->setSpacing(0);
    crop_ = new Section(QStringLiteral("crop"), tr("Crop"), body);
    loop_ = new Section(QStringLiteral("loop"), tr("Loop"), body);
    output_ = new Section(QStringLiteral("output"), tr("Output"), body);
    quality_ = new Section(QStringLiteral("quality"), tr("Quality"), body);
    for (Section* s : {crop_, loop_, output_, quality_})
        sections->addWidget(s);
    sections->addStretch(1);
    scroll->setWidget(body);
    column->addWidget(scroll, 1);
}

} // namespace avloop
