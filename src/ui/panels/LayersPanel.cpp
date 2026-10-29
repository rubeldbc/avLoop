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
    const QList<QWidget*> fields = {x, y, scale, opacity, start, track, flags};

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
        x->setValueQuietly(l.transform.x);
        y->setValueQuietly(l.transform.y);
        scale->setValueQuietly(l.transform.scale * 100);
        opacity->setValueQuietly(l.opacity);
        start->setValueQuietly(l.start);
        track->setValueQuietly(l.track);
        const QSignalBlocker b(locked);
        locked->setChecked(l.locked);
    };
    connect(project, &Project::changed, this, [refresh](Project::Aspect a) {
        if (a == Project::Aspect::Layers || a == Project::Aspect::LayerSelection || a == Project::Aspect::Source)
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
        project->changeLayers(l, QStringLiteral("field:layers.%1").arg(field));
    };
    connect(x, &StepSpinBox::valueChanged, this,
            [=](double v) { edit(QStringLiteral("x"), [v](VideoLayer& l) { l.transform.x = v; }); });
    connect(y, &StepSpinBox::valueChanged, this,
            [=](double v) { edit(QStringLiteral("y"), [v](VideoLayer& l) { l.transform.y = v; }); });
    connect(scale, &StepSpinBox::valueChanged, this,
            [=](double v) { edit(QStringLiteral("scale"), [v](VideoLayer& l) { l.transform.scale = v / 100; }); });
    connect(opacity, &StepSpinBox::valueChanged, this,
            [=](double v) { edit(QStringLiteral("opacity"), [v](VideoLayer& l) { l.opacity = v; }); });
    connect(start, &StepSpinBox::valueChanged, this,
            [=](double v) { edit(QStringLiteral("start"), [v](VideoLayer& l) { l.start = v; }); });
    connect(track, &StepSpinBox::valueChanged, this,
            [=](double v) { edit(QStringLiteral("track"), [v](VideoLayer& l) { l.track = int(v); }); });
    connect(locked, &QCheckBox::clicked, this,
            [=](bool on) { edit(QStringLiteral("locked"), [on](VideoLayer& l) { l.locked = on; }); });
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
