#include "ui/timeline/Transport.h"

#include "ui/theme/IconFactory.h"
#include "ui/widgets/TimeEdit.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>

namespace avloop {

Transport::Transport(QWidget* parent) : QWidget(parent) {
    setObjectName(QStringLiteral("TransportRow"));
    setAttribute(Qt::WA_StyledBackground);
    setFixedHeight(30);
    auto* row = new QHBoxLayout(this);
    row->setContentsMargins(6, 0, 6, 0);
    row->setSpacing(2);

    goIn_ = button(QStringLiteral("transport.goIn"), QStringLiteral("go-in"), tr("Go to In (Home)"));
    prev_ = button(QStringLiteral("transport.prev"), QStringLiteral("frame-prev"), tr("Previous frame (←)"));
    play_ = button(QStringLiteral("transport.play"), QStringLiteral("play"), tr("Play/Pause (Space)"));
    next_ = button(QStringLiteral("transport.next"), QStringLiteral("frame-next"), tr("Next frame (→)"));
    goOut_ = button(QStringLiteral("transport.goOut"), QStringLiteral("go-out"), tr("Go to Out (End)"));
    loop_ = button(QStringLiteral("transport.loop"), QStringLiteral("loop-on"), tr("Loop playback (L)"));
    for (QToolButton* b : {goIn_, prev_, play_, next_, goOut_, loop_})
        row->addWidget(b);

    now_ = new TimeEdit(QStringLiteral("transport.time"), this);
    now_->setProperty("timeNow", true);
    total_ = new QLabel(QStringLiteral("/ 00:00:00.00"), this);
    total_->setObjectName(QStringLiteral("TimeTotal"));
    total_->setProperty("mono", true);
    range_ = new QLabel(this);
    range_->setObjectName(QStringLiteral("RangeReadout"));
    range_->setProperty("quiet", true);
    range_->setProperty("mono", true);
    row->addSpacing(10);
    row->addWidget(now_);
    row->addSpacing(4);
    row->addWidget(total_);
    row->addSpacing(14);
    row->addWidget(range_);
    row->addStretch(1);
    setEnabled(false);
}

QToolButton* Transport::button(const QString& name, const QString& icon, const QString& tip) {
    auto* b = new QToolButton(this);
    b->setObjectName(name);
    b->setIcon(IconFactory::icon(icon));
    b->setIconSize(QSize(18, 18));
    b->setFixedSize(26, 26);
    b->setToolTip(tip);
    b->setAutoRaise(true);
    return b;
}

void Transport::setTotal(const QString& total) {
    total_->setText(QStringLiteral("/ %1").arg(total));
}

void Transport::setRangeText(const QString& text) {
    range_->setText(text);
}

void Transport::setPlaying(bool playing) {
    play_->setIcon(IconFactory::icon(playing ? QStringLiteral("pause") : QStringLiteral("play")));
}

void Transport::setLoopOn(bool on) {
    loop_->setIcon(IconFactory::icon(on ? QStringLiteral("loop-on") : QStringLiteral("loop-off")));
}

} // namespace avloop
