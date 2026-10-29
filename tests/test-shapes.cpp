// The shapes' model and the Shape tool's geometry (D-022): outlines, the file's form, a freehand outline simplified,
// names, hit-testing, the walks' summary.
#include "core/MaskShape.h"
#include "ui/tools/ShapeEdit.h"

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
