// The Effect Controls (D-035), as Premiere's, for the selected clip — else the main clip: Motion (Position X, Y,
// Scale) and Opacity, at the playhead; the stopwatch (Keyframes: on, a value typed at the playhead makes or updates the
// key there; off, the keys go and the clip stays as shown); ◀ ◆ ▶ — the previous key, a key at the playhead added or
// removed, the next key; the Masks — one row each, Add rectangle or ellipse (a Select mask over its middle half, made a
// hole by Mode Cut), and the selected one's Mode, Feather, Strength, Delete. Every edit one Undo step (a typed field
// merges its own); the Clip effects section edits the same clip (Project::panelEffects).
#include "core/Project.h"
#include "core/SequenceEdit.h"
#include "core/SequenceView.h"
#include "ui/WalkRunner.h"
#include "ui/panels/Section.h"
#include "ui/panels/SidePanel.h"
#include "ui/widgets/ResetLabel.h"
#include "ui/widgets/StepSpinBox.h"

#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>

#include <algorithm>
#include <cmath>
#include <memory>

namespace avloop {

namespace {

const Clip* clipOf(const Project* p) {
    if (const int id = p->selectedClip()) return sequenceedit::clip(p->sequence(), id);
    return sequenceview::mainClip(p->sequence());
}

// The clip's own time at the playhead, held at its ends when it does not show then.
double ownAt(const Clip& c, double t) {
    return std::clamp(c.range.in + (t - c.start), c.range.in, c.range.out);
}

QString modeWord(MaskShape::Role r) {
    switch (r) {
    case MaskShape::Role::Cut:
        return QObject::tr("Cut (a hole)");
    case MaskShape::Role::KeepOnly:
        return QObject::tr("Keep only");
    case MaskShape::Role::Exclude:
        return QObject::tr("Exclude");
    case MaskShape::Role::Add:
        break;
    }
    return QObject::tr("Select");
}

} // namespace

void SidePanel::bindControls(Project* project, std::function<void(double)> seek) {
    Section* s = controls_;
    QWidget* body = s->body();
    auto* hint = new QLabel(tr("Select a clip on the timeline."), body);
    hint->setProperty("quiet", true);
    hint->setObjectName(QStringLiteral("controls.hint"));
    s->addWide(hint);
    auto number = [&](const char* key, const QString& name, double min, double max, double step, int decimals,
                      const QString& suffix) {
        auto* box =
            new StepSpinBox(QStringLiteral("controls.%1").arg(QLatin1String(key)), min, max, step, decimals, body);
        box->setSuffix(suffix);
        s->addRow(name, box);
        return box;
    };
    StepSpinBox* x = number("x", tr("Position X"), -100000, 100000, 1, 1, QString());
    StepSpinBox* y = number("y", tr("Position Y"), -100000, 100000, 1, 1, QString());
    StepSpinBox* scale = number("scale", tr("Scale"), 5, 1000, 1, 1, QStringLiteral(" %"));
    StepSpinBox* opacity = number("opacity", tr("Opacity"), 0, 100, 1, 0, QStringLiteral(" %"));
    reset::setDefault(scale, 100.0);
    reset::setDefault(opacity, 100.0);
    auto* keys = new QWidget(body);
    auto* keysRow = new QHBoxLayout(keys);
    keysRow->setContentsMargins(0, 0, 0, 0);
    keysRow->setSpacing(4);
    auto* watch = new QCheckBox(tr("Keyframes"), keys);
    watch->setObjectName(QStringLiteral("controls.keys"));
    watch->setToolTip(tr("The stopwatch: on, a change at the playhead makes or updates a key there"));
    auto button = [&](const char* name, const QString& text, const QString& tip) {
        auto* b = new QPushButton(text, keys);
        b->setObjectName(QStringLiteral("controls.%1").arg(QLatin1String(name)));
        b->setToolTip(tip);
        b->setFixedWidth(28);
        keysRow->addWidget(b);
        return b;
    };
    keysRow->addWidget(watch);
    QPushButton* prev = button("prev", QStringLiteral("◀"), tr("The previous key"));
    QPushButton* key = button("key", QStringLiteral("◆"), tr("A key at the playhead: add or remove"));
    QPushButton* next = button("next", QStringLiteral("▶"), tr("The next key"));
    keysRow->addStretch(1);
    s->addRow(tr("Motion"), keys);
    auto* keyInfo = new QLabel(body);
    keyInfo->setProperty("quiet", true);
    keyInfo->setObjectName(QStringLiteral("controls.keyInfo"));
    s->addRow(QString(), keyInfo);

    auto* masks = new QListWidget(body);
    masks->setObjectName(QStringLiteral("controls.masks"));
    masks->setFixedHeight(72);
    s->addRow(tr("Masks"), masks);
    auto* adds = new QWidget(body);
    auto* addsRow = new QHBoxLayout(adds);
    addsRow->setContentsMargins(0, 0, 0, 0);
    auto* addRect = new QPushButton(tr("Rectangle"), adds);
    addRect->setObjectName(QStringLiteral("controls.addRect"));
    auto* addEllipse = new QPushButton(tr("Ellipse"), adds);
    addEllipse->setObjectName(QStringLiteral("controls.addEllipse"));
    auto* removeMask = new QPushButton(tr("Delete"), adds);
    removeMask->setObjectName(QStringLiteral("controls.maskDelete"));
    addsRow->addWidget(addRect);
    addsRow->addWidget(addEllipse);
    addsRow->addWidget(removeMask);
    s->addRow(QString(), adds);
    auto* mode = new QComboBox(body);
    mode->setObjectName(QStringLiteral("controls.maskMode"));
    for (MaskShape::Role r :
         {MaskShape::Role::Add, MaskShape::Role::Cut, MaskShape::Role::KeepOnly, MaskShape::Role::Exclude})
        mode->addItem(modeWord(r), MaskShape::roleId(r));
    s->addRow(tr("Mode"), mode);
    StepSpinBox* feather = number("maskFeather", tr("Feather"), 0, CropShape::kMaxFeather, 1, 0, QStringLiteral(" px"));
    StepSpinBox* strength = number("maskStrength", tr("Strength"), 0, 100, 1, 0, QStringLiteral(" %"));
    const auto selectedMask = std::make_shared<int>(-1);
    const QList<QWidget*> fields = {x, y, scale, opacity, keys, keyInfo, masks, adds};
    const QList<QWidget*> maskFields = {mode, feather, strength};

    auto refresh = [=] {
        const Clip* c = clipOf(project);
        s->setTitle(c ? tr("Effect Controls — %1").arg(c->name) : tr("Effect Controls"));
        hint->setVisible(!c);
        for (QWidget* w : fields)
            for (QWidget* part : s->row(w))
                part->setVisible(c);
        if (c && *selectedMask >= c->masks.size()) *selectedMask = -1;
        for (QWidget* w : maskFields)
            for (QWidget* part : s->row(w))
                part->setVisible(c && *selectedMask >= 0);
        if (!c) return;
        const double own = ownAt(*c, project->time());
        const VideoLayer::Look look = c->look(own);
        x->setValueQuietly(look.transform.x);
        y->setValueQuietly(look.transform.y);
        scale->setValueQuietly(look.transform.scale * 100);
        opacity->setValueQuietly(look.opacity);
        const QSignalBlocker bw(watch);
        watch->setChecked(!c->keys.isEmpty());
        const int k = c->keyAt(own, project->keyTolerance());
        keyInfo->setText(c->keys.isEmpty() ? tr("No keyframes")
                         : k >= 0          ? tr("Keyframe %1 of %2").arg(k + 1).arg(c->keys.size())
                                           : tr("%1 keyframes").arg(c->keys.size()));
        prev->setEnabled(!c->keys.isEmpty());
        next->setEnabled(!c->keys.isEmpty());
        const QSignalBlocker bm(masks);
        masks->clear();
        for (const MaskShape& m : c->masks)
            masks->addItem(QStringLiteral("%1 · %2").arg(m.name, modeWord(m.role)));
        masks->setCurrentRow(*selectedMask);
        removeMask->setEnabled(*selectedMask >= 0);
        if (*selectedMask < 0) return;
        const MaskShape& m = c->masks[*selectedMask];
        const QSignalBlocker bo(mode);
        mode->setCurrentIndex(mode->findData(MaskShape::roleId(m.role)));
        feather->setValueQuietly(m.feather);
        strength->setValueQuietly(m.strength);
    };
    connect(project, &Project::changed, this, [refresh](Project::Aspect a) {
        using A = Project::Aspect;
        if (a == A::Sequence || a == A::LayerSelection || a == A::Source || a == A::Time) refresh();
    });
    connect(masks, &QListWidget::currentRowChanged, this, [=](int row) {
        *selectedMask = row;
        refresh();
    });

    auto edit = [project](const QString& field, auto change) {
        const Clip* c = clipOf(project);
        if (!c) return;
        Clip after = *c;
        change(after);
        if (after == *c) return;
        // A typed field merges its edits; a press ("!…") is a step of its own.
        project->changeClip(after, tr("Change %1").arg(after.name),
                            field.startsWith(QLatin1Char('!')) ? QString()
                                                               : QStringLiteral("field:controls.%1").arg(field));
    };
    auto look = [=](const QString& field, auto change) {
        edit(field, [=](Clip& c) {
            const double own = ownAt(c, project->time());
            VideoLayer::Look now = c.look(own);
            change(now);
            c.setLook(own, now, project->keyTolerance());
        });
    };
    connect(x, &StepSpinBox::valueChanged, this,
            [=](double v) { look(QStringLiteral("x"), [v](VideoLayer::Look& k) { k.transform.x = v; }); });
    connect(y, &StepSpinBox::valueChanged, this,
            [=](double v) { look(QStringLiteral("y"), [v](VideoLayer::Look& k) { k.transform.y = v; }); });
    connect(scale, &StepSpinBox::valueChanged, this, [=](double v) {
        look(QStringLiteral("scale"), [v](VideoLayer::Look& k) { k.transform.scale = v / 100; });
    });
    connect(opacity, &StepSpinBox::valueChanged, this,
            [=](double v) { look(QStringLiteral("opacity"), [v](VideoLayer::Look& k) { k.opacity = v; }); });
    // The stopwatch and ◆: a key made with the values shown; the last one gone, the clip stays as it was shown.
    auto unkeyAll = [](Clip& c, double own) {
        const VideoLayer::Look now = c.look(own);
        c.transform.x = now.transform.x;
        c.transform.y = now.transform.y;
        c.transform.scale = now.transform.scale;
        c.opacity = now.opacity;
        c.keys.clear();
    };
    auto keyHere = [](Clip& c, double own) {
        const VideoLayer::Look now = c.look(own);
        c.keys.append({own, now.transform.x, now.transform.y, now.transform.scale, now.opacity, false});
        std::sort(c.keys.begin(), c.keys.end(), [](const LayerKey& a, const LayerKey& b) { return a.t < b.t; });
    };
    connect(watch, &QCheckBox::toggled, this, [=](bool on) {
        edit(QStringLiteral("!watch"), [=](Clip& c) {
            const double own = ownAt(c, project->time());
            on&& c.keys.isEmpty() ? keyHere(c, own) : on ? void() : unkeyAll(c, own);
        });
    });
    connect(key, &QPushButton::clicked, this, [=] {
        edit(QStringLiteral("!key"), [=](Clip& c) {
            const double own = ownAt(c, project->time());
            const int k = c.keyAt(own, project->keyTolerance());
            if (k < 0) return keyHere(c, own);
            if (c.keys.size() == 1) return unkeyAll(c, own);
            c.keys.removeAt(k);
        });
    });
    auto step = [=](int direction) {
        const Clip* c = clipOf(project);
        if (!c || !seek) return;
        const double now = project->time(), tol = project->keyTolerance();
        double best = direction < 0 ? -1e9 : 1e9;
        for (const LayerKey& k : c->keys) {
            const double at = c->start + (k.t - c->range.in);
            if (direction < 0 && at < now - tol && at > best) best = at;
            if (direction > 0 && at > now + tol && at < best) best = at;
        }
        if (std::abs(best) < 1e8) seek(best);
    };
    connect(prev, &QPushButton::clicked, this, [step] { step(-1); });
    connect(next, &QPushButton::clicked, this, [step] { step(1); });
    // The masks (in the clip's own px; drawn on the monitor in AS-006).
    auto addMask = [=](MaskShape::Kind kind) {
        edit(QStringLiteral("!mask"), [=](Clip& c) {
            if (c.masks.size() >= MaskShape::kMaxShapes) return;
            MaskShape m;
            m.kind = kind;
            m.rect = QRectF(c.size.width() / 4, c.size.height() / 4, c.size.width() / 2, c.size.height() / 2);
            m.name = maskshapes::nextName(c.masks, kind);
            c.masks.append(m);
            *selectedMask = int(c.masks.size() - 1);
        });
    };
    connect(addRect, &QPushButton::clicked, this, [=] { addMask(MaskShape::Kind::Rect); });
    connect(addEllipse, &QPushButton::clicked, this, [=] { addMask(MaskShape::Kind::Ellipse); });
    auto maskEdit = [=](const QString& field, auto change) {
        edit(field, [=](Clip& c) {
            if (*selectedMask >= 0 && *selectedMask < c.masks.size()) change(c.masks[*selectedMask]);
        });
    };
    connect(removeMask, &QPushButton::clicked, this, [=] {
        const int i = *selectedMask;
        *selectedMask = -1;
        edit(QStringLiteral("!maskDelete"), [i](Clip& c) {
            if (i >= 0 && i < c.masks.size()) c.masks.removeAt(i);
        });
    });
    connect(mode, &QComboBox::activated, this, [=] {
        const MaskShape::Role r = MaskShape::roleFromId(mode->currentData().toString());
        maskEdit(QStringLiteral("!maskMode"), [r](MaskShape& m) { m.role = r; });
    });
    connect(feather, &StepSpinBox::valueChanged, this,
            [=](double v) { maskEdit(QStringLiteral("maskFeather"), [v](MaskShape& m) { m.feather = v; }); });
    connect(strength, &StepSpinBox::valueChanged, this,
            [=](double v) { maskEdit(QStringLiteral("maskStrength"), [v](MaskShape& m) { m.strength = v; }); });
    // The walks' readings: the clip shown, its look at the playhead (x, y, scale %, opacity %), its keys, its masks.
    WalkProbes::add(QStringLiteral("controls.clip"), [project] {
        const Clip* c = clipOf(project);
        return c ? c->name : QString();
    });
    WalkProbes::add(QStringLiteral("controls.look"), [project] {
        const Clip* c = clipOf(project);
        if (!c) return QString();
        const VideoLayer::Look l = c->look(ownAt(*c, project->time()));
        return QStringLiteral("%1,%2,%3,%4")
            .arg(l.transform.x, 0, 'f', 1)
            .arg(l.transform.y, 0, 'f', 1)
            .arg(l.transform.scale * 100, 0, 'f', 1)
            .arg(l.opacity, 0, 'f', 1);
    });
    WalkProbes::add(QStringLiteral("controls.keyCount"), [project] {
        const Clip* c = clipOf(project);
        return c ? int(c->keys.size()) : 0;
    });
    WalkProbes::add(QStringLiteral("controls.maskList"), [project] {
        QStringList list;
        if (const Clip* c = clipOf(project))
            for (const MaskShape& m : c->masks)
                list << QStringLiteral("%1:%2").arg(MaskShape::kindId(m.kind), MaskShape::roleId(m.role));
        return list.join(QLatin1Char(','));
    });
    refresh();
}

} // namespace avloop
