// The preset row above the side panel's sections (spec §11.3): the built-in OBS presets, then the user's; choosing one
// applies its Loop, Output and Quality as one Undo step; the box shows the preset the settings match, else "— Preset
// —". ✓ saves the current settings under a name (%APPDATA%\AvLoop\presets\<name>.json), ✕ deletes the chosen user
// preset.
#include "core/AppSettings.h"
#include "core/Presets.h"
#include "core/Project.h"
#include "ui/WalkRunner.h"
#include "ui/panels/SidePanel.h"
#include "ui/widgets/Toast.h"

#include <QComboBox>
#include <QMainWindow>
#include <QToolButton>

#include <memory>

namespace avloop {

void SidePanel::bindPresets(Project* project, std::function<QString()> askName,
                            std::function<bool(const QString&)> confirm) {
    auto list = std::make_shared<QList<Preset>>();
    auto refresh = [=] {
        const QSignalBlocker block(presets_);
        *list = presets::all(appsettings::presetsDir());
        presets_->clear();
        presets_->addItem(tr("— Preset —"), QString());
        int current = 0;
        for (const Preset& p : *list) {
            presets_->addItem(p.name, p.name);
            if (current == 0 && p.matches(project->loop(), project->output(), project->quality()))
                current = presets_->count() - 1;
        }
        presets_->setCurrentIndex(current);
        deletePreset_->setEnabled(current > 0 && !list->at(current - 1).builtIn);
    };
    connect(project, &Project::changed, this, [refresh](Project::Aspect a) {
        if (a == Project::Aspect::Loop || a == Project::Aspect::Output || a == Project::Aspect::Quality ||
            a == Project::Aspect::Source)
            refresh();
    });
    connect(presets_, &QComboBox::activated, this, [=](int index) {
        if (index <= 0 || index > list->size()) return;
        const Preset& p = list->at(index - 1);
        project->changeSettings(p.loop, p.output, p.quality, p.name);
        refresh();
    });
    connect(savePreset_, &QToolButton::clicked, this, [=] {
        const QString name = askName().trimmed();
        if (name.isEmpty()) return;
        Preset p{name, project->loop(), project->output(), project->quality(), false};
        QString error;
        auto* main = qobject_cast<QMainWindow*>(window());
        if (!presets::save(appsettings::presetsDir(), p, &error) && main)
            Toast::post(main->centralWidget(), tr("The preset was not saved: %1").arg(error), Toast::Kind::Error);
        refresh();
    });
    connect(deletePreset_, &QToolButton::clicked, this, [=] {
        const QString name = presets_->currentData().toString();
        if (name.isEmpty() || presets::isBuiltIn(name) || !confirm(tr("Delete the preset “%1”?").arg(name))) return;
        presets::remove(appsettings::presetsDir(), name);
        refresh();
    });
    WalkProbes::add(QStringLiteral("preset.current"), [this] { return presets_->currentData().toString(); });
    WalkProbes::add(QStringLiteral("preset.names"), [list] {
        QStringList names;
        for (const Preset& p : *list)
            names << p.name;
        return names.join(QLatin1Char('|'));
    });
    refresh();
}

} // namespace avloop
