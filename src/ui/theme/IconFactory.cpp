#include "ui/theme/IconFactory.h"

#include "ui/theme/Theme.h"

#include <QFile>
#include <QHash>
#include <QIconEngine>
#include <QPainter>
#include <QSvgRenderer>

namespace avloop {

namespace {

QHash<QString, QPixmap>& pixmapCache() {
    static QHash<QString, QPixmap> cache;
    return cache;
}

QHash<QString, QByteArray>& svgCache() {
    static QHash<QString, QByteArray> cache;
    return cache;
}

QByteArray svgOf(const QString& name) {
    auto& cache = svgCache();
    auto it = cache.find(name);
    if (it != cache.end()) return *it;
    QFile f(QStringLiteral(":/icons/%1.svg").arg(name));
    const QByteArray data = f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
    cache.insert(name, data);
    return data;
}

QColor colorFor(IconTone tone, QIcon::Mode mode, QIcon::State state) {
    const ThemeTokens& t = Theme::instance().tokens();
    QColor c;
    switch (tone) {
    case IconTone::OnAccent:
        c = t.onAccent;
        break;
    case IconTone::Accent:
        c = t.accent;
        break;
    case IconTone::Warning:
        c = t.warning;
        break;
    case IconTone::Danger:
        c = t.danger;
        break;
    case IconTone::Success:
        c = t.success;
        break;
    case IconTone::Text:
        c = t.text;
        break;
    case IconTone::Default:
        if (state == QIcon::On)
            c = t.accent;
        else if (mode == QIcon::Active || mode == QIcon::Selected)
            c = t.text;
        else
            c = t.text2;
        break;
    }
    if (mode == QIcon::Disabled) c.setAlphaF(0.4f);
    return c;
}

// Renders on demand at the size and scale asked, so the icon stays sharp on any DPI (spec §12.3).
class ThemedIconEngine : public QIconEngine {
public:
    ThemedIconEngine(QString name, IconTone tone) : name_(std::move(name)), tone_(tone) {}

    void paint(QPainter* painter, const QRect& rect, QIcon::Mode mode, QIcon::State state) override {
        const qreal dpr = painter->device() ? painter->device()->devicePixelRatioF() : 1.0;
        painter->drawPixmap(rect, IconFactory::pixmap(name_, rect.size(), dpr, mode, state, tone_));
    }
    QPixmap pixmap(const QSize& size, QIcon::Mode mode, QIcon::State state) override {
        return IconFactory::pixmap(name_, size, 1.0, mode, state, tone_);
    }
    QPixmap scaledPixmap(const QSize& size, QIcon::Mode mode, QIcon::State state, qreal scale) override {
        return IconFactory::pixmap(name_, size, scale, mode, state, tone_);
    }
    QIconEngine* clone() const override { return new ThemedIconEngine(name_, tone_); }
    QString key() const override { return QStringLiteral("avloop-themed"); }
    QString iconName() override { return name_; }
    bool isNull() override { return svgOf(name_).isEmpty(); }

private:
    QString name_;
    IconTone tone_;
};

} // namespace

QIcon IconFactory::icon(const QString& name, IconTone tone) {
    return QIcon(new ThemedIconEngine(name, tone));
}

QPixmap IconFactory::pixmap(const QString& name, const QSize& size, qreal dpr, QIcon::Mode mode, QIcon::State state,
                            IconTone tone) {
    const QColor color = colorFor(tone, mode, state);
    const QString key = QStringLiteral("%1|%2x%3|%4|%5")
                            .arg(name)
                            .arg(size.width())
                            .arg(size.height())
                            .arg(dpr)
                            .arg(color.name(QColor::HexArgb));
    auto& cache = pixmapCache();
    if (auto it = cache.find(key); it != cache.end()) return *it;

    QByteArray svg = svgOf(name);
    QColor opaque = color;
    opaque.setAlpha(255);
    svg.replace("currentColor", opaque.name(QColor::HexRgb).toLatin1());
    const QSize px(qMax(1, qRound(size.width() * dpr)), qMax(1, qRound(size.height() * dpr)));
    QImage image(px, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    {
        QSvgRenderer renderer(svg);
        QPainter p(&image);
        p.setRenderHint(QPainter::Antialiasing);
        if (color.alpha() < 255) p.setOpacity(color.alphaF());
        renderer.render(&p);
    }
    QPixmap result = QPixmap::fromImage(image);
    result.setDevicePixelRatio(dpr);
    cache.insert(key, result);
    return result;
}

QIcon IconFactory::appIcon() {
    QIcon icon;
    QSvgRenderer renderer(QStringLiteral(":/app/avloop.svg"));
    for (int s : {16, 24, 32, 48, 64, 128, 256}) {
        QImage image(s, s, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        QPainter p(&image);
        p.setRenderHint(QPainter::Antialiasing);
        renderer.render(&p);
        p.end();
        icon.addPixmap(QPixmap::fromImage(image));
    }
    return icon;
}

const QStringList& IconFactory::allNames() {
    // icons:begin — spec §5.3's table, group by group.
    static const QStringList names = {
        // Tools
        QStringLiteral("tool-select"),
        QStringLiteral("tool-crop-rect"),
        QStringLiteral("tool-crop-round"),
        QStringLiteral("tool-crop-ellipse"),
        QStringLiteral("tool-hand"),
        QStringLiteral("tool-zoom"),
        QStringLiteral("tool-in"),
        QStringLiteral("tool-out"),
        QStringLiteral("tool-keyframe"),
        // Top bar
        QStringLiteral("menu"),
        QStringLiteral("open"),
        QStringLiteral("preview"),
        QStringLiteral("export"),
        // Transport
        QStringLiteral("play"),
        QStringLiteral("pause"),
        QStringLiteral("frame-prev"),
        QStringLiteral("frame-next"),
        QStringLiteral("go-in"),
        QStringLiteral("go-out"),
        QStringLiteral("loop-on"),
        QStringLiteral("loop-off"),
        // Options bar
        QStringLiteral("fit"),
        QStringLiteral("fill"),
        QStringLiteral("center"),
        QStringLiteral("flip-h"),
        QStringLiteral("flip-v"),
        QStringLiteral("lock"),
        QStringLiteral("unlock"),
        QStringLiteral("reset"),
        QStringLiteral("zoom-fit"),
        QStringLiteral("zoom-100"),
        QStringLiteral("kf-prev"),
        QStringLiteral("kf-next"),
        QStringLiteral("kf-delete"),
        // Other
        QStringLiteral("undo"),
        QStringLiteral("redo"),
        QStringLiteral("settings"),
        QStringLiteral("batch"),
        QStringLiteral("finder"),
        QStringLiteral("warning"),
        QStringLiteral("check"),
        QStringLiteral("close"),
        QStringLiteral("chevron"),
        QStringLiteral("eyedropper"),
        QStringLiteral("invert"),
        QStringLiteral("tool-shape"),
        QStringLiteral("shape-polygon"),
        QStringLiteral("shape-freehand"),
        QStringLiteral("shape-cut"),
        QStringLiteral("tool-pen"),
        QStringLiteral("role-add"),
        QStringLiteral("role-keep"),
        QStringLiteral("role-exclude"),
        QStringLiteral("align-left"),
        QStringLiteral("align-centre"),
        QStringLiteral("align-right"),
        QStringLiteral("align-top"),
        QStringLiteral("align-middle"),
        QStringLiteral("align-bottom"),
        QStringLiteral("distribute-h"),
        QStringLiteral("distribute-v"),
    };
    // icons:end
    return names;
}

bool IconFactory::exists(const QString& name) {
    const QByteArray svg = svgOf(name);
    return !svg.isEmpty() && QSvgRenderer(svg).isValid();
}

void IconFactory::clearCache() {
    pixmapCache().clear();
}

} // namespace avloop
