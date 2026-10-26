// About AvLoop by Rubel (spec §5.1's menu, §3.8): the version, the developer and the ways to reach them, the licence,
// and the libraries AvLoop links with theirs.
#include "ui/dialogs/AboutDialog.h"

#include "core/AppInfo.h"
#include "media/MediaProbe.h"
#include "ui/theme/IconFactory.h"

#include <QDate>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace avloop {

namespace {

// The developer's details: proper nouns, addresses and numbers, never translated.
constexpr auto kDeveloper = "Md. Kamrul Islam Rubel";
constexpr auto kEmail = "rubel.tv@gmail.com";
constexpr auto kProfile = "fb.com/rubel.social";
constexpr auto kWhatsApp = "+880 1760 002332";
constexpr auto kWhatsAppLink = "https://wa.me/8801760002332"; // wa.me: the full number, no punctuation
constexpr auto kRepository = "github.com/rubeldbc/avLoop";

QString link(const QString& href, const QString& text) {
    return QStringLiteral("<a href=\"%1\">%2</a>").arg(href.toHtmlEscaped(), text.toHtmlEscaped());
}

QString https(const char* address) {
    const QString text = QString::fromUtf8(address);
    return link(QStringLiteral("https://") + text, text);
}

// A value of the contact list: selectable, and a link opens in the browser or the mail program.
QLabel* value(const QString& html, const QString& id, QWidget* parent) {
    auto* label = new QLabel(html, parent);
    label->setObjectName(id);
    label->setTextFormat(Qt::RichText);
    label->setTextInteractionFlags(Qt::TextBrowserInteraction);
    label->setOpenExternalLinks(true);
    return label;
}

} // namespace

AboutDialog::AboutDialog(QWidget* parent) : QDialog(parent) {
    setObjectName(QStringLiteral("AboutDialog"));
    setWindowTitle(tr("About %1").arg(AppInfo::displayName()));
    setAttribute(Qt::WA_DeleteOnClose);
    auto* column = new QVBoxLayout(this);
    auto* head = new QHBoxLayout;
    auto* logo = new QLabel(this);
    logo->setPixmap(IconFactory::appIcon().pixmap(64, 64));
    head->addWidget(logo, 0, Qt::AlignTop);
    auto* title = value(tr("<h2>%1</h2><p>Version %2</p><p>Cut any video into a seamless, transparent loop for OBS "
                           "Studio.</p><p>MIT licence · %3</p>")
                            .arg(AppInfo::displayName(), AppInfo::version(), https(kRepository)),
                        QStringLiteral("about.title"), this);
    title->setWordWrap(true);
    head->addWidget(title, 1);
    column->addLayout(head);
    column->addSpacing(8);

    column->addWidget(new QLabel(tr("<b>Developer &amp; contact</b>"), this));
    auto* contact = new QFormLayout;
    contact->setLabelAlignment(Qt::AlignLeft);
    const QString email = QString::fromUtf8(kEmail);
    contact->addRow(tr("Developer"),
                    value(QString::fromUtf8(kDeveloper).toHtmlEscaped(), QStringLiteral("about.developer"), this));
    contact->addRow(tr("Country"), value(tr("Bangladesh"), QStringLiteral("about.country"), this));
    contact->addRow(tr("Email"),
                    value(link(QStringLiteral("mailto:") + email, email), QStringLiteral("about.email"), this));
    contact->addRow(tr("Developer profile"), value(https(kProfile), QStringLiteral("about.profile"), this));
    contact->addRow(tr("WhatsApp"), value(link(QString::fromUtf8(kWhatsAppLink), QString::fromUtf8(kWhatsApp)),
                                          QStringLiteral("about.whatsapp"), this));
    column->addLayout(contact);

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
    column->addWidget(list);
    column->addStretch(1);

    auto* footer = new QLabel(tr("© %1 %2 · Designed and built in Bangladesh")
                                  .arg(QDate::currentDate().year())
                                  .arg(QString::fromUtf8(kDeveloper)),
                              this);
    footer->setObjectName(QStringLiteral("about.footer"));
    column->addWidget(footer);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    buttons->button(QDialogButtonBox::Close)->setObjectName(QStringLiteral("about.close"));
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::close);
    column->addWidget(buttons);
    resize(560, 560);
}

} // namespace avloop
