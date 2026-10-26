// The side panel's Loop section (spec §8.5) and the Output section's frame rate (§8.3, §10.2). Loop: Mode, Crossfade
// in seconds and frames (linked), Curve, Turn, Speed, FPS method, the slow-motion hint and the summary
// "Loop 4.97 s · 149 frames @ 30 fps"; a value the loop clamps flashes yellow for 600 ms. Every edit is one Undo
// step (typing or the wheel in one field merges).
#include "core/LoopMap.h"
#include "core/Project.h"
#include "core/Timecode.h"
#include "ui/WalkRunner.h"
#include "ui/panels/Section.h"
#include "ui/panels/SidePanel.h"
#include "ui/widgets/StepSpinBox.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QStyle>
#include <QTimer>

namespace avloop {

namespace {

using Mode = LoopSettings::Mode;

// A clamped value: the field's border turns yellow for 600 ms.
void flash(QWidget* w) {
    w->setProperty("flash", true);
    w->style()->unpolish(w);
    w->style()->polish(w);
    QTimer::singleShot(600, w, [w] {
        w->setProperty("flash", false);
        w->style()->unpolish(w);
        w->style()->polish(w);
    });
}

QLabel* quiet(const QString& text, QWidget* parent) {
    auto* l = new QLabel(text, parent);
    l->setProperty("quiet", true);
    l->setWordWrap(true);
    return l;
}

} // namespace

void SidePanel::bindLoop(Project* project) {
    Section* s = loop_;
    QWidget* body = s->body();
    QLabel* hint = quiet(tr("Open a video to set its loop."), body);
    s->addWide(hint);
    auto* mode = new QComboBox(body);
    mode->setObjectName(QStringLiteral("panel.loop.mode"));
    mode->addItem(tr("Simple"), int(Mode::Simple));
    mode->addItem(tr("Crossfade"), int(Mode::Crossfade));
    mode->addItem(tr("Ping-pong"), int(Mode::PingPong));
    mode->addItem(tr("Ping-pong + Smooth turn"), int(Mode::PingPongSmooth));
    s->addRow(tr("Mode"), mode);
    auto* fadeRow = new QWidget(body);
    auto* fadeLayout = new QHBoxLayout(fadeRow);
    fadeLayout->setContentsMargins(0, 0, 0, 0);
    fadeLayout->setSpacing(4);
    auto* fade = new StepSpinBox(QStringLiteral("panel.loop.fade"), 0, 600, 0.1, 2, fadeRow);
    fade->setSuffix(QStringLiteral(" s"));
    auto* fadeFrames = new StepSpinBox(QStringLiteral("panel.loop.fadeFrames"), 1, 36000, 1, 0, fadeRow);
    fadeFrames->setSuffix(QStringLiteral(" f"));
    for (StepSpinBox* half : {fade, fadeFrames}) {
        half->setMinimumWidth(44);
        fadeLayout->addWidget(half);
    }
    s->addRow(tr("Crossfade"), fadeRow);
    auto* curve = new QComboBox(body);
    curve->setObjectName(QStringLiteral("panel.loop.curve"));
    curve->addItem(tr("Smooth"), int(LoopSettings::Curve::Smooth));
    curve->addItem(tr("Linear"), int(LoopSettings::Curve::Linear));
    s->addRow(tr("Curve"), curve);
    auto* turn = new StepSpinBox(QStringLiteral("panel.loop.turn"), 0, 600, 0.05, 2, body);
    turn->setSuffix(QStringLiteral(" s"));
    s->addRow(tr("Turn"), turn);
    auto* speed = new StepSpinBox(QStringLiteral("panel.loop.speed"), LoopSettings::kMinSpeed, LoopSettings::kMaxSpeed,
                                  0.05, 2, body);
    speed->setSuffix(QStringLiteral("×"));
    s->addRow(tr("Speed"), speed);
    auto* method = new QComboBox(body);
    method->setObjectName(QStringLiteral("panel.loop.method"));
    method->addItem(tr("Nearest"), int(LoopSettings::Method::Nearest));
    method->addItem(tr("Frame Blend"), int(LoopSettings::Method::FrameBlend));
    method->addItem(tr("Motion Interpolation"), int(LoopSettings::Method::Motion));
    s->addRow(tr("FPS method"), method);
    QLabel* slow = quiet(tr("Motion Interpolation gives smoother slow motion"), body);
    slow->setObjectName(QStringLiteral("panel.loop.slowHint"));
    s->addWide(slow);
    QLabel* summary = quiet(QString(), body);
    summary->setObjectName(QStringLiteral("panel.loop.summary"));
    summary->setProperty("mono", true);
    s->addWide(summary);
    // The rows a mode does not use are hidden with their labels.
    const QList<QWidget*> fadeWidgets = s->row(fadeRow) + s->row(curve);
    const QList<QWidget*> turnWidgets = s->row(turn);

    auto refresh = [=] {
        const LoopSettings& l = project->loop();
        const Rational fps = project->outputFps();
        const TimeRange r = project->range();
        const LoopPlan plan = LoopPlan::of(l, r.in, r.out, fps);
        const bool open = project->source().isValid();
        hint->setVisible(!open);
        for (QWidget* w : body->findChildren<QWidget*>(Qt::FindDirectChildrenOnly))
            if (w != hint) w->setEnabled(open);
        {
            const QSignalBlocker b1(mode), b2(curve), b3(method);
            mode->setCurrentIndex(mode->findData(int(l.mode)));
            curve->setCurrentIndex(curve->findData(int(l.curve)));
            method->setCurrentIndex(method->findData(int(l.method)));
        }
        fade->setValueQuietly(loopmap::secondsOf(plan.fade, fps));
        fadeFrames->setValueQuietly(plan.fade);
        turn->setValueQuietly(loopmap::secondsOf(plan.turn, fps));
        speed->setValueQuietly(l.speed);
        for (QWidget* w : fadeWidgets)
            w->setVisible(l.mode == Mode::Crossfade);
        for (QWidget* w : turnWidgets)
            w->setVisible(l.mode == Mode::PingPongSmooth);
        slow->setVisible(open && l.speed < 1.0 - 1e-9 && l.method == LoopSettings::Method::Nearest);
        summary->setText(open ? tr("Loop %1 s · %2 frames @ %3 fps")
                                    .arg(loopmap::secondsOf(plan.length, fps), 0, 'f', 2)
                                    .arg(plan.length)
                                    .arg(timecode::rateText(fps))
                              : QString());
    };
    connect(project, &Project::changed, this, [refresh](Project::Aspect a) {
        if (a != Project::Aspect::Time) refresh(); // the playhead changes nothing here
    });

    auto edit = [project](const QString& field, auto change) {
        LoopSettings l = project->loop();
        change(l);
        project->changeLoop(l, QStringLiteral("field:loop.%1").arg(field));
    };
    // F typed beyond ⌊D/2⌋ (or T beyond ⌊(D − 1)/2⌋) is clamped by the loop: the field says so.
    auto checkClamp = [project](QWidget* field, bool turnField) {
        const TimeRange r = project->range();
        const LoopPlan plan = LoopPlan::of(project->loop(), r.in, r.out, project->outputFps());
        if (turnField ? plan.turnClamped : plan.fadeClamped) flash(field);
    };
    connect(mode, &QComboBox::activated, this, [=] {
        edit(QStringLiteral("mode"), [&](LoopSettings& l) { l.mode = Mode(mode->currentData().toInt()); });
    });
    connect(curve, &QComboBox::activated, this, [=] {
        edit(QStringLiteral("curve"),
             [&](LoopSettings& l) { l.curve = LoopSettings::Curve(curve->currentData().toInt()); });
    });
    connect(method, &QComboBox::activated, this, [=] {
        edit(QStringLiteral("method"),
             [&](LoopSettings& l) { l.method = LoopSettings::Method(method->currentData().toInt()); });
    });
    connect(fade, &StepSpinBox::valueChanged, this, [=](double v) {
        edit(QStringLiteral("fade"), [v](LoopSettings& l) { l.fade = v; });
        checkClamp(fade, false);
    });
    connect(fadeFrames, &StepSpinBox::valueChanged, this, [=](double v) {
        const double seconds = loopmap::secondsOf(int(v), project->outputFps());
        edit(QStringLiteral("fade"), [seconds](LoopSettings& l) { l.fade = seconds; });
        checkClamp(fadeFrames, false);
    });
    connect(turn, &StepSpinBox::valueChanged, this, [=](double v) {
        edit(QStringLiteral("turn"), [v](LoopSettings& l) { l.turn = v; });
        checkClamp(turn, true);
    });
    connect(speed, &StepSpinBox::valueChanged, this,
            [=](double v) { edit(QStringLiteral("speed"), [v](LoopSettings& l) { l.speed = v; }); });

    WalkProbes::add(QStringLiteral("panel.loop.summary"), [summary] { return summary->text(); });
    WalkProbes::add(QStringLiteral("loop.plan"), [project] {
        const TimeRange r = project->range();
        const LoopPlan p = LoopPlan::of(project->loop(), r.in, r.out, project->outputFps());
        return QStringLiteral("D%1 F%2 T%3 L%4").arg(p.segment).arg(p.fade).arg(p.turn).arg(p.length);
    });
    WalkProbes::add(QStringLiteral("loop.mode"), [project] { return LoopSettings::modeId(project->loop().mode); });
    WalkProbes::add(QStringLiteral("loop.speed"), [project] { return project->loop().speed; });
    WalkProbes::add(QStringLiteral("panel.loop.slowHint"), [slow] { return !slow->isHidden(); });
    WalkProbes::add(QStringLiteral("panel.loop.fadeFrames"), [fadeFrames] { return fadeFrames->value(); });
    WalkProbes::add(QStringLiteral("panel.loop.fadeFlash"), [fade, fadeFrames] {
        return fade->property("flash").toBool() || fadeFrames->property("flash").toBool();
    });
    refresh();
}

} // namespace avloop
