#include "ui/canvas/DropZone.h"

#include "ui/theme/IconFactory.h"
#include "ui/theme/Theme.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QLabel>
#include <QMimeData>
#include <QPainter>
#include <QPushButton>
#include <QVBoxLayout>

namespace avloop {

DropZone::DropZone(QWidget* parent) : QWidget(parent) {
    setObjectName(QStringLiteral("DropZone"));
    setAcceptDrops(true);
    auto* column = new QVBoxLayout(this);
    column->setAlignment(Qt::AlignCenter);
    column->setSpacing(10);

    auto* icon = new QLabel(this);
    icon->setObjectName(QStringLiteral("DropZoneIcon"));
    icon->setAlignment(Qt::AlignCenter);
    icon->setPixmap(IconFactory::icon(QStringLiteral("open")).pixmap(QSize(48, 48)));
    auto* title = new QLabel(tr("Drop a video here"), this);
    title->setObjectName(QStringLiteral("DropZoneTitle"));
    title->setAlignment(Qt::AlignCenter);
    QFont big = title->font();
    big.setPointSizeF(big.pointSizeF() * 1.6);
    title->setFont(big);
    auto* hint = new QLabel(tr("Any video file — MP4, MOV, MKV, WebM, AVI and more"), this);
    hint->setObjectName(QStringLiteral("DropZoneHint"));
    hint->setAlignment(Qt::AlignCenter);
    open_ = new QPushButton(IconFactory::icon(QStringLiteral("open"), IconTone::OnAccent), tr("Open video…"), this);
    open_->setObjectName(QStringLiteral("dropzone.open"));
    open_->setProperty("primary", true);
    open_->setCursor(Qt::PointingHandCursor);

    column->addWidget(icon);
    column->addWidget(title);
    column->addWidget(hint);
    column->addSpacing(6);
    column->addWidget(open_, 0, Qt::AlignCenter);
    connect(open_, &QPushButton::clicked, this, &DropZone::openRequested);
    connect(&Theme::instance(), &Theme::changed, this,
            [icon] { icon->setPixmap(IconFactory::icon(QStringLiteral("open")).pixmap(QSize(48, 48))); });
}

QRect DropZone::zoneRect() const {
    const int w = qMin(560, width() - 80);
    const int h = qMin(320, height() - 80);
    return QRect((width() - w) / 2, (height() - h) / 2, w, h);
}

void DropZone::paintEvent(QPaintEvent*) {
    const ThemeTokens& t = Theme::instance().tokens();
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    QPen pen(hovering_ ? t.accent : t.border, 1.5, Qt::DashLine);
    p.setPen(pen);
    p.setBrush(hovering_ ? t.accentSoft : Qt::transparent);
    p.drawRoundedRect(QRectF(zoneRect()).adjusted(0.5, 0.5, -0.5, -0.5), 2, 2);
}

void DropZone::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        hovering_ = true;
        update();
        event->acceptProposedAction();
    }
}

void DropZone::dragLeaveEvent(QDragLeaveEvent*) {
    hovering_ = false;
    update();
}

void DropZone::dropEvent(QDropEvent* event) {
    hovering_ = false;
    update();
    QStringList paths;
    for (const QUrl& url : event->mimeData()->urls())
        if (url.isLocalFile()) paths << url.toLocalFile();
    if (!paths.isEmpty()) {
        event->acceptProposedAction();
        emit filesDropped(paths);
    }
}

} // namespace avloop
