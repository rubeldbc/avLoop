// The shapes' model and the Shape tool's geometry (D-022): outlines, the file's form, a freehand outline simplified,
// names, hit-testing, the walks' summary.
#include "core/MaskShape.h"
#include "core/ShapeAlign.h"
#include "core/ShapeCombine.h"
#include "ui/tools/PathEdit.h"
#include "ui/tools/ShapeEdit.h"

#include <QLineF>
#include <QTest>

using namespace avloop;

class TestShapes : public QObject {
    Q_OBJECT
private slots:
    void outlines() {
        MaskShape r;
        r.rect = QRectF(10, 10, 100, 50);
        QVERIFY(r.path().contains(QPointF(50, 30)));
        QVERIFY(!r.path().contains(QPointF(5, 30)));
        MaskShape e = r;
        e.kind = MaskShape::Kind::Ellipse;
        QVERIFY(e.path().contains(QPointF(60, 35)));
        QVERIFY(!e.path().contains(QPointF(12, 12))); // the box's corner is outside the ellipse
        const MaskShape tri = shapeedit::polygon({{0, 0}, {100, 0}, {0, 100}}, MaskShape::Role::Cut);
        QVERIFY(tri.path().contains(QPointF(20, 20)));
        QVERIFY(!tri.path().contains(QPointF(80, 80)));
        QCOMPARE(tri.bounds(), QRectF(0, 0, 100, 100));
    }
    void fileForm() {
        MaskShape s = shapeedit::standard(ShapeDraft::Kind::Rounded, MaskShape::Role::Cut, QRectF(5, 6, 60, 30));
        s.name = QStringLiteral("Rounded 1");
        QCOMPARE(s.radius, 5.0); // a sixth of the short side
        QCOMPARE(MaskShape::fromJson(s.toJson()), s);
        MaskShape p = shapeedit::polygon({{1, 2}, {30, 4}, {9, 40}}, MaskShape::Role::Add);
        p.points[1].out = QPointF(35, 10);
        p.points[1].smooth = true;
        p.kind = MaskShape::Kind::Path;
        p.visible = false;
        QCOMPARE(MaskShape::fromJson(p.toJson()), p);
        QCOMPARE(MaskShape::roleFromId(QStringLiteral("cut")), MaskShape::Role::Cut);
        QCOMPARE(MaskShape::roleFromId(QStringLiteral("unknown")), MaskShape::Role::Add);
    }
    void simplify() {
        QList<QPointF> line;
        for (int i = 0; i <= 100; ++i)
            line << QPointF(i, i % 2 == 0 ? 0.0 : 0.3); // a straight line with a 0.3 px tremble
        line << QPointF(100, 50);
        const QList<QPointF> fewer = maskshapes::simplified(line, 1.0);
        QCOMPARE(fewer.size(), 3);
        QCOMPARE(fewer.first(), QPointF(0, 0));
        QCOMPARE(fewer.last(), QPointF(100, 50));
    }
    void namesAndHits() {
        QList<MaskShape> shapes;
        MaskShape a = shapeedit::standard(ShapeDraft::Kind::Rect, MaskShape::Role::Add, QRectF(0, 0, 100, 100));
        a.name = maskshapes::nextName(shapes, a.kind);
        shapes << a;
        QCOMPARE(a.name, QStringLiteral("Rectangle 1"));
        QCOMPARE(maskshapes::nextName(shapes, MaskShape::Kind::Rect), QStringLiteral("Rectangle 2"));
        MaskShape b = shapeedit::standard(ShapeDraft::Kind::Ellipse, MaskShape::Role::Cut, QRectF(50, 50, 100, 100));
        shapes << b;
        QCOMPARE(shapeedit::hitShape(shapes, QPointF(90, 90)), 1); // the topmost
        QCOMPARE(shapeedit::hitShape(shapes, QPointF(10, 10)), 0);
        QCOMPARE(shapeedit::hitShape(shapes, QPointF(300, 300)), -1);
        shapes[1].locked = true;
        QCOMPARE(shapeedit::hitShape(shapes, QPointF(90, 90)), 0); // a locked shape is passed by
        QCOMPARE(shapeedit::summary(shapes), QStringLiteral("rect:add:0,0,100,100;ellipse:cut:50,50,100,100"));
        QCOMPARE(shapeedit::moved(shapes, {0}, {5, 5})[0].rect.topLeft(), QPointF(5, 5));
        QCOMPARE(shapeedit::removed(shapes, {0}).size(), 1);
        QCOMPARE(maskshapes::start(shapes), 0.0);
        QCOMPARE(maskshapes::start({b}), 1.0); // cuts alone cut from the whole frame
    }
    // The Pen's geometry (AV-025): an anchor added on a curve leaves the outline where it was; convert both ways; a
    // smooth handle turns its pair, Alt breaks it; a segment is hit where it runs, an anchor's end is not.
    void penGeometry() {
        MaskShape s = shapeedit::polygon({{100, 100}, {300, 100}, {300, 300}, {100, 300}}, MaskShape::Role::Add);
        s.kind = MaskShape::Kind::Path;
        s.points[0].out = QPointF(150, 20); // the top bulges up
        s.points[1].in = QPointF(250, 20);
        const QPointF mid = pathedit::at(s, 0, 0.5);
        QVERIFY(mid.y() < 50);
        int added = -1;
        const MaskShape more = pathedit::inserted(s, 0, 0.5, &added);
        QCOMPARE(added, 1);
        QCOMPARE(more.points.size(), 5);
        QVERIFY(QLineF(more.points[1].p, mid).length() < 1e-9);
        QVERIFY(more.points[1].smooth);
        for (double t : {0.25, 0.5, 0.75}) // the first half of the old curve is the new first segment
            QVERIFY(QLineF(pathedit::at(more, 0, t), pathedit::at(s, 0, t / 2)).length() < 1e-6);
        const MaskShape straight = pathedit::inserted(s, 2, 0.5, &added); // a straight side: a corner in its middle
        QCOMPARE(straight.points[3].p, QPointF(200, 300));
        QVERIFY(!straight.points[3].hasHandles());
        const MaskShape smooth = pathedit::converted(s, 2);
        QVERIFY(smooth.points[2].smooth);
        QVERIFY(smooth.points[2].in != smooth.points[2].p);
        QCOMPARE(pathedit::kinds(smooth), QStringLiteral("b,b,s,c"));
        QVERIFY(!pathedit::converted(smooth, 2).points[2].hasHandles());
        PathPoint pt = smooth.points[2];
        const double inLen = QLineF(pt.p, pt.in).length();
        pathedit::moveHandle(pt, true, pt.p + QPointF(0, 50), false);
        QVERIFY(QLineF(pt.in, pt.p + QPointF(0, -inLen)).length() < 1e-9); // opposite, its own length kept
        pathedit::moveHandle(pt, true, pt.p + QPointF(50, 0), true);
        QVERIFY(!pt.smooth);
        QVERIFY(QLineF(pt.in, pt.p + QPointF(0, -inLen)).length() < 1e-9); // the other untouched
        QCOMPARE(pathedit::removed(s, 0).points.size(), 3);
        const QTransform one;
        QCOMPARE(pathedit::hitSegment(s, one, QPointF(300, 200)).segment, 1);
        QCOMPARE(pathedit::hitSegment(s, one, QPointF(300, 300)).segment, -1); // the anchor, not a segment
        bool out = false;
        QCOMPARE(pathedit::hitHandle(s, one, QPointF(151, 21), &out), 0);
        QVERIFY(out);
        QCOMPARE(pathedit::asPath(shapeedit::polygon({{0, 0}, {1, 0}, {0, 1}}, MaskShape::Role::Add)).kind,
                 MaskShape::Kind::Path);
    }
    // Pathfinder (D-023) on two overlapping squares, A (0, 0, 100, 100) below B (50, 0, 100, 100): where the result is.
    void combine() {
        using shapecombine::Op;
        const MaskShape a = shapeedit::standard(ShapeDraft::Kind::Rect, MaskShape::Role::Cut, QRectF(0, 0, 100, 100));
        const MaskShape b = shapeedit::standard(ShapeDraft::Kind::Rect, MaskShape::Role::Add, QRectF(50, 0, 100, 100));
        const QPointF left(25, 50), both(75, 50), right(125, 50);
        auto inside = [](const MaskShape& s, QPointF p) { return s.path().contains(p); };
        struct Case {
            Op op;
            bool l, m, r;
        };
        for (const Case& c : {Case{Op::Unite, true, true, true}, Case{Op::MinusFront, true, false, false},
                              Case{Op::Intersect, false, true, false}, Case{Op::Exclude, true, false, true}}) {
            int at = -1;
            const QList<MaskShape> out = shapecombine::combined({a, b}, {0, 1}, c.op, &at);
            QCOMPARE(out.size(), 1);
            QCOMPARE(at, 0);
            const MaskShape& s = out.first();
            QCOMPARE(s.kind, MaskShape::Kind::Path);
            QCOMPARE(s.role, MaskShape::Role::Cut); // the lowest shape's role
            QCOMPARE(inside(s, left), c.l);
            QCOMPARE(inside(s, both), c.m);
            QCOMPARE(inside(s, right), c.r);
            QCOMPARE(MaskShape::fromJson(s.toJson()).path().contains(both), c.m); // the file keeps it
        }
        // Exclude of two squares: two outlines; a ring (minus front of a small square in a big one) has a hole.
        const MaskShape x = shapecombine::combined({a, b}, {0, 1}, Op::Exclude).first();
        QCOMPARE(x.outlines().size(), 2);
        const MaskShape inner =
            shapeedit::standard(ShapeDraft::Kind::Rect, MaskShape::Role::Add, QRectF(25, 25, 50, 50));
        const MaskShape ring = shapecombine::combined({a, inner}, {0, 1}, Op::MinusFront).first();
        QVERIFY(ring.path().contains(QPointF(10, 10)));
        QVERIFY(!ring.path().contains(QPointF(50, 50)));
        QCOMPARE(MaskShape::fromJson(ring.toJson()), ring);
        // Fewer than two: unchanged.
        int at = 0;
        QCOMPARE(shapecombine::combined({a, b}, {1}, Op::Unite, &at).size(), 2);
        QCOMPARE(at, -1);
        // A third shape between keeps its place; the result takes the lowest's.
        const QList<MaskShape> three = shapecombine::combined({b, a, inner}, {0, 2}, Op::Unite, &at);
        QCOMPARE(three.size(), 2);
        QCOMPARE(at, 0);
        QCOMPARE(three[1], a);
    }
    // Align and the layers (D-026): three boxes A (0, 0, 10 × 10), B (40, 20, 20 × 20), C (100, 50, 10 × 30).
    void alignAndLayers() {
        using shapealign::Align;
        const QList<MaskShape> s = {
            shapeedit::standard(ShapeDraft::Kind::Rect, MaskShape::Role::Add, QRectF(0, 0, 10, 10)),
            shapeedit::standard(ShapeDraft::Kind::Rect, MaskShape::Role::Add, QRectF(40, 20, 20, 20)),
            shapeedit::standard(ShapeDraft::Kind::Rect, MaskShape::Role::Add, QRectF(100, 50, 10, 30))};
        const QList<int> all = {0, 1, 2};
        QCOMPARE(shapealign::selectionBounds(s, all), QRectF(0, 0, 110, 80));
        QList<MaskShape> a = shapealign::aligned(s, all, Align::Left);
        QCOMPARE(a[1].rect.left(), 0.0);
        QCOMPARE(a[2].rect.left(), 0.0);
        QCOMPARE(a[1].rect.size(), QSizeF(20, 20)); // moved, never resized
        a = shapealign::aligned(s, all, Align::Bottom);
        QCOMPARE(a[0].rect.bottom(), 80.0);
        a = shapealign::aligned(s, all, Align::Centre);
        QCOMPARE(a[0].rect.center().x(), 55.0);
        a = shapealign::aligned(s, {1}, Align::Middle, QRectF(0, 0, 640, 360)); // one shape to the frame
        QCOMPARE(a[1].rect.center().y(), 180.0);
        QCOMPARE(a[0], s[0]);
        QList<MaskShape> locked = s;
        locked[2].locked = true;
        QCOMPARE(shapealign::aligned(locked, all, Align::Top)[2].rect.top(), 50.0); // a locked shape stays
        // Distribute: A and C stay, B's gaps equal — (110 − 10 − 20 − 10) / 2 = 35 → B at 45.
        const QList<MaskShape> d = shapealign::distributed(s, all, shapealign::Spread::Horizontal);
        QCOMPARE(d[1].rect.left(), 45.0);
        QCOMPARE(d[0].rect.left(), 0.0);
        QCOMPARE(d[2].rect.left(), 100.0);
        QCOMPARE(shapealign::distributed(s, {0, 1}, shapealign::Spread::Horizontal), s); // two: nothing
        // The layers: a move, a copy above its original.
        QCOMPARE(shapealign::moved(s, 0, 2)[2], s[0]);
        MaskShape named = s[1];
        named.name = QStringLiteral("Rectangle 2");
        const QList<MaskShape> dup = shapealign::duplicated({s[0], named}, 1);
        QCOMPARE(dup.size(), 3);
        QCOMPARE(dup[2].name, QStringLiteral("Rectangle 2 copy"));
        QCOMPARE(dup[2].rect.topLeft(), QPointF(50, 30));
    }
    void joins() {
        using R = MaskShape::Role;
        QCOMPARE(maskshapes::join(R::Add, 0.25, 0.5), 0.5);
        QCOMPARE(maskshapes::join(R::Cut, 1.0, 0.25), 0.75);
        QCOMPARE(maskshapes::join(R::KeepOnly, 0.5, 0.5), 0.25);
        QCOMPARE(maskshapes::join(R::Exclude, 1.0, 1.0), 0.0);
    }
};

QTEST_GUILESS_MAIN(TestShapes)
#include "test-shapes.moc"
