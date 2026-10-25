// The theme (spec §5.2): the token table's values, the QSS written with tokens only, the expansion complete, the
// switch without restart.
#include "ui/theme/Theme.h"

#include <QApplication>
#include <QFile>
#include <QRegularExpression>
#include <QSignalSpy>
#include <QTest>

using namespace avloop;

class TestTheme : public QObject {
    Q_OBJECT
private slots:
    void navyTokensAreTheSpecs() {
        const ThemeTokens t = Theme::tokensFor(ThemeId::Navy);
        QCOMPARE(t.bg.name(), QStringLiteral("#0e1726"));
        QCOMPARE(t.panel.name(), QStringLiteral("#16223a"));
        QCOMPARE(t.panel2.name(), QStringLiteral("#1d2b47"));
        QCOMPARE(t.border.name(), QStringLiteral("#2a3a5c"));
        QCOMPARE(t.text.name(), QStringLiteral("#e6eaf2"));
        QCOMPARE(t.text2.name(), QStringLiteral("#9aa6bd"));
    }
    void blackTokensAreTheSpecs() {
        const ThemeTokens t = Theme::tokensFor(ThemeId::Black);
        QCOMPARE(t.bg.name(), QStringLiteral("#0b0b0c"));
        QCOMPARE(t.panel.name(), QStringLiteral("#151517"));
        QCOMPARE(t.panel2.name(), QStringLiteral("#1e1e21"));
        QCOMPARE(t.border.name(), QStringLiteral("#2c2c30"));
        QCOMPARE(t.text.name(), QStringLiteral("#ececec"));
        QCOMPARE(t.text2.name(), QStringLiteral("#9a9aa0"));
    }
    void accentsAreShared() {
        for (ThemeId id : {ThemeId::Navy, ThemeId::Black}) {
            const ThemeTokens t = Theme::tokensFor(id);
            QCOMPARE(t.accent.name(), QStringLiteral("#ff7a1a"));
            QCOMPARE(t.accentHover.name(), QStringLiteral("#ff9447"));
            QCOMPARE(t.accentPress.name(), QStringLiteral("#e5650a"));
            QCOMPARE(t.accentSoft.rgb(), t.accent.rgb());
            QCOMPARE(t.accentSoft.alpha(), qRound(0.18 * 255));
            QCOMPARE(t.warning.name(), QStringLiteral("#ffc53d"));
            QCOMPARE(t.danger.name(), QStringLiteral("#ff4d4f"));
            QCOMPARE(t.success.name(), QStringLiteral("#3dd68c"));
        }
    }
    void qssHasNoColourLiterals() {
        static const QRegularExpression hex(QStringLiteral("#[0-9A-Fa-f]{3,8}\\b"));
        static const QRegularExpression rgb(QStringLiteral("\\brgba?\\("));
        for (const char* name : {":/themes/app.qss", ":/themes/navy.qss", ":/themes/black.qss"}) {
            QFile f(QString::fromLatin1(name));
            QVERIFY2(f.open(QIODevice::ReadOnly), name);
            const QString qss = QString::fromUtf8(f.readAll());
            QVERIFY2(!hex.match(qss).hasMatch(), qPrintable(QStringLiteral("hex colour in %1").arg(name)));
            QVERIFY2(!rgb.match(qss).hasMatch(), qPrintable(QStringLiteral("rgb() in %1").arg(name)));
        }
    }
    void expansionLeavesNoToken() {
        for (ThemeId id : {ThemeId::Navy, ThemeId::Black}) {
            QStringList unknown;
            const QString base = QStringLiteral("a { color: @accent; background: @accentSoft; border: @accentLine; }");
            const QString out = Theme::expand(base, Theme::tokensFor(id), &unknown);
            QVERIFY(unknown.isEmpty());
            QVERIFY(!out.contains(QLatin1Char('@')));
            QVERIFY(out.contains(QStringLiteral("#ff7a1a")));
            QVERIFY(out.contains(QStringLiteral("rgba(255, 122, 26, 46)")));
            const QString sheet = Theme::styleSheetFor(id);
            QVERIFY(!sheet.isEmpty());
            QVERIFY2(!sheet.contains(QLatin1Char('@')), "a token was left in the stylesheet");
        }
    }
    void unknownTokensAreReported() {
        QStringList unknown;
        Theme::expand(QStringLiteral("x { color: @nope; }"), Theme::tokensFor(ThemeId::Navy), &unknown);
        QCOMPARE(unknown, QStringList{QStringLiteral("nope")});
    }
    void switchesWithoutRestart() {
        QSignalSpy spy(&Theme::instance(), &Theme::changed);
        Theme::instance().apply(ThemeId::Black);
        QCOMPARE(Theme::instance().id(), ThemeId::Black);
        QCOMPARE(qApp->palette().color(QPalette::Window).name(), QStringLiteral("#0b0b0c"));
        Theme::instance().apply(ThemeId::Navy);
        QCOMPARE(qApp->palette().color(QPalette::Window).name(), QStringLiteral("#0e1726"));
        QCOMPARE(spy.count(), 2);
    }
    void namesRoundTrip() {
        QCOMPARE(Theme::idFromName(QStringLiteral("black")), ThemeId::Black);
        QCOMPARE(Theme::idFromName(QStringLiteral("Navy")), ThemeId::Navy);
        QCOMPARE(Theme::idFromName(QStringLiteral("unknown")), ThemeId::Navy);
        QCOMPARE(Theme::idName(ThemeId::Black), QStringLiteral("black"));
    }
};

QTEST_MAIN(TestTheme)
#include "test-theme.moc"
