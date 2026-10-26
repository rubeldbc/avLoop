#include "ui/export/BatchWindow.h"

#include "export/ExportQueue.h"
#include "export/FormatProfiles.h"
#include "export/SizeEstimator.h"

#include <QDir>
#include <QDropEvent>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <algorithm>
#include <functional>

namespace avloop {

namespace {

using exporting::ExportQueue;

// The list, its rows dragged to a new place; `dropped` then reads the new order.
class QueueList : public QTreeWidget {
public:
    using QTreeWidget::QTreeWidget;
    std::function<void()> dropped;

protected:
    void dropEvent(QDropEvent* event) override {
        QTreeWidget::dropEvent(event);
        if (dropped) dropped();
    }
};

enum Column { Name, Format, Size, State, Progress };

QString stateText(const ExportQueue::Entry& e) {
    switch (e.state) {
    case ExportQueue::State::Running:
        return BatchWindow::tr("Running");
    case ExportQueue::State::Done:
        return BatchWindow::tr("Done · %1").arg(exporting::SizeEstimator::sizeText(e.bytes));
    case ExportQueue::State::Failed:
        return BatchWindow::tr("Failed");
    case ExportQueue::State::Pending:
        break;
    }
    return BatchWindow::tr("Pending");
}

QString progressText(const ExportQueue::Entry& e) {
    return e.state == ExportQueue::State::Running || e.state == ExportQueue::State::Done
               ? QStringLiteral("%1 %").arg(int(e.progress * 100))
               : QString();
}

} // namespace

BatchWindow::BatchWindow(ExportQueue* queue, QWidget* parent) : QWidget(parent, Qt::Window), queue_(queue) {
    setObjectName(QStringLiteral("BatchWindow"));
    setWindowTitle(tr("AvLoop — Batch Queue"));
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_StyledBackground);
    resize(720, 360);
    auto* column = new QVBoxLayout(this);
    column->setContentsMargins(10, 10, 10, 10);
    column->setSpacing(8);

    note_ = new QLabel(this);
    note_->setObjectName(QStringLiteral("batch.note"));
    note_->setProperty("quiet", true);
    note_->setWordWrap(true);
    column->addWidget(note_);

    auto* list = new QueueList(this);
    list_ = list;
    list_->setObjectName(QStringLiteral("batch.list"));
    list_->setHeaderLabels({tr("Name"), tr("Format"), tr("Size"), tr("State"), tr("Progress")});
    list_->setRootIsDecorated(false);
    list_->setUniformRowHeights(true);
    list_->setSelectionMode(QAbstractItemView::SingleSelection);
    list_->setDragDropMode(QAbstractItemView::InternalMove);
    list_->setDefaultDropAction(Qt::MoveAction);
    list_->header()->setStretchLastSection(false);
    list_->header()->setSectionResizeMode(Name, QHeaderView::Stretch);
    for (int c : {Format, Size, State, Progress})
        list_->header()->setSectionResizeMode(c, QHeaderView::ResizeToContents);
    list->dropped = [this] {
        QStringList ids;
        for (int i = 0; i < list_->topLevelItemCount(); ++i)
            ids << list_->topLevelItem(i)->data(Name, Qt::UserRole).toString();
        queue_->reorder(ids);
    };
    column->addWidget(list_, 1);

    auto* buttons = new QHBoxLayout;
    auto button = [this, buttons](const QString& name, const QString& text) {
        auto* b = new QPushButton(text, this);
        b->setObjectName(name);
        buttons->addWidget(b);
        return b;
    };
    add_ = button(QStringLiteral("batch.add"), tr("Add current"));
    add_->setToolTip(tr("Add the open project (Ctrl+Shift+E)"));
    remove_ = button(QStringLiteral("batch.remove"), tr("Remove"));
    clear_ = button(QStringLiteral("batch.clear"), tr("Clear done"));
    buttons->addStretch(1);
    pause_ = button(QStringLiteral("batch.pause"), tr("Pause"));
    pause_->setToolTip(tr("Stops the entry under way; it starts over next time"));
    start_ = button(QStringLiteral("batch.start"), tr("Start"));
    start_->setProperty("primary", true);
    column->addLayout(buttons);

    connect(add_, &QPushButton::clicked, this, &BatchWindow::addRequested);
    connect(start_, &QPushButton::clicked, queue_, &ExportQueue::start);
    connect(pause_, &QPushButton::clicked, queue_, &ExportQueue::pause);
    connect(clear_, &QPushButton::clicked, queue_, &ExportQueue::clearDone);
    connect(remove_, &QPushButton::clicked, this, [this] {
        if (QTreeWidgetItem* item = list_->currentItem()) queue_->remove(item->data(Name, Qt::UserRole).toString());
    });
    connect(list_, &QTreeWidget::currentItemChanged, this, &BatchWindow::refreshButtons);
    connect(queue_, &ExportQueue::changed, this, &BatchWindow::rebuild);
    connect(queue_, &ExportQueue::progressed, this, &BatchWindow::showProgress);
    connect(queue_, &ExportQueue::startedChanged, this, &BatchWindow::refreshButtons);
    rebuild();
}

void BatchWindow::rebuild() {
    const QString current =
        list_->currentItem() ? list_->currentItem()->data(Name, Qt::UserRole).toString() : QString();
    list_->clear();
    for (const ExportQueue::Entry& e : queue_->entries()) {
        auto* item = new QTreeWidgetItem(list_);
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled); // nothing dropped onto it
        item->setData(Name, Qt::UserRole, e.id);
        item->setText(Name, QFileInfo(e.output).fileName());
        item->setToolTip(Name, QDir::toNativeSeparators(e.output));
        item->setText(Format, exporting::profileOf(formatFromId(e.format)).name);
        item->setText(Size, QStringLiteral("%1×%2").arg(e.size.width()).arg(e.size.height()));
        item->setText(State, stateText(e));
        if (!e.reason.isEmpty()) item->setToolTip(State, e.reason);
        item->setText(Progress, progressText(e));
        if (e.id == current) list_->setCurrentItem(item);
    }
    note_->setText(queue_->isSaved() ? tr("Entries run one after another, in this order. Drag one to move it.")
                                     : tr("Another AvLoop holds the queue: this one is not saved."));
    refreshButtons();
}

void BatchWindow::showProgress(int index) {
    if (QTreeWidgetItem* item = list_->topLevelItem(index))
        item->setText(Progress, progressText(queue_->entries().at(index)));
}

void BatchWindow::refreshButtons() {
    const QList<ExportQueue::Entry>& entries = queue_->entries();
    const bool pending = std::any_of(entries.begin(), entries.end(), [](const ExportQueue::Entry& e) {
        return e.state == ExportQueue::State::Pending;
    });
    const bool done = std::any_of(entries.begin(), entries.end(),
                                  [](const ExportQueue::Entry& e) { return e.state == ExportQueue::State::Done; });
    start_->setEnabled(!queue_->isStarted() && pending && canStart_);
    pause_->setEnabled(queue_->isStarted());
    remove_->setEnabled(list_->currentItem() != nullptr);
    clear_->setEnabled(done);
}

void BatchWindow::setCanAdd(bool can, const QString& reason) {
    add_->setEnabled(can);
    add_->setToolTip(can ? tr("Add the open project (Ctrl+Shift+E)") : reason);
}

void BatchWindow::setCanStart(bool can, const QString& reason) {
    canStart_ = can;
    start_->setToolTip(can ? QString() : reason);
    refreshButtons();
}

QString BatchWindow::states() const {
    QStringList s;
    for (const ExportQueue::Entry& e : queue_->entries())
        s << ExportQueue::stateId(e.state);
    return s.join(QLatin1Char(','));
}

} // namespace avloop
