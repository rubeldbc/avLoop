#pragma once

#include <QIcon>
#include <QPixmap>
#include <QStringList>

namespace avloop {

// How an icon is coloured. Default follows the four states of spec §5.3 (normal text2, hover text, active accent,
// disabled 40%); the others hold one colour in every state but disabled.
enum class IconTone { Default, OnAccent, Accent, Warning, Danger, Success, Text };

// The app's own icons (spec §5.3): resources/icons/<name>.svg, `currentColor` replaced at runtime by the state's
// colour, rendered per device pixel ratio by QSvgRenderer and cached until the theme changes.
class IconFactory {
public:
    static QIcon icon(const QString& name, IconTone tone = IconTone::Default);
    static QPixmap pixmap(const QString& name, const QSize& size, qreal dpr, QIcon::Mode mode, QIcon::State state,
                          IconTone tone = IconTone::Default);
    static QIcon appIcon();
    // Every icon the app uses, as listed in spec §5.3 (tools/icons/check-icons.mjs reads this list).
    static const QStringList& allNames();
    static bool exists(const QString& name);
    static void clearCache();
};

} // namespace avloop
