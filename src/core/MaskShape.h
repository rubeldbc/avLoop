#pragma once

#include <QJsonObject>
#include <QList>
#include <QPainterPath>
#include <QPointF>
#include <QRectF>
#include <QString>

namespace avloop {

// One anchor of a custom shape (D-022): its point and the handles before and after it — the point itself for a
// corner without handles (a polygon's, a freehand outline's); `smooth` keeps the two handles in line (the Pen, AV-025).
struct PathPoint {
    QPointF p;
    QPointF in;
    QPointF out;
    bool smooth = false;

    static PathPoint corner(const QPointF& at) { return {at, at, at, false}; }
    bool hasHandles() const { return in != p || out != p; }
    friend bool operator==(const PathPoint&, const PathPoint&) = default;
};

// A shape inside the crop's frame (D-022): standard (a rectangle, a rounded rectangle, an ellipse) by its rectangle,
// custom (a polygon, a freehand outline, a Pen path) by its anchors, all in scene px, never keyframed. Its role says
// how it joins the shapes below it (R-82, R-88): Add keeps its inside, Cut clears it, KeepOnly keeps only where both
// are, Exclude where one is and not both. The project's shapes are drawn bottom (first) to top (last).
struct MaskShape {
    enum class Kind { Rect, Rounded, Ellipse, Polygon, Path };
    enum class Role { Add, Cut, KeepOnly, Exclude };
    static constexpr int kMaxShapes = 64; // AVADR-0002: a bounded list
    static constexpr double kMaxFeather = 200;

    Kind kind = Kind::Rect;
    Role role = Role::Add;
    QRectF rect;             // the standard kinds
    double radius = 0;       // Rounded, scene px
    QList<PathPoint> points; // Polygon (corners), Path (anchors with handles)
    QString name;
    bool visible = true;
    bool locked = false;
    double feather = 0;    // output px, softening its edge (AV-027)
    double strength = 100; // %, how strongly it applies (AV-027)

    bool isStandard() const { return kind == Kind::Rect || kind == Kind::Rounded || kind == Kind::Ellipse; }
    // The outline in scene px: a closed path (a custom shape's segments cubic where it has handles).
    QPainterPath path() const;
    QRectF bounds() const;
    void translate(const QPointF& d);
    MaskShape clamped() const;

    QJsonObject toJson() const;
    static MaskShape fromJson(const QJsonObject& json);
    static QString kindId(Kind k); // "rect", "rounded", "ellipse", "polygon", "path"
    static Kind kindFromId(const QString& id);
    static QString roleId(Role r); // "add", "cut", "keep", "exclude"
    static Role roleFromId(const QString& id);

    friend bool operator==(const MaskShape&, const MaskShape&) = default;
};

namespace maskshapes {

// The running coverage after one shape of coverage `s` (0 … 1) joins `r` by its role (D-022, D-023).
inline double join(MaskShape::Role role, double r, double s) {
    switch (role) {
    case MaskShape::Role::Cut:
        return r * (1 - s);
    case MaskShape::Role::KeepOnly:
        return r * s;
    case MaskShape::Role::Exclude:
        return r + s - 2 * r * s;
    case MaskShape::Role::Add:
        break;
    }
    return r > s ? r : s;
}
// Where the coverage starts: nothing when a visible shape adds (only what shapes add is kept), else the whole frame.
double start(const QList<MaskShape>& shapes);
bool anyVisible(const QList<MaskShape>& shapes);
// A freehand outline's points made fewer (Douglas–Peucker within `tolerance` scene px), the first and last kept.
QList<QPointF> simplified(const QList<QPointF>& points, double tolerance);
// The default name of a new shape: "Rectangle 3" — its kind's word and the next free number.
QString nextName(const QList<MaskShape>& shapes, MaskShape::Kind kind);

} // namespace maskshapes

} // namespace avloop
