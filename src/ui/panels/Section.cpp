#include "ui/panels/Section.h"

#include "ui/theme/IconFactory.h"
#include "ui/theme/Theme.h"

#include <QGridLayout>
#include <QLabel>
#include <QSettings>
#include <QToolButton>
#include <QTransform>
#include <QVBoxLayout>

namespace avloop {

namespace {

// The chevron turned to point right, for a closed section (rendered at 1x and 2x).
QIcon closedChevron() {
    QIcon icon;
    for (qreal dpr : {1.0, 2.0}) {
        QPixmap pm = IconFactory::pixmap(QStringLiteral("chevron"), QSize(14, 14), dpr, QIcon::Normal, QIcon::Off);
        QPixmap turned = pm.transformed(QTransform().rotate(-90));
        turned.setDevicePixelRatio(dpr);
        icon.addPixmap(turned);
    }
    return icon;
}

} // namespace

Section::Section(const QString& key, const QString& title, QWidget* parent) : QWidget(parent), key_(key) {
    setObjectName(QStringLiteral("Section.%1").arg(key));
    auto* column = new QVBoxLayout(this);
    column->setContentsMargins(0, 0, 0, 0);
    column->setSpacing(0);

    header_ = new QToolButton(this);
    header_->setObjectName(QStringLiteral("SectionHeader"));
    header_->setText(title);
    header_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    header_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    header_->setIconSize(QSize(14, 14));
    header_->setCheckable(true);
    column->addWidget(header_);

    body_ = new QWidget(this);
    grid_ = new QGridLayout(body_);
    grid_->setContentsMargins(8, 6, 8, 10);
    grid_->setHorizontalSpacing(8);
    grid_->setVerticalSpacing(6);
    grid_->setColumnMinimumWidth(0, 86);
    grid_->setColumnStretch(1, 1);
    column->addWidget(body_);

    const bool open = QSettings().value(QStringLiteral("ui/sectionOpen/%1").arg(key_), true).toBool();
    header_->setChecked(open);
    body_->setVisible(open);
    updateHeader();
    connect(header_, &QToolButton::toggled, this, [this](bool on) {
        body_->setVisible(on);
        QSettings().setValue(QStringLiteral("ui/sectionOpen/%1").arg(key_), on);
        updateHeader();
        emit toggled(on);
    });
    connect(&Theme::instance(), &Theme::changed, this, [this] { updateHeader(); });
}

void Section::addRow(const QString& label, QWidget* value) {
    const int row = grid_->rowCount();
    auto* name = new QLabel(label, body_);
    name->setProperty("quiet", true);
    grid_->addWidget(name, row, 0);
    grid_->addWidget(value, row, 1);
}

void Section::addWide(QWidget* widget) {
    grid_->addWidget(widget, grid_->rowCount(), 0, 1, 2);
}

bool Section::isOpen() const {
    return header_->isChecked();
}

void Section::setOpen(bool open) {
    header_->setChecked(open);
}

void Section::updateHeader() {
    header_->setIcon(header_->isChecked() ? IconFactory::icon(QStringLiteral("chevron")) : closedChevron());
}

} // namespace avloop
