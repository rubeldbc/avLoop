#include "core/AspectRatio.h"

#include <cmath>
#include <numeric>

namespace avloop::aspect {

const QStringList& presets() {
    static const QStringList ids = {QStringLiteral("free"), QStringLiteral("original"), QStringLiteral("1:1"),
                                    QStringLiteral("4:3"),  QStringLiteral("3:4"),      QStringLiteral("16:9"),
                                    QStringLiteral("9:16"), QStringLiteral("21:9"),     QStringLiteral("3:2"),
                                    QStringLiteral("2:3"),  QStringLiteral("4:5")};
    return ids;
}

bool isCustom(const QString& id) {
    return !presets().contains(id) && id.contains(QLatin1Char(':'));
}

QString custom(double w, double h) {
    if (w <= 0 || h <= 0) return {};
    return QStringLiteral("%1:%2").arg(w, 0, 'g', 6).arg(h, 0, 'g', 6);
}

QString reduced(double w, double h) {
    const long a = std::lround(w);
    const long b = std::lround(h);
    if (a <= 0 || b <= 0) return {};
    const long g = std::gcd(a, b);
    return custom(double(a / g), double(b / g));
}

double value(const QString& id, const QSizeF& video) {
    if (id == QLatin1String("original")) return video.height() > 0 ? video.width() / video.height() : 0;
    const qsizetype colon = id.indexOf(QLatin1Char(':'));
    if (colon < 0) return 0; // Free
    const double w = id.left(colon).toDouble();
    const double h = id.mid(colon + 1).toDouble();
    return w > 0 && h > 0 ? w / h : 0;
}

QRectF apply(const QRectF& rect, double ratio) {
    if (ratio <= 0 || rect.isEmpty()) return rect;
    const double area = rect.width() * rect.height();
    const double w = std::sqrt(area * ratio);
    const double h = std::sqrt(area / ratio);
    const QPointF c = rect.center();
    return {c.x() - w / 2, c.y() - h / 2, w, h};
}

} // namespace avloop::aspect
