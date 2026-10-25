#pragma once

#include <QColor>
#include <QObject>
#include <QString>
#include <QStringList>

namespace avloop {

enum class ThemeId { Navy, Black };

// The colour tokens of spec §5.2: one table for both themes; the QSS refers to them as @name.
struct ThemeTokens {
    QColor bg, panel, panel2, border, text, text2;
    QColor accent, accentHover, accentPress, accentSoft;
    QColor warning, danger, success;
    QColor onAccent;   // text and icons drawn on an accent fill
    QColor accentLine; // the active tool's outline: accent at 35%
};

class Theme : public QObject {
    Q_OBJECT
public:
    static Theme& instance();

    static ThemeTokens tokensFor(ThemeId id);
    static QString idName(ThemeId id);
    static ThemeId idFromName(const QString& name, ThemeId fallback = ThemeId::Navy);
    static QStringList tokenNames();
    static QColor token(const ThemeTokens& tokens, const QString& name);
    // Replaces every @token of a QSS template with the theme's colour; unknown tokens stay and are reported.
    static QString expand(const QString& qss, const ThemeTokens& tokens, QStringList* unknown = nullptr);
    static QString cssColor(const QColor& c);
    // The stylesheet of a theme: resources/themes/app.qss + the theme's own file, expanded.
    static QString styleSheetFor(ThemeId id);

    ThemeId id() const { return id_; }
    const ThemeTokens& tokens() const { return tokens_; }
    // Applies the theme to the running application: palette, fonts, stylesheet; icons re-render.
    void apply(ThemeId id);

signals:
    void changed(avloop::ThemeId id);

private:
    Theme();
    ThemeId id_ = ThemeId::Navy;
    ThemeTokens tokens_;
};

} // namespace avloop
