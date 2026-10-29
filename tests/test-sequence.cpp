// The sequence (D-031): a clip's own time and keyed look; the edits — the Razor, trims, moves, removes, ripple, insert,
// overwrite, tracks — and their refusals; the file's form; an AvLoop project (a crop, an inverse, shapes, keyframes,
// layers) migrated.
#include "core/ProjectJson.h"
#include "core/SequenceEdit.h"

#include <QTest>

using namespace avloop;
namespace se = avloop::sequenceedit;

namespace {

Clip media(double duration, QSizeF size = {640, 360}) {
    Clip c;
    c.name = QStringLiteral("m");
    c.path = QStringLiteral("D:/m.mp4");
    c.size = size;
    c.duration = duration;
    c.range = {0, duration};
    c.transform = VideoTransform::initial(size);
    return c;
}

Sequence twoTracks() {
    Sequence s;
    s.tracks = {{QStringLiteral("V1"), true, false, {}}, {QStringLiteral("V2"), true, false, {}}};
    return s;
}

} // namespace

class TestSequence : public QObject {
    Q_OBJECT
private slots:
    void ownTimeAndLook() {
        Clip c = media(10);
        c.start = 2;
        c.range = {1, 5};
        double own = 0;
        QVERIFY(c.ownTime(2, &own));
        QCOMPARE(own, 1.0);
        QVERIFY(c.ownTime(5.5, &own));
        QCOMPARE(own, 4.5);
        QVERIFY(!c.ownTime(6, nullptr)); // its end is not shown
        c.keys = {{1, 0, 0, 1, 100, false}, {3, 100, 0, 1, 0, false}};
        QCOMPARE(c.look(2).transform.x, 50.0);
        QCOMPARE(c.look(2).opacity, 50.0);
    }
    void razorAndTrims() {
        Sequence s = twoTracks();
        int a = 0;
        QVERIFY(se::insert(s, 0, 0, media(10), &a));
        int b = 0;
        QVERIFY(se::split(s, a, 4, &b));
        QCOMPARE(s.tracks[0].clips.size(), 2);
        QCOMPARE(se::clip(s, a)->range.out, 4.0);
        QCOMPARE(se::clip(s, b)->start, 4.0);
        QCOMPARE(se::clip(s, b)->range.in, 4.0);
        QVERIFY(!se::split(s, a, 4)); // at its end: nothing to cut
        QVERIFY(se::trimOut(s, a, 3));
        QCOMPARE(se::clip(s, a)->length(), 3.0);
        QVERIFY(!se::trimIn(s, b, 2.5)); // over a: refused
        QVERIFY(se::trimIn(s, b, 5));
        QCOMPARE(se::clip(s, b)->range.in, 5.0);
        QVERIFY(!se::trimOut(s, b, 12)); // beyond its media
        QCOMPARE(se::splitAll(s, 7), 1);
    }
    void movesRemovesRipple() {
        Sequence s = twoTracks();
        int a = 0, b = 0, c = 0;
        QVERIFY(se::insert(s, 0, 0, media(2), &a));
        QVERIFY(se::insert(s, 0, 2, media(3), &b));
        QVERIFY(se::insert(s, 0, 5, media(1), &c));
        QVERIFY(!se::move(s, c, 1, 0)); // over a
        QVERIFY(se::move(s, c, 1, 1));  // to V2
        QCOMPARE(se::find(s, c).track, 1);
        QVERIFY(se::rippleRemove(s, a));
        QCOMPARE(se::clip(s, b)->start, 0.0); // moved left by a's length
        QCOMPARE(se::clip(s, c)->start, 1.0); // another track: as it was
        QVERIFY(se::remove(s, b));
        QCOMPARE(s.tracks[0].clips.size(), 0);
        s.tracks[1].locked = true;
        QVERIFY(!se::remove(s, c)); // a locked track
    }
    void insertAndOverwrite() {
        Sequence s = twoTracks();
        int a = 0, b = 0;
        QVERIFY(se::insert(s, 0, 0, media(10), &a));
        QVERIFY(se::insert(s, 0, 4, media(2), &b)); // between a's halves: a cut at 4, its right half moved to 6
        QCOMPARE(s.tracks[0].clips.size(), 3);
        QCOMPARE(s.tracks[0].clips[2].start, 6.0);
        QCOMPARE(s.duration(), 12.0);
        Sequence o = twoTracks();
        int x = 0;
        QVERIFY(se::insert(o, 0, 0, media(10), &x));
        QVERIFY(se::overwrite(o, 0, 4, media(2)));
        QCOMPARE(o.tracks[0].clips.size(), 3);
        QCOMPARE(o.duration(), 10.0); // nothing pushed
        QCOMPARE(o.tracks[0].clips[2].start, 6.0);
        QCOMPARE(o.tracks[0].clips[2].range.in, 6.0);
    }
    void tracksAndHits() {
        Sequence s = twoTracks();
        QVERIFY(se::addTrack(s));
        QCOMPARE(s.tracks.size(), 3);
        int top = 0;
        Clip small = media(5, {100, 100});
        small.transform.x = 50;
        small.transform.y = 50;
        QVERIFY(se::insert(s, 0, 0, media(5), nullptr));
        QVERIFY(se::insert(s, 2, 0, small, &top));
        QCOMPARE(se::topClipAt(s, {50, 50}, 1), top);
        QVERIFY(se::topClipAt(s, {300, 200}, 1) != top);
        QVERIFY(se::removeTrack(s, 2));
        QVERIFY(!se::removeTrack(s, 0));
        QCOMPARE(s.tracks.size(), 2);
    }
    void fileForm() {
        Sequence s = twoTracks();
        Clip c = media(4);
        c.keys = {{0, 1, 2, 1, 100, true}};
        c.masks = {MaskShape{}};
        c.effects.sepia = 30;
        QVERIFY(se::insert(s, 1, 1.5, c));
        s.inOut = {1, 3};
        s.frame = {1080, 1920};
        s.matte.kind = CropShape::Kind::Ellipse;
        s.matte.rect = QRectF(10, 20, 300, 200);
        s.matte.invert = true;
        s.matte.feather = 4;
        s.matte.shapes = {MaskShape{}};
        QCOMPARE(Sequence::fromJson(s.toJson()), s);
    }
    // AvLoop's project as a sequence: an ellipse crop with a feather and a shape, keyframes, a layer.
    void fromAvLoop() {
        ProjectData d;
        d.source.path = QStringLiteral("D:/clips/fire.mp4");
        d.source.width = 640;
        d.source.height = 360;
        d.source.duration = 10;
        d.source.fps = {30, 1};
        d.range = {1, 4};
        d.transform = VideoTransform::initial({640, 360}); // centre (320, 180), scale 1
        d.crop.kind = CropShape::Kind::Ellipse;
        d.crop.rect = QRectF(100, 50, 200, 100);
        d.crop.feather = 10;
        MaskShape cut;
        cut.role = MaskShape::Role::Cut;
        cut.rect = QRectF(120, 60, 20, 20);
        d.crop.shapes = {cut};
        d.keyframes = {{2, 400, 200, 1.5, true}};
        VideoLayer l;
        l.name = QStringLiteral("b");
        l.path = QStringLiteral("D:/clips/b.mp4");
        l.size = QSizeF(320, 180);
        l.duration = 5;
        l.range = {0, 5};
        l.track = 3;
        l.transform.x = 200;
        l.transform.y = 100;
        d.layers = {l};
        const Sequence s = Sequence::fromAvLoop(d);
        QCOMPARE(s.frame, QSize(200, 100));
        QCOMPARE(s.inOut.in, 1.0);
        QCOMPARE(s.tracks.size(), 3);
        const Clip& main = s.tracks[0].clips.first();
        QCOMPARE(main.transform.x, 220.0); // 320 − the frame's left 100
        QCOMPARE(main.keys.first().x, 300.0);
        QVERIFY(main.masks.isEmpty());
        QCOMPARE(s.matte.rect, QRectF(0, 0, 200, 100)); // the crop at the frame's place: over everything (RS-10)
        QCOMPARE(s.matte.kind, CropShape::Kind::Ellipse);
        QCOMPARE(s.matte.feather, 10.0);
        QCOMPARE(s.matte.shapes.first().rect, QRectF(20, 10, 20, 20));
        QCOMPARE(s.tracks[2].clips.first().transform.x, 100.0);
        d.crop.invert = true;
        const Sequence hole = Sequence::fromAvLoop(d);
        // An inverse: the whole video where it stands at In — held at the first key (400, 200, ×1.5): 960×540 from
        // (−80, −70).
        QCOMPARE(hole.frame, QSize(960, 540));
        QVERIFY(hole.matte.invert);
        QCOMPARE(hole.matte.rect, QRectF(180, 120, 200, 100));
        QCOMPARE(hole.tracks[0].clips.first().keys.first().x, 480.0);
        // The file carries it, and reads it back.
        const QJsonObject json = projectjson::toJson(d);
        QCOMPARE(json.value(QStringLiteral("version")).toInt(), 4);
        QCOMPARE(Sequence::fromJson(json.value(QStringLiteral("sequence")).toObject()), hole);
        // The project's own (RS-9: made from its fields until AS-003).
        Project p;
        p.restore(d);
        QCOMPARE(p.sequence(), hole);
    }
};

QTEST_GUILESS_MAIN(TestSequence)
#include "test-sequence.moc"
