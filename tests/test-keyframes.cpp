// Keyframes (spec §6.2, §6.4, §11.1, §11.2, §12.2): the value between two keyframes, Linear and Ease (smoothstep) at
// u = ¼, ½, ¾; before the first and after the last; the flips hold; a keyframe per frame (replaced, a move refused
// where one is); the previous and the next; the project — the Selection writing into the keyframe at the playhead
// (made there if none), Undo, the last one deleted leaving the video where it is, a drag one step, Ease set; the file.
#include "core/Commands.h"
#include "core/Keyframes.h"
#include "core/Project.h"
#include "core/ProjectJson.h"

#include <QJsonArray>
#include <QTest>

using namespace avloop;

namespace {

SourceInfo source() {
    SourceInfo s;
    s.path = QStringLiteral("x.mp4");
    s.width = 640;
    s.height = 360;
    s.fps = {30, 1};
    s.duration = 10;
    return s;
}

VideoTransform base() {
    VideoTransform t;
    t.x = 100;
    t.y = 50;
    t.flipH = true;
    return t;
}

const Keyframes two = {{2.0, 100, 200, 1.0, false}, {4.0, 300, 400, 2.0, false}};

} // namespace

class TestKeyframes : public QObject {
    Q_OBJECT
private slots:
    void noKeyframesIsTheTransform() { QCOMPARE(keyframes::valueAt({}, base(), 3.0), base()); }
    void linearBetween_data() {
        QTest::addColumn<double>("t");
        QTest::addColumn<double>("x");
        QTest::addColumn<double>("scale");
        QTest::newRow("u ¼") << 2.5 << 150.0 << 1.25;
        QTest::newRow("u ½") << 3.0 << 200.0 << 1.5;
        QTest::newRow("u ¾") << 3.5 << 250.0 << 1.75;
        QTest::newRow("before the first") << 0.0 << 100.0 << 1.0;
        QTest::newRow("after the last (outside In–Out still counts)") << 9.0 << 300.0 << 2.0;
    }
    void linearBetween() {
        QFETCH(double, t);
        QFETCH(double, x);
        QFETCH(double, scale);
        const VideoTransform v = keyframes::valueAt(two, base(), t);
        QCOMPARE(v.x, x);
        QCOMPARE(v.scale, scale);
        QVERIFY(v.flipH); // the flips hold
    }
    void easeBetween_data() {
        QTest::addColumn<double>("t");
        QTest::addColumn<double>("x");
        // smoothstep: ¼ → 0.15625, ½ → 0.5, ¾ → 0.84375 of the way.
        QTest::newRow("u ¼") << 2.5 << 100 + 200 * 0.15625;
        QTest::newRow("u ½") << 3.0 << 200.0;
        QTest::newRow("u ¾") << 3.5 << 100 + 200 * 0.84375;
    }
    void easeBetween() {
        QFETCH(double, t);
        QFETCH(double, x);
        Keyframes eased = two;
        eased[0].ease = true; // the earlier keyframe's interpolation counts
        QCOMPARE(keyframes::valueAt(eased, base(), t).x, x);
        Keyframes lastEased = two;
        lastEased[1].ease = true; // the later one's does not
        QCOMPARE(keyframes::valueAt(lastEased, base(), 2.5).x, 150.0);
    }
    void onePerFrame() {
        const double half = 1.0 / 60;
        Keyframes k = keyframes::withKey(two, {3.0, 0, 0, 1, false}, half);
        QCOMPARE(k.size(), 3);
        QCOMPARE(k[1].t, 3.0);
        k = keyframes::withKey(k, {3.01, 9, 9, 1, true}, half); // within half a frame: the same keyframe
        QCOMPARE(k.size(), 3);
        QCOMPARE(k[1].x, 9.0);
        QCOMPARE(keyframes::moved(k, 0, 4.0, half), k);        // where one is: refused
        const Keyframes m = keyframes::moved(k, 0, 5.0, half); // past the others: kept in order
        QCOMPARE(m.size(), 3);
        QCOMPARE(m.back().t, 5.0);
        QCOMPARE(m.front().t, 3.01);
        QCOMPARE(keyframes::indexAt(m, 5.0 + half / 2, half), 2);
        QCOMPARE(keyframes::previous(m, 5.0, half), 4.0);
        QCOMPARE(keyframes::next(m, 3.01, half), 4.0);
        QCOMPARE(keyframes::next(m, 5.0, half), 5.0); // none after: where it is
    }
    void selectionWritesIntoKeyframes() {
        Project p;
        p.setSource(source());
        VideoTransform t = p.transform();
        t.x = 111;
        p.changeTransform(t); // no keyframes: the one transform
        QCOMPARE(p.baseTransform().x, 111.0);
        QVERIFY(p.keyframes().isEmpty());
        p.setTime(2.0);
        p.addKeyframe(); // K: the transform shown, at the playhead
        QCOMPARE(p.keyframes().size(), 1);
        QCOMPARE(p.keyframes()[0].x, 111.0);
        p.setTime(4.0);
        t = p.transform();
        t.x = 311;
        t.scale = 2.0;
        p.changeTransform(t); // with keyframes: a new one at the playhead
        QCOMPARE(p.keyframes().size(), 2);
        QCOMPARE(p.keyframes()[1].t, 4.0);
        p.setTime(3.0);
        QCOMPARE(p.transform().x, 211.0); // halfway
        QCOMPARE(p.transform().scale, 1.5);
        QCOMPARE(p.baseTransform().x, 111.0); // the one transform untouched
        // A drag at the playhead: one step, into the keyframe made there.
        for (double x : {220.0, 230.0, 240.0}) {
            t = p.transform();
            t.x = x;
            p.changeTransform(t, QStringLiteral("drag:move:1"));
        }
        QCOMPARE(p.keyframes().size(), 3);
        QCOMPARE(p.keyframes()[1].x, 240.0);
        const int steps = p.undoStack()->count();
        p.undoStack()->undo();
        QCOMPARE(p.keyframes().size(), 2);
        QCOMPARE(p.transform().x, 211.0);
        QCOMPARE(steps, p.undoStack()->count()); // undone, not removed
        // A flip with keyframes: on the one transform, the keyframes untouched.
        t = p.transform();
        t.flipV = true;
        p.changeTransform(t);
        QVERIFY(p.baseTransform().flipV);
        QCOMPARE(p.keyframes().size(), 2);
    }
    void easeMoveDelete() {
        Project p;
        p.setSource(source());
        p.setTime(2.0);
        p.addKeyframe();
        p.setTime(4.0);
        VideoTransform t = p.transform();
        t.x += 200;
        p.changeTransform(t);
        p.setTime(2.0);
        p.setKeyframeEase(p.keyframeAtTime(), true);
        QVERIFY(p.keyframes()[0].ease);
        const int before = p.undoStack()->count();
        for (double to : {2.1, 2.2, 2.5})
            p.moveKeyframe(0, to, QStringLiteral("drag:key:1"));
        QCOMPARE(p.undoStack()->count(), before + 1); // one drag, one step
        QCOMPARE(p.keyframes()[0].t, 2.5);
        // Deleting down to none leaves the video where it is shown.
        p.setTime(4.0);
        p.deleteKeyframe(1);
        const double shown = p.transform().x; // the last one's value, after it
        p.deleteKeyframe(0);
        QVERIFY(p.keyframes().isEmpty());
        QCOMPARE(p.transform().x, shown);
        p.undoStack()->undo();
        QCOMPARE(p.keyframes().size(), 1);
    }
    void theFile() {
        ProjectData d;
        d.source.path = QStringLiteral("D:/a.mp4");
        d.keyframes = {{12.4, 960, 540, 1.0, true}, {14.0, 900, 500, 1.5, false}};
        const QJsonObject json = projectjson::toJson(d);
        const QJsonArray keys =
            json.value(QStringLiteral("transform")).toObject().value(QStringLiteral("keyframes")).toArray();
        QCOMPARE(keys.size(), 2);
        QCOMPARE(keys[0].toObject().value(QStringLiteral("interp")).toString(), QStringLiteral("ease"));
        QCOMPARE(keys[0].toObject().value(QStringLiteral("t")).toDouble(), 12.4);
        ProjectData back;
        QString error;
        QVERIFY(projectjson::fromJson(json, back, &error));
        QCOMPARE(back.keyframes, d.keyframes);
    }
};

QTEST_GUILESS_MAIN(TestKeyframes)
#include "test-keyframes.moc"
