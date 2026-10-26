#include "ui/dialogs/SettingsDialog.h"

#include "core/AppSettings.h"
#include "ui/theme/Theme.h"
#include "ui/widgets/StepSpinBox.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>

namespace avloop {

namespace {

QLabel* note(const QString& text, QWidget* parent) {
    auto* l = new QLabel(text, parent);
    l->setProperty("quiet", true);
    l->setWordWrap(true);
    return l;
}

QComboBox* autoOff(const QString& name, const char* key, QWidget* parent) {
    auto* box = new QComboBox(parent);
    box->setObjectName(name);
    box->addItem(QObject::tr("Auto"), QStringLiteral("auto"));
    box->addItem(QObject::tr("Off"), QStringLiteral("off"));
    box->setCurrentIndex(appsettings::value(key, QStringLiteral("auto")).toString() == QLatin1String("off") ? 1 : 0);
    QObject::connect(box, &QComboBox::activated, box,
                     [box, key] { appsettings::setValue(key, box->currentData().toString()); });
    return box;
}

// A folder: its path, typed or chosen with "…"; empty means the default.
QWidget* folderField(const QString& name, const char* key, const QString& placeholder, QWidget* parent) {
    auto* row = new QWidget(parent);
    auto* h = new QHBoxLayout(row);
    h->setContentsMargins(0, 0, 0, 0);
    h->setSpacing(4);
    auto* edit = new QLineEdit(appsettings::value(key).toString(), row);
    edit->setObjectName(name);
    edit->setPlaceholderText(placeholder);
    auto* choose = new QToolButton(row);
    choose->setText(QStringLiteral("…"));
    choose->setToolTip(QObject::tr("Choose a folder"));
    h->addWidget(edit, 1);
    h->addWidget(choose);
    QObject::connect(edit, &QLineEdit::editingFinished, edit,
                     [edit, key] { appsettings::setValue(key, edit->text().trimmed()); });
    QObject::connect(choose, &QToolButton::clicked, edit, [edit, key] {
        const QString dir =
            QFileDialog::getExistingDirectory(edit->window(), QObject::tr("Choose a folder"), edit->text());
        if (dir.isEmpty()) return;
        edit->setText(QDir::toNativeSeparators(dir));
        appsettings::setValue(key, edit->text());
    });
    return row;
}

} // namespace

SettingsDialog::SettingsDialog(QWidget* parent) : QDialog(parent) {
    setObjectName(QStringLiteral("SettingsDialog"));
    setWindowTitle(tr("Settings"));
    setAttribute(Qt::WA_DeleteOnClose);
    auto* column = new QVBoxLayout(this);
    tabs_ = new QTabWidget(this);
    tabs_->setObjectName(QStringLiteral("settings.tabs"));
    tabs_->addTab(general(), tr("General"));
    tabs_->addTab(performance(), tr("Performance"));
    tabs_->addTab(exporting(), tr("Export"));
    column->addWidget(tabs_, 1);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    buttons->button(QDialogButtonBox::Close)->setObjectName(QStringLiteral("settings.close"));
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::close);
    column->addWidget(buttons);
    resize(520, 400);
}

void SettingsDialog::showTab(int index) {
    tabs_->setCurrentIndex(index);
}

QWidget* SettingsDialog::general() {
    auto* page = new QWidget(this);
    auto* form = new QFormLayout(page);
    auto* theme = new QComboBox(page);
    theme->setObjectName(QStringLiteral("settings.theme"));
    theme->addItem(tr("Navy"), Theme::idName(ThemeId::Navy));
    theme->addItem(tr("Black"), Theme::idName(ThemeId::Black));
    theme->setCurrentIndex(std::max(0, theme->findData(Theme::idName(Theme::instance().id()))));
    connect(theme, &QComboBox::activated, this, [this, theme] {
        appsettings::setValue(appsettings::kTheme, theme->currentData().toString());
        emit themeChanged(theme->currentData().toString());
    });
    form->addRow(tr("Theme"), theme);
    auto* openLast = new QCheckBox(tr("Open the last project at start"), page);
    openLast->setObjectName(QStringLiteral("settings.openLast"));
    openLast->setChecked(appsettings::openLastProject());
    connect(openLast, &QCheckBox::toggled, this,
            [](bool on) { appsettings::setValue(appsettings::kOpenLastProject, on); });
    form->addRow(QString(), openLast);
    auto* autosave = new StepSpinBox(QStringLiteral("settings.autosave"), 1, 60, 1, 0, page);
    autosave->setSuffix(tr(" min"));
    autosave->setValueQuietly(appsettings::autosaveMinutes());
    connect(autosave, &StepSpinBox::valueChanged, this, [this](double v) {
        appsettings::setValue(appsettings::kAutosaveMinutes, int(v));
        emit autosaveChanged(int(v));
    });
    form->addRow(tr("Autosave every"), autosave);
    return page;
}

QWidget* SettingsDialog::performance() {
    auto* page = new QWidget(this);
    auto* form = new QFormLayout(page);
    form->addRow(tr("Hardware decoding"),
                 autoOff(QStringLiteral("settings.hwdec"), appsettings::kHardwareDecoding, page));
    form->addRow(tr("GPU rendering"), autoOff(QStringLiteral("settings.gpu"), appsettings::kGpuRendering, page));
    form->addRow(QString(), note(tr("Both apply to the next video opened and the next render."), page));
    auto* ram = new StepSpinBox(QStringLiteral("settings.ram"), 256, 8192, 256, 0, page);
    ram->setSuffix(tr(" MB"));
    ram->setValueQuietly(appsettings::ramCacheMB());
    connect(ram, &StepSpinBox::valueChanged, this, [this](double v) {
        appsettings::setValue(appsettings::kRamCacheMB, int(v));
        emit ramCacheChanged(int(v));
    });
    form->addRow(tr("RAM cache"), ram);
    form->addRow(tr("Disk cache folder"),
                 folderField(QStringLiteral("settings.diskFolder"), appsettings::kDiskCacheFolder,
                             tr("Default (%LOCALAPPDATA%)"), page));
    auto* disk = new StepSpinBox(QStringLiteral("settings.diskGB"), 1, 500, 1, 0, page);
    disk->setSuffix(tr(" GB"));
    disk->setValueQuietly(appsettings::diskCacheGB());
    connect(disk, &StepSpinBox::valueChanged, this,
            [](double v) { appsettings::setValue(appsettings::kDiskCacheGB, int(v)); });
    form->addRow(tr("Disk cache size"), disk);
    form->addRow(QString(), note(tr("The disk cache's folder and size apply from the next start."), page));
    auto* clear = new QPushButton(tr("Clear cache"), page);
    clear->setObjectName(QStringLiteral("settings.clearCache"));
    connect(clear, &QPushButton::clicked, this, &SettingsDialog::clearCacheRequested);
    form->addRow(QString(), clear);
    return page;
}

QWidget* SettingsDialog::exporting() {
    auto* page = new QWidget(this);
    auto* form = new QFormLayout(page);
    form->addRow(tr("Default folder"), folderField(QStringLiteral("settings.exportFolder"), appsettings::kExportFolder,
                                                   tr("The last folder used"), page));
    auto* name = new QLineEdit(appsettings::nameTemplate(), page);
    name->setObjectName(QStringLiteral("settings.nameTemplate"));
    connect(name, &QLineEdit::editingFinished, this,
            [name] { appsettings::setValue(appsettings::kNameTemplate, name->text().trimmed()); });
    form->addRow(tr("Name template"), name);
    form->addRow(QString(), note(tr("{source} the video's name, {w} × {h} the output size."), page));
    auto* tips = new QCheckBox(tr("Show the OBS tips after an export"), page);
    tips->setObjectName(QStringLiteral("settings.obsTips"));
    tips->setChecked(appsettings::showObsTips());
    connect(tips, &QCheckBox::toggled, this, [](bool on) { appsettings::setValue(appsettings::kShowObsTips, on); });
    form->addRow(QString(), tips);
    return page;
}

} // namespace avloop
