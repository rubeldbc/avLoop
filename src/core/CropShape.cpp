#include "core/CropShape.h"

#include <algorithm>

namespace avloop {

CropShape CropShape::clamped() const {
    CropShape c = *this;
    c.rect = rect.normalized();
    c.feather = std::clamp(feather, 0.0, kMaxFeather);
    c.radius = std::clamp(radius, 0.0, c.maxRadius());
    return c;
}

QString CropShape::kindId(Kind kind) {
    switch (kind) {
    case Kind::Rounded:
        return QStringLiteral("rounded");
    case Kind::Ellipse:
        return QStringLiteral("ellipse");
    case Kind::Rect:
        break;
    }
    return QStringLiteral("rect");
}

CropShape::Kind CropShape::kindFromId(const QString& id) {
    if (id == QLatin1String("rounded")) return Kind::Rounded;
    if (id == QLatin1String("ellipse")) return Kind::Ellipse;
    return Kind::Rect;
}

QString CropShape::toolId(Kind kind) {
    switch (kind) {
    case Kind::Rounded:
        return QStringLiteral("crop-round");
    case Kind::Ellipse:
        return QStringLiteral("crop-ellipse");
    case Kind::Rect:
        break;
    }
    return QStringLiteral("crop-rect");
}

} // namespace avloop
