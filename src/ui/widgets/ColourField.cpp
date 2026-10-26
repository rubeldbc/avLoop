#include "ui/widgets/ColourField.h"

#include "ui/theme/IconFactory.h"

#include <QColorDialog>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPixmap>
#include <QRegularExpressionValidator>
#include <QToolButton>

namespace avloop {

ColourField::ColourField(const QString& name, const QString& title, QWidget* parent) : QWidget(parent), title_(title) {
    setObjectName(name + QStringLiteral(".row"));
    auto* row = new QHBoxLayout(this);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(4);
    hex_ = new QLineEdit(this);
    hex_->setObjectName(name);
    hex_->setProperty("mono", true);
    hex_->setValidator(new QRegularExpressionValidator(QRegularExpression(QStringLiteral("#[0-9A-Fa-f]{6}")), hex_));
    swatch_ = new QToolButton(this);
    swatch_->setObjectName(name + QStringLiteral(".choose"));
    swatch_->setToolTip(tr("Choose the colour"));
    row->addWidget(hex_, 1);
    row->addWidget(swatch_);
    connect(hex_, &QLineEdit::editingFinished, this, [this] { take(QColor(hex_->text())); });
    connect(swatch_, &QToolButton::clicked, this, [this] { take(QColorDialog::getColor(colour_, this, title_)); });
}

void ColourField::setPicker(Picker pick) {
    picker_ = std::move(pick);
    if (pick_ || !picker_) return;
    pick_ = new QToolButton(this);
    pick_->setObjectName(hex_->objectName() + QStringLiteral(".pick"));
    pick_->setIcon(IconFactory::icon(QStringLiteral("eyedropper")));
    pick_->setIconSize(QSize(14, 14));
    pick_->setToolTip(tr("Pick the colour from the video: click it on the canvas (Esc cancels)"));
    layout()->addWidget(pick_);
    connect(pick_, &QToolButton::clicked, this, [this] { picker_([this](const QColor& c) { take(c); }); });
}

void ColourField::setColourQuietly(const QColor& colour) {
    colour_ = colour;
    display(colour);
}

void ColourField::take(const QColor& colour) {
    if (!colour.isValid()) {
        display(colour_); // a dialog cancelled, a hex half typed: as it was
        return;
    }
    const QColor rgb = colour.toRgb();
    display(rgb);
    if (rgb == colour_) return;
    colour_ = rgb;
    emit colourChosen(rgb);
}

void ColourField::display(const QColor& colour) {
    if (!hex_->hasFocus()) hex_->setText(colour.name());
    QPixmap swatch(14, 14);
    swatch.fill(colour);
    swatch_->setIcon(QIcon(swatch));
}

} // namespace avloop
