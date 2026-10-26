// About AvLoop (spec §5.1's menu, §3.8): the version, the licence, and the libraries AvLoop links with theirs.
#include "ui/dialogs/AboutDialog.h"

#include "core/AppInfo.h"
#include "media/MediaProbe.h"
#include "ui/theme/IconFactory.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace avloop {

AboutDialog::AboutDialog(QWidget* parent) : QDialog(parent) {
    setObjectName(QStringLiteral("AboutDialog"));
    setWindowTitle(tr("About AvLoop"));
    setAttribute(Qt::WA_DeleteOnClose);
    auto* column = new QVBoxLayout(this);
    auto* head = new QHBoxLayout;
    auto* logo = new QLabel(this);
    logo->setPixmap(IconFactory::appIcon().pixmap(64, 64));
    head->addWidget(logo, 0, Qt::AlignTop);
    auto* title = new QLabel(tr("<h2>AvLoop %1</h2><p>Cut any video into a seamless, transparent loop for OBS "
                                "Studio.</p><p>MIT licence · github.com/rubeldbc/avLoop</p>")
                                 .arg(AppInfo::version()),
                             this);
    title->setObjectName(QStringLiteral("about.title"));
    title->setWordWrap(true);
    head->addWidget(title, 1);
    column->addLayout(head);
    const QString libraries =
        tr("<p><b>Built with</b></p><ul>"
           "<li>Qt %1 (LGPLv3) — Core, Gui, Widgets, OpenGL, Svg, Concurrent</li>"
           "<li>%2 (LGPL 2.1+) — libavformat, libavcodec, libavutil, libswscale, libavfilter</li>"
           "<li>libvpx (BSD) — VP8/VP9 with alpha · dav1d (BSD) — AV1 · zimg (WTFPL) — HDR to SDR</li>"
           "<li>Windows: D3D11, DXGI, PSAPI, DbgHelp</li></ul>"
           "<p>Qt and FFmpeg are linked dynamically; their licences are in LICENSE-THIRD-PARTY.md.</p>")
            .arg(QString::fromLatin1(qVersion()), media::MediaProbe::libraryVersions());
    auto* list = new QLabel(libraries, this);
    list->setObjectName(QStringLiteral("about.libraries"));
    list->setWordWrap(true);
    list->setTextInteractionFlags(Qt::TextSelectableByMouse);
    column->addWidget(list, 1);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    buttons->button(QDialogButtonBox::Close)->setObjectName(QStringLiteral("about.close"));
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::close);
    column->addWidget(buttons);
    resize(520, 360);
}

} // namespace avloop
