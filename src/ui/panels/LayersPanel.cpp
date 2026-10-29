// The side panel's Layers section (D-027): the videos over the main one, top first ("name · V3"), each row's check box
// shows or hides it; for the selected: X, Y, Scale, Opacity, Start, Track, Locked, Delete. Every edit one Undo step;
// a right click resets the group (the layers removed). Add one by ☰ → Add as layer… (Ctrl+Shift+O).
#include "core/Project.h"
#include "ui/WalkRunner.h"
#include "ui/panels/Section.h"
#include "ui/panels/SidePanel.h"
#include "ui/widgets/ResetLabel.h"
#include "ui/widgets/StepSpinBox.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>

namespace avloop {

void SidePanel::bindLayers(Project* project) {
    Section* s = layers_;
    QWidget* body = s->body();
    auto* hint = new QLabel(tr("Add a video over this one: ☰ → Add as layer… (Ctrl+Shift+O)."), body);
    hint->setProperty("quiet", true);
    hint->setWordWrap(true);
    s->addWide(hint);
    auto* list = new QListWidget(body);
    list->setObjectName(QStringLiteral("panel.layers.list"));
    list->setFixedHeight(96);
    s->addWide(list);
    auto number = [&](const char* key, const QString& name, double min, double max, double step, int decimals,
                      const QString& suffix) {
        auto* box =
            new StepSpinBox(QStringLiteral("panel.layers.%1").arg(QLatin1String(key)), min, max, step, decimals, body);
        box->setSuffix(suffix);
        s->addRow(name, box);
        return box;
    };
    StepSpinBox* x = number("x", QStringLiteral("X"), -100000, 100000, 1, 1, QString());
    StepSpinBox* y = number("y", QStringLiteral("Y"), -100000, 100000, 1, 1, QString());
    StepSpinBox* scale = number("scale", tr("Scale"), 5, 1000, 1, 1, QStringLiteral(" %"));
    StepSpinBox* opacity = number("opacity", tr("Opacity"), 0, 100, 1, 0, QStringLiteral(" %"));
    StepSpinBox* start = number("start", tr("Start"), -3600, 36000, 0.1, 2, QStringLiteral(" s"));
    StepSpinBox* track = number("track", tr("Track"), 2, 1 + VideoLayer::kMaxLayers, 1, 0, QString());
    reset::setDefault(scale, 100.0);
    reset::setDefault(opacity, 100.0);
    auto* flags = new QWidget(body);
    auto* flagsRow = new QHBoxLayout(flags);
    flagsRow->setContentsMargins(0, 0, 0, 0);
    auto* locked = new QCheckBox(tr("Locked"), flags);
    locked->setObjectName(QStringLiteral("panel.layers.locked"));
    auto* remove = new QPushButton(tr("Delete"), flags);
    remove->setObjectName(QStringLiteral("panel.layers.delete"));
    flagsRow->addWidget(locked);
    flagsRow->addStretch(1);
    flagsRow->addWidget(remove);
    s->addRow(QString(), flags);
    auto* keys = new QWidget(body);
    auto* keysRow = new QHBoxLayout(keys);
    keysRow->setContentsMargins(0, 0, 0, 0);
    keysRow->setSpacing(4);
    auto* key = new QPushButton(tr("Key"), keys);
    key->setObjectName(QStringLiteral("panel.layers.key"));
    key->setToolTip(tr("A keyframe at the playhead: its place, scale and opacity"));
    auto* unkey = new QPushButton(tr("Delete key"), keys);
    unkey->setObjectName(QStringLiteral("panel.layers.unkey"));
    auto* ease = new QCheckBox(tr("Ease"), keys);
    ease->setObjectName(QStringLiteral("panel.layers.ease"));
    keysRow->addWidget(key);
    keysRow->addWidget(unkey);
    keysRow->addWidget(ease);
    keysRow->addStretch(1);
    s->addRow(tr("Keyframes"), keys);
    auto* keyInfo = new QLabel(body);
    keyInfo->setProperty("quiet", true);
    keyInfo->setObjectName(QStringLiteral("panel.layers.keyInfo"));
    s->addRow(QString(), keyInfo);
    const QList<QWidget*> fields = {x, y, scale, opacity, start, track, flags, keys, keyInfo};

    // Rows top first: by track, the higher above.
    auto order = [project] {
        QList<int> o = videolayers::drawOrder(project->layers());
        std::reverse(o.begin(), o.end());
        return o;
    };
    auto refresh = [=] {
        const QList<VideoLayer>& layers = project->layers();
        const int sel = project->selectedLayer();
        {
            const QSignalBlocker block(list);
            list->clear();
            for (int i : order()) {
                auto* item =
                    new QListWidgetItem(QStringLiteral("%1 · V%2").arg(layers[i].name).arg(layers[i].track), list);
                item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
                item->setCheckState(layers[i].visible ? Qt::Checked : Qt::Unchecked);
                item->setData(Qt::UserRole, i);
                item->setSelected(i == sel);
            }
        }
        hint->setVisible(layers.isEmpty());
        list->setVisible(!layers.isEmpty());
        for (QWidget* w : fields)
            for (QWidget* part : s->row(w))
                part->setVisible(sel >= 0);
        if (sel < 0) return;
        const VideoLayer& l = layers[sel];
        // The values shown are the layer's at the playhead (keyed, D-029).
        double own = l.range.in;
        l.sourceTime(project->time(), &own);
        const VideoLayer::Look look = l.at(own);
        x->setValueQuietly(look.transform.x);
        y->setValueQuietly(look.transform.y);
        scale->setValueQuietly(look.transform.scale * 100);
        opacity->setValueQuietly(look.opacity);
        const int k = l.keyAt(own, project->layerKeyTolerance());
        keyInfo->setText(l.keys.isEmpty() ? tr("No keyframes")
                         : k >= 0         ? tr("Keyframe %1 of %2").arg(k + 1).arg(l.keys.size())
                                          : tr("%1 keyframes").arg(l.keys.size()));
        const QSignalBlocker be(ease);
        ease->setChecked(k >= 0 && l.keys[k].ease);
        ease->setEnabled(k >= 0);
        unkey->setEnabled(k >= 0);
        start->setValueQuietly(l.start);
        track->setValueQuietly(l.track);
        const QSignalBlocker b(locked);
        locked->setChecked(l.locked);
    };
    connect(project, &Project::changed, this, [refresh](Project::Aspect a) {
        if (a == Project::Aspect::Layers || a == Project::Aspect::LayerSelection || a == Project::Aspect::Source ||
            a == Project::Aspect::Time)
            refresh();
    });
    connect(list, &QListWidget::itemSelectionChanged, this, [=] {
        const QList<QListWidgetItem*> chosen = list->selectedItems();
        project->setSelectedLayer(chosen.isEmpty() ? -1 : chosen.first()->data(Qt::UserRole).toInt());
    });
    connect(list, &QListWidget::itemChanged, this, [=](QListWidgetItem* item) {
        QList<VideoLayer> l = project->layers();
        const int i = item->data(Qt::UserRole).toInt();
        if (i < 0 || i >= l.size()) return;
        l[i].visible = item->checkState() == Qt::Checked;
        project->changeLayers(l);
    });
    auto edit = [project](const QString& field, auto change) {
        const int i = project->selectedLayer();
        if (i < 0) return;
        QList<VideoLayer> l = project->layers();
        change(l[i]);
        // A typed field merges its edits; a press ("!…": Key, Delete key, Ease, Locked) is a step of its own.
        project->changeLayers(l, field.startsWith(QLatin1Char('!')) ? QString()
                                                                    : QStringLiteral("field:layers.%1").arg(field));
    };
    // A place, scale or opacity typed: into the key at the playhead when the layer is keyed (made when none, as
    // Premiere's once its stopwatch runs), else the layer's own (D-029).
    auto look = [=](const QString& field, auto change) {
        edit(field, [=](VideoLayer& l) {
            double own = l.range.in;
            l.sourceTime(project->time(), &own);
            VideoLayer::Look now = l.at(own);
            change(now);
            l.setLook(own, now, project->layerKeyTolerance());
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
    // The keyframes: Key adds one at the playhead with the values shown (or keeps the one there); Delete key; Ease.
    connect(key, &QPushButton::clicked, this, [=] {
        edit(QStringLiteral("!key"), [=](VideoLayer& l) {
            double own = l.range.in;
            l.sourceTime(project->time(), &own);
            const VideoLayer::Look now = l.at(own);
            if (l.keyAt(own, project->layerKeyTolerance()) >= 0 || l.keys.size() >= VideoLayer::kMaxKeys) return;
            l.keys.append({own, now.transform.x, now.transform.y, now.transform.scale, now.opacity, false});
            std::sort(l.keys.begin(), l.keys.end(), [](const LayerKey& a, const LayerKey& b) { return a.t < b.t; });
        });
    });
    auto atKey = [=](auto change) {
        edit(QStringLiteral("!keys"), [=](VideoLayer& l) {
            double own = l.range.in;
            l.sourceTime(project->time(), &own);
            if (const int k = l.keyAt(own, project->layerKeyTolerance()); k >= 0) change(l, k);
        });
    };
    connect(unkey, &QPushButton::clicked, this, [=] {
        atKey([](VideoLayer& l, int k) {
            // The last key gone: the layer stays where it was shown.
            if (l.keys.size() == 1) {
                l.transform.x = l.keys[0].x;
                l.transform.y = l.keys[0].y;
                l.transform.scale = l.keys[0].scale;
                l.opacity = l.keys[0].opacity;
            }
            l.keys.removeAt(k);
        });
    });
    connect(ease, &QCheckBox::clicked, this,
            [=](bool on) { atKey([on](VideoLayer& l, int k) { l.keys[k].ease = on; }); });
    connect(start, &StepSpinBox::valueChanged, this,
            [=](double v) { edit(QStringLiteral("start"), [v](VideoLayer& l) { l.start = v; }); });
    connect(track, &StepSpinBox::valueChanged, this,
            [=](double v) { edit(QStringLiteral("track"), [v](VideoLayer& l) { l.track = int(v); }); });
    connect(locked, &QCheckBox::clicked, this,
            [=](bool on) { edit(QStringLiteral("!locked"), [on](VideoLayer& l) { l.locked = on; }); });
    connect(remove, &QPushButton::clicked, this, [project] {
        const int i = project->selectedLayer();
        if (i < 0) return;
        QList<VideoLayer> l = project->layers();
        l.removeAt(i);
        project->setSelectedLayer(-1);
        project->changeLayers(l);
    });
    s->addResetAction(tr("Reset Layers"), QStringLiteral("layers"), [project] {
        project->setSelectedLayer(-1);
        project->changeLayers({});
    });
    WalkProbes::add(QStringLiteral("layers.count"), [project] { return int(project->layers().size()); });
    WalkProbes::add(QStringLiteral("layers.selected"), [project] { return project->selectedLayer(); });
    // The selected layer's keys "t:x,y,scale,opacity" (";" between) and its effects (D-029).
    WalkProbes::add(QStringLiteral("layers.keys"), [project] {
        const int i = project->selectedLayer();
        QStringList parts;
        if (i >= 0)
            for (const LayerKey& k : project->layers()[i].keys)
                parts << QStringLiteral("%1:%2,%3,%4,%5%6")
                             .arg(k.t, 0, 'f', 2)
                             .arg(std::lround(k.x))
                             .arg(std::lround(k.y))
                             .arg(std::lround(k.scale * 100))
                             .arg(std::lround(k.opacity))
                             .arg(k.ease ? QStringLiteral("e") : QString());
        return parts.join(QLatin1Char(';'));
    });
    WalkProbes::add(QStringLiteral("layers.effects"), [project] {
        const int i = project->selectedLayer();
        return i >= 0 ? !project->layers()[i].effects.isNeutral() : false;
    });
    WalkProbes::add(QStringLiteral("effects.main"), [project] { return !project->effects().isNeutral(); });
    // The selected layer's timing and track (AV-031's walks read them with near).
    auto selectedOf = [project](auto read) {
        const int i = project->selectedLayer();
        return i >= 0 ? QVariant(read(project->layers()[i])) : QVariant();
    };
    WalkProbes::add(QStringLiteral("layers.start"),
                    [=] { return selectedOf([](const VideoLayer& l) { return l.start; }); });
    WalkProbes::add(QStringLiteral("layers.in"),
                    [=] { return selectedOf([](const VideoLayer& l) { return l.range.in; }); });
    WalkProbes::add(QStringLiteral("layers.out"),
                    [=] { return selectedOf([](const VideoLayer& l) { return l.range.out; }); });
    WalkProbes::add(QStringLiteral("layers.track"),
                    [=] { return selectedOf([](const VideoLayer& l) { return l.track; }); });
    // "name:V<track>:<start>:<x>,<y>,<scale %>:<opacity>" a layer, ";" between.
    WalkProbes::add(QStringLiteral("layers.summary"), [project] {
        QStringList parts;
        for (const VideoLayer& l : project->layers())
            parts << QStringLiteral("%1:V%2:%3:%4,%5,%6:%7%8")
                         .arg(l.name)
                         .arg(l.track)
                         .arg(l.start, 0, 'f', 2)
                         .arg(std::lround(l.transform.x))
                         .arg(std::lround(l.transform.y))
                         .arg(std::lround(l.transform.scale * 100))
                         .arg(std::lround(l.opacity))
                         .arg(l.visible ? QString() : QStringLiteral(":hidden"));
        return parts.join(QLatin1Char(';'));
    });
    refresh();
}

} // namespace avloop
