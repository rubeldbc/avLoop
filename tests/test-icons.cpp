// The icons (spec §5.3): every listed icon exists and renders in its four states at 1x and 2x, each state in its
// colour — normal text2, hover text, active accent, disabled 40%.
#include "ui/theme/IconFactory.h"
#include "ui/theme/Theme.h"

#include <QImage>
#include <QTest>

using namespace avloop;

namespace {

// The colour of the most opaque pixel: the stroke's own colour, free of anti-aliasing.
QColor strokeColour(const QPixmap& pm) {
    const QImage img = pm.toImage().convertToFormat(QImage::Format_ARGB32);
    QRgb best = 0;
    for (int y = 0; y < img.height(); ++y)
        for (int x = 0; x < img.width(); ++x)
            if (qAlpha(img.pixel(x, y)) > qAlpha(best)) best = img.pixel(x, y);
    return QColor::fromRgba(best);
}

} // namespace

class TestIcons : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() { Theme::instance().apply(ThemeId::Navy); }

    void listMatchesTheSpec() {
        QCOMPARE(IconFactory::allNames().size(), 49);
        QStringList names = IconFactory::allNames();
        QCOMPARE(names.removeDuplicates(), 0);
    }
    void everyIconRenders() {
        for (const QString& name : IconFactory::allNames()) {
            QVERIFY2(IconFactory::exists(name), qPrintable(name));
            for (qreal dpr : {1.0, 2.0}) {
                const QPixmap pm = IconFactory::pixmap(name, QSize(20, 20), dpr, QIcon::Normal, QIcon::Off);
                QCOMPARE(pm.width(), qRound(20 * dpr));
                QVERIFY2(strokeColour(pm).alpha() > 100, qPrintable(name + QStringLiteral(" is blank")));
            }
        }
    }
    void statesHaveTheirColours() {
        const ThemeTokens& t = Theme::instance().tokens();
        const QString name = QStringLiteral("tool-crop-ellipse");
        auto at = [&](QIcon::Mode mode, QIcon::State state) {
            return strokeColour(IconFactory::pixmap(name, QSize(24, 24), 2.0, mode, state));
        };
        QCOMPARE(at(QIcon::Normal, QIcon::Off).rgb(), t.text2.rgb());
        QCOMPARE(at(QIcon::Active, QIcon::Off).rgb(), t.text.rgb());
        QCOMPARE(at(QIcon::Normal, QIcon::On).rgb(), t.accent.rgb());
        const QColor disabled = at(QIcon::Disabled, QIcon::Off);
        QVERIFY2(qAbs(disabled.alpha() - qRound(0.4 * 255)) <= 3, qPrintable(QString::number(disabled.alpha())));
    }
    void themeChangeRecolours() {
        const QString name = QStringLiteral("play");
        Theme::instance().apply(ThemeId::Black);
        const QColor black = strokeColour(IconFactory::pixmap(name, QSize(20, 20), 1.0, QIcon::Normal, QIcon::Off));
        Theme::instance().apply(ThemeId::Navy);
        const QColor navy = strokeColour(IconFactory::pixmap(name, QSize(20, 20), 1.0, QIcon::Normal, QIcon::Off));
        QCOMPARE(black.rgb(), Theme::tokensFor(ThemeId::Black).text2.rgb());
        QCOMPARE(navy.rgb(), Theme::tokensFor(ThemeId::Navy).text2.rgb());
    }
    void appIconHasEverySize() {
        const QIcon icon = IconFactory::appIcon();
        QCOMPARE(icon.availableSizes().size(), 7);
        QVERIFY(!icon.pixmap(QSize(256, 256)).isNull());
    }
};

QTEST_MAIN(TestIcons)
#include "test-icons.moc"
