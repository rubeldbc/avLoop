#pragma once

#include <QRectF>
#include <QString>

#include <algorithm>

namespace avloop {

// The crop (spec §3.3, §6.1): a rectangle in scene space with a kind, a corner radius (scene px, up to half the short
// side), a feather (output px, 0–200) and its aspect ratio id. An empty rectangle is no crop: the whole video is
// exported (reading R-8). Inverted (D-021), the shape is a hole: the whole video is exported and the shape is clear in
// it, the feather softening outward.
struct CropShape {
    enum class Kind { Rect, Rounded, Ellipse };
    static constexpr double kMaxFeather = 200;
    static constexpr double kMinSide = 16; // a smaller drawing is cancelled (spec §12.2)

    Kind kind = Kind::Rect;
    QRectF rect;
    double radius = 0;
    double feather = 0;
    QString ratio = QStringLiteral("free");
    bool invert = false; // the chosen area cut out of the whole video (R-79)

    bool isEmpty() const { return rect.width() <= 0 || rect.height() <= 0; }
    double maxRadius() const { return isEmpty() ? 0 : std::min(rect.width(), rect.height()) / 2; }
    CropShape clamped() const;

    static QString kindId(Kind kind);          // "rect", "rounded", "ellipse"
    static Kind kindFromId(const QString& id); // unknown → Rect
    static QString toolId(Kind kind);          // "crop-rect", "crop-round", "crop-ellipse"

    friend bool operator==(const CropShape& a, const CropShape& b) = default;
};

} // namespace avloop
