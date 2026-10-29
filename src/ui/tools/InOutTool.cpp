// In (I) and Out (O) as tools (spec §6.4): on the canvas they do nothing; while one is active a click on the timeline
// sets In or Out there (Timeline::setClickTool). The keys I and O set them at the playhead in any tool. Options: the
// In time; the Out time and the range's length. A time typed and Enter sets it by the same rules.
#include "core/Project.h"
#include "core/Timecode.h"
#include "ui/WalkRunner.h"
#include "ui/options/OptionsBar.h"
#include "ui/tools/Tool.h"
#include "ui/widgets/TimeEdit.h"

#include <QLabel>
#include <QPointer>

namespace avloop {

namespace {

class InOutTool : public Tool {
public:
    InOutTool(bool out, Canvas* canvas, Project* project)
        : Tool(out ? QStringLiteral("out") : QStringLiteral("in"), canvas, project), out_(out) {}

    QWidget* createOptions(QWidget* parent) override {
        auto* row = new OptionsRow(id_, out_ ? QObject::tr("Out") : QObject::tr("In"), parent);
        auto* time = new TimeEdit(QStringLiteral("options.%1.time").arg(id_), row);
        row->add(time, out_ ? QObject::tr("Out") : QObject::tr("In"));
        QLabel* length = out_ ? row->addLabel(QString()) : nullptr;
        if (length) length->setProperty("mono", true);
        auto refresh = [=, this] {
            const TimeRange r = project_->range();
            time->setFps(project_->outputFps());
            time->setSeconds(out_ ? r.out : r.in);
            if (length)
                length->setText(QObject::tr("Length %1 · %2 frames")
                                    .arg(timecode::format(r.length(), project_->outputFps()))
                                    .arg(timecode::nearestFrame(r.length(), project_->outputFps())));
            row->setEnabled(project_->source().isValid());
        };
        QObject::connect(project_, &Project::changed, row, [refresh](Project::Aspect) { refresh(); });
        QObject::connect(time, &TimeEdit::timeEntered, row, [this](double t) {
            const double d = project_->length();
            const Rational fps = project_->outputFps();
            project_->changeRange(out_ ? timerange::withOut(project_->range(), t, d, fps)
                                       : timerange::withIn(project_->range(), t, d, fps));
        });
        if (length)
            WalkProbes::add(QStringLiteral("options.out.length"),
                            [p = QPointer<QLabel>(length)] { return p ? p->text() : QString(); });
        refresh();
        return row;
    }

private:
    bool out_;
};

} // namespace

Tool* createInOutTool(bool out, Canvas* canvas, Project* project) {
    return new InOutTool(out, canvas, project);
}

} // namespace avloop
