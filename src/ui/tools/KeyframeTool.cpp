// The Keyframe tool (spec §6, §6.4): K adds a keyframe at the playhead with the transform shown, or updates the one
// there (MainWindow.cpp's key). The options: Add, Delete, ◀ / ▶ (the previous / next keyframe, as [ and ]) and the
// Interpolation — Linear or Ease — of the keyframe at the playhead; what the row says: that keyframe and how many.
// On the canvas the tool does nothing (the Selection moves and scales, writing into the keyframes); Delete removes
// the keyframe at the playhead.
#include "core/Project.h"
#include "core/Timecode.h"
#include "ui/WalkRunner.h"
#include "ui/options/OptionsBar.h"
#include "ui/tools/Tool.h"

#include <QComboBox>
#include <QKeyEvent>
#include <QLabel>
#include <QPointer>
#include <QToolButton>

#include <functional>

namespace avloop {

namespace {

class KeyframeTool : public Tool {
public:
    KeyframeTool(Canvas* canvas, Project* project, std::function<void(double)> seek)
        : Tool(QStringLiteral("keyframe"), canvas, project), seek_(std::move(seek)) {}

    bool keyPress(QKeyEvent* event) override {
        if ((event->key() != Qt::Key_Delete && event->key() != Qt::Key_Backspace) || project_->keyframeAtTime() < 0)
            return false;
        project_->deleteKeyframe(project_->keyframeAtTime());
        return true;
    }
    bool wantsKey(const QKeyEvent* event) const override {
        return (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) && project_->keyframeAtTime() >= 0;
    }

    QWidget* createOptions(QWidget* parent) override {
        auto* row = new OptionsRow(id_, QObject::tr("Keyframe"), parent);
        QToolButton* add = row->addIcon(QStringLiteral("keyframe.add"), QStringLiteral("tool-keyframe"),
                                        QObject::tr("Add or update the keyframe at the playhead (K)"));
        QToolButton* remove = row->addIcon(QStringLiteral("keyframe.delete"), QStringLiteral("kf-delete"),
                                           QObject::tr("Delete the keyframe at the playhead (Delete)"));
        QToolButton* prev = row->addIcon(QStringLiteral("keyframe.prev"), QStringLiteral("kf-prev"),
                                         QObject::tr("Previous keyframe ([)"));
        QToolButton* next =
            row->addIcon(QStringLiteral("keyframe.next"), QStringLiteral("kf-next"), QObject::tr("Next keyframe (])"));
        row->addSeparator();
        auto* interp = new QComboBox(row);
        interp->setObjectName(QStringLiteral("options.keyframe.interp"));
        interp->addItem(QObject::tr("Linear"), false);
        interp->addItem(QObject::tr("Ease"), true);
        interp->setToolTip(QObject::tr("How the video goes on from this keyframe to the next"));
        row->add(interp, QObject::tr("Interpolation"));
        QLabel* status = row->addLabel(QString());
        status->setObjectName(QStringLiteral("options.keyframe.status"));
        status->setProperty("quiet", true);

        auto refresh = [=, this] {
            const Keyframes& keys = project_->keyframes();
            const int at = project_->keyframeAtTime();
            const bool open = project_->source().isValid();
            add->setEnabled(open);
            remove->setEnabled(at >= 0);
            prev->setEnabled(open &&
                             keyframes::previous(keys, project_->time(), project_->keyTolerance()) < project_->time());
            next->setEnabled(open &&
                             keyframes::next(keys, project_->time(), project_->keyTolerance()) > project_->time());
            interp->setEnabled(at >= 0);
            const QSignalBlocker block(interp);
            interp->setCurrentIndex(at >= 0 && keys[at].ease ? 1 : 0);
            status->setText(keys.isEmpty() ? QObject::tr("No keyframes: the video stays where it is")
                            : at >= 0      ? QObject::tr("Keyframe %1 of %2 at %3")
                                            .arg(at + 1)
                                            .arg(keys.size())
                                            .arg(timecode::format(keys[at].t, project_->outputFps()))
                                      : QObject::tr("%n keyframe(s); none at the playhead", "", int(keys.size())));
        };
        QObject::connect(project_, &Project::changed, row, [refresh](Project::Aspect) { refresh(); });
        QObject::connect(add, &QToolButton::clicked, row, [this] { project_->addKeyframe(); });
        QObject::connect(remove, &QToolButton::clicked, row,
                         [this] { project_->deleteKeyframe(project_->keyframeAtTime()); });
        QObject::connect(prev, &QToolButton::clicked, row, [this] {
            seek_(keyframes::previous(project_->keyframes(), project_->time(), project_->keyTolerance()));
        });
        QObject::connect(next, &QToolButton::clicked, row, [this] {
            seek_(keyframes::next(project_->keyframes(), project_->time(), project_->keyTolerance()));
        });
        QObject::connect(interp, &QComboBox::activated, row, [this, interp] {
            project_->setKeyframeEase(project_->keyframeAtTime(), interp->currentData().toBool());
        });
        WalkProbes::add(QStringLiteral("options.keyframe.status"),
                        [p = QPointer<QLabel>(status)] { return p ? p->text() : QString(); });
        refresh();
        return row;
    }

private:
    std::function<void(double)> seek_;
};

} // namespace

Tool* createKeyframeTool(Canvas* canvas, Project* project, std::function<void(double)> seek) {
    return new KeyframeTool(canvas, project, std::move(seek));
}

} // namespace avloop
