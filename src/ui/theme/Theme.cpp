#include "ui/theme/Theme.h"

#include "ui/theme/IconFactory.h"

#include <QApplication>
#include <QFile>
#include <QFont>
#include <QPalette>
#include <QRegularExpression>
#include <QWidget>

namespace avloop {

namespace {

QColor withAlpha(QColor c, double alpha) {
    c.setAlphaF(static_cast<float>(alpha));
    return c;
}

QString readResource(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return {};
    return QString::fromUtf8(f.readAll());
}

} // namespace

Theme::Theme() : tokens_(tokensFor(ThemeId::Navy)) {}

Theme& Theme::instance() {
    static Theme theme;
    return theme;
}

ThemeTokens Theme::tokensFor(ThemeId id) {
    ThemeTokens t;
    if (id == ThemeId::Black) {
        t.bg = QColor(0x0B, 0x0B, 0x0C);
        t.panel = QColor(0x15, 0x15, 0x17);
        t.panel2 = QColor(0x1E, 0x1E, 0x21);
        t.border = QColor(0x2C, 0x2C, 0x30);
        t.text = QColor(0xEC, 0xEC, 0xEC);
        t.text2 = QColor(0x9A, 0x9A, 0xA0);
    } else {
        t.bg = QColor(0x0E, 0x17, 0x26);
        t.panel = QColor(0x16, 0x22, 0x3A);
        t.panel2 = QColor(0x1D, 0x2B, 0x47);
        t.border = QColor(0x2A, 0x3A, 0x5C);
        t.text = QColor(0xE6, 0xEA, 0xF2);
        t.text2 = QColor(0x9A, 0xA6, 0xBD);
    }
    t.accent = QColor(0xFF, 0x7A, 0x1A);
    t.accentHover = QColor(0xFF, 0x94, 0x47);
    t.accentPress = QColor(0xE5, 0x65, 0x0A);
    t.accentSoft = withAlpha(t.accent, 0.18);
    t.warning = QColor(0xFF, 0xC5, 0x3D);
    t.danger = QColor(0xFF, 0x4D, 0x4F);
    t.success = QColor(0x3D, 0xD6, 0x8C);
    t.onAccent = QColor(0x11, 0x11, 0x11);
    t.accentLine = withAlpha(t.accent, 0.35);
    return t;
}

QString Theme::idName(ThemeId id) {
    return id == ThemeId::Black ? QStringLiteral("black") : QStringLiteral("navy");
}

ThemeId Theme::idFromName(const QString& name, ThemeId fallback) {
    if (name.compare(QLatin1String("black"), Qt::CaseInsensitive) == 0) return ThemeId::Black;
    if (name.compare(QLatin1String("navy"), Qt::CaseInsensitive) == 0) return ThemeId::Navy;
    return fallback;
}

QStringList Theme::tokenNames() {
    return {QStringLiteral("bg"),         QStringLiteral("panel"),       QStringLiteral("panel2"),
            QStringLiteral("border"),     QStringLiteral("text"),        QStringLiteral("text2"),
            QStringLiteral("accent"),     QStringLiteral("accentHover"), QStringLiteral("accentPress"),
            QStringLiteral("accentSoft"), QStringLiteral("warning"),     QStringLiteral("danger"),
            QStringLiteral("success"),    QStringLiteral("onAccent"),    QStringLiteral("accentLine")};
}

QColor Theme::token(const ThemeTokens& t, const QString& name) {
    const QColor* table[] = {&t.bg,      &t.panel,  &t.panel2,      &t.border,      &t.text,
                             &t.text2,   &t.accent, &t.accentHover, &t.accentPress, &t.accentSoft,
                             &t.warning, &t.danger, &t.success,     &t.onAccent,    &t.accentLine};
    const QStringList names = tokenNames();
    const qsizetype at = names.indexOf(name);
    return at < 0 ? QColor() : *table[at];
}

QString Theme::cssColor(const QColor& c) {
    if (c.alpha() == 255) return c.name(QColor::HexRgb);
    return QStringLiteral("rgba(%1, %2, %3, %4)").arg(c.red()).arg(c.green()).arg(c.blue()).arg(c.alpha());
}

QString Theme::expand(const QString& qss, const ThemeTokens& tokens, QStringList* unknown) {
    static const QRegularExpression re(QStringLiteral("@([A-Za-z][A-Za-z0-9]*)"));
    QString out;
    out.reserve(qss.size());
    qsizetype last = 0;
    for (auto it = re.globalMatch(qss); it.hasNext();) {
        const auto m = it.next();
        out += QStringView(qss).mid(last, m.capturedStart() - last);
        const QColor c = token(tokens, m.captured(1));
        if (c.isValid()) {
            out += cssColor(c);
        } else {
            out += m.captured(0);
            if (unknown) unknown->append(m.captured(1));
        }
        last = m.capturedEnd();
    }
    out += QStringView(qss).mid(last);
    return out;
}

QString Theme::styleSheetFor(ThemeId id) {
    const QString base = readResource(QStringLiteral(":/themes/app.qss"));
    const QString own = readResource(QStringLiteral(":/themes/%1.qss").arg(idName(id)));
    return expand(base + QLatin1Char('\n') + own, tokensFor(id));
}

void Theme::apply(ThemeId id) {
    id_ = id;
    tokens_ = tokensFor(id);
    auto* app = qobject_cast<QApplication*>(QCoreApplication::instance());
    if (app) {
        QFont font(QStringLiteral("Segoe UI"), 9);
        app->setFont(font);
        QPalette p;
        p.setColor(QPalette::Window, tokens_.bg);
        p.setColor(QPalette::WindowText, tokens_.text);
        p.setColor(QPalette::Base, tokens_.panel2);
        p.setColor(QPalette::AlternateBase, tokens_.panel);
        p.setColor(QPalette::Text, tokens_.text);
        p.setColor(QPalette::PlaceholderText, tokens_.text2);
        p.setColor(QPalette::Button, tokens_.panel2);
        p.setColor(QPalette::ButtonText, tokens_.text);
        p.setColor(QPalette::Highlight, tokens_.accent);
        p.setColor(QPalette::HighlightedText, tokens_.onAccent);
        p.setColor(QPalette::ToolTipBase, tokens_.panel2);
        p.setColor(QPalette::ToolTipText, tokens_.text);
        p.setColor(QPalette::Disabled, QPalette::Text, tokens_.text2);
        p.setColor(QPalette::Disabled, QPalette::ButtonText, tokens_.text2);
        p.setColor(QPalette::Disabled, QPalette::WindowText, tokens_.text2);
        app->setPalette(p);
        IconFactory::clearCache();
        app->setStyleSheet(styleSheetFor(id));
        for (QWidget* w : QApplication::allWidgets())
            w->update();
    }
    emit changed(id);
}

} // namespace avloop
