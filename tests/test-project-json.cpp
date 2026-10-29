// The .avloop file (spec §11.1): written and read back the same; unknown keys kept at every level; a newer version or
// another format refused with a message; the source found by its relative path; the save atomic.
#include "core/ProjectJson.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <QTest>

using namespace avloop;

namespace {

ProjectData sample(const QString& video) {
    ProjectData d;
    d.source.path = video;
    d.source.fileSize = 48213344;
    d.source.modified = QDateTime(QDate(2026, 10, 26), QTime(10, 12), QTimeZone::UTC);
    d.transform.x = 960;
    d.transform.y = 540;
    d.transform.scale = 1.25;
    d.transform.flipH = true;
    d.view.zoom = 0.5;
    d.view.tool = QStringLiteral("crop-ellipse");
    return d;
}

} // namespace

class TestProjectJson : public QObject {
    Q_OBJECT
private slots:
    void roundTrip() {
        const ProjectData d = sample(QStringLiteral("D:/clips/fire.mp4"));
        ProjectData back;
        QString error;
        QVERIFY(projectjson::fromJson(projectjson::toJson(d), back, &error));
        QCOMPARE(back.transform, d.transform);
        QCOMPARE(back.view.zoom, 0.5);
        QCOMPARE(back.view.tool, QStringLiteral("crop-ellipse"));
        QCOMPARE(back.source.fileSize, d.source.fileSize);
        QCOMPARE(back.source.modified, d.source.modified);
        const QJsonObject json = projectjson::toJson(d);
        QCOMPARE(json.value(QStringLiteral("format")).toString(), QStringLiteral("avloop"));
        QCOMPARE(json.value(QStringLiteral("version")).toInt(), 4);
        QCOMPARE(json.value(QStringLiteral("source")).toObject().value(QStringLiteral("mtime")).toString(),
                 QStringLiteral("2026-10-26T10:12:00Z"));
    }
    // "output" (spec §11.1): format, size, w, h, the lock, alpha, background; "export": lastPath.
    void outputRoundTrip() {
        ProjectData d = sample(QStringLiteral("D:/clips/fire.mp4"));
        d.output.format = OutputFormat::WebmVp8;
        d.output.setSizeId(QStringLiteral("long-720"));
        d.output.custom = QSize(500, 300);
        d.output.lockRatio = false;
        d.output.alpha = OutputSettings::Alpha::Off;
        d.output.background = QStringLiteral("#112233");
        d.exportPath = QStringLiteral("D:/obs/loops/fire_loop_640x640.webm");
        const QJsonObject json = projectjson::toJson(d);
        const QJsonObject o = json.value(QStringLiteral("output")).toObject();
        QCOMPARE(o.value(QStringLiteral("size")).toString(), QStringLiteral("long-720"));
        QCOMPARE(o.value(QStringLiteral("alpha")).toString(), QStringLiteral("off"));
        QCOMPARE(json.value(QStringLiteral("export")).toObject().value(QStringLiteral("lastPath")).toString(),
                 d.exportPath);
        ProjectData back;
        QString error;
        QVERIFY(projectjson::fromJson(json, back, &error));
        QCOMPARE(back.output, d.output);
        QCOMPARE(back.exportPath, d.exportPath);
        // The defaults (reading R-6): Crop size, Alpha Auto, black, the lock on.
        ProjectData plain;
        QVERIFY(projectjson::fromJson(projectjson::toJson(sample(QStringLiteral("D:/a.mp4"))), plain, &error));
        QCOMPARE(plain.output.size, OutputSettings::Size::Crop);
        QCOMPARE(plain.output.alpha, OutputSettings::Alpha::Auto);
        QCOMPARE(plain.output.background, QStringLiteral("#000000"));
        QVERIFY(plain.output.lockRatio);
    }
    // Spec §11.1: ProjectMigrate — a file without a version (version 0), of version 1 or 2 reads as written, without
    // clip effects (before 2) or an inverted crop (before 3), and is written as version 3; a newer one is refused.
    void migration() {
        QJsonObject old = projectjson::toJson(sample(QStringLiteral("D:/a.mp4")));
        old.remove(QStringLiteral("version"));
        ProjectData back;
        QString error;
        QVERIFY2(projectjson::fromJson(old, back, &error), qPrintable(error));
        QCOMPARE(back.transform.scale, 1.25);
        QCOMPARE(projectjson::toJson(back).value(QStringLiteral("version")).toInt(), 4);
        QVERIFY(back.effects.isNeutral());
        QJsonObject one = old;
        one.insert(QStringLiteral("version"), 1);
        one.remove(QStringLiteral("effects"));
        QVERIFY2(projectjson::fromJson(one, back, &error), qPrintable(error));
        QCOMPARE(back.effects, ClipEffects{});
        QJsonObject two = old;
        two.insert(QStringLiteral("version"), 2);
        QJsonObject crop = two.value(QStringLiteral("crop")).toObject();
        crop.remove(QStringLiteral("invert"));
        two.insert(QStringLiteral("crop"), crop);
        QVERIFY2(projectjson::fromJson(two, back, &error), qPrintable(error));
        QVERIFY(!back.crop.invert);
        QJsonObject newer = old;
        newer.insert(QStringLiteral("version"), 7);
        QVERIFY(!projectjson::fromJson(newer, back, &error));
        QCOMPARE(error,
                 QStringLiteral("This project was saved by a newer AvLoop (version 7); update AvLoop to open it."));
    }
    // Save → open → save: the same bytes (nothing lost or reordered on the way).
    void saveOpenSaveIsIdentical() {
        QTemporaryDir dir;
        ProjectData d = sample(QStringLiteral("D:/clips/fire.mp4"));
        d.crop.kind = CropShape::Kind::Rounded;
        d.crop.rect = QRectF(100, 50, 300, 200);
        d.crop.radius = 12;
        d.loop.mode = LoopSettings::Mode::PingPongSmooth;
        d.output.setSizeId(QStringLiteral("custom"));
        d.output.custom = QSize(320, 240);
        d.quality.preset = QualitySettings::Preset::Custom;
        d.quality.targetMB = 1.5;
        d.original.insert(QStringLiteral("someday"), QStringLiteral("kept"));
        const QString a = dir.filePath(QStringLiteral("a.avloop"));
        const QString b = dir.filePath(QStringLiteral("b.avloop"));
        QString error;
        QVERIFY(projectjson::save(d, a, &error));
        ProjectData read;
        QVERIFY2(projectjson::load(a, read, &error), qPrintable(error));
        QVERIFY(projectjson::save(read, b, &error));
        QFile fa(a), fb(b);
        QVERIFY(fa.open(QIODevice::ReadOnly) && fb.open(QIODevice::ReadOnly));
        QCOMPARE(fb.readAll(), fa.readAll());
        QCOMPARE(read.output, d.output);
        QCOMPARE(read.loop, d.loop);
        QCOMPARE(read.quality, d.quality);
    }
    // Mode None is written "none" and read back (D-020).
    void noneRoundTrip() {
        QTemporaryDir dir;
        ProjectData d = sample(QStringLiteral("D:/clips/fire.mp4"));
        d.loop.mode = LoopSettings::Mode::None;
        const QString path = dir.filePath(QStringLiteral("none.avloop"));
        QString error;
        QVERIFY(projectjson::save(d, path, &error));
        QFile f(path);
        QVERIFY(f.open(QIODevice::ReadOnly));
        QVERIFY(f.readAll().contains("\"mode\": \"none\""));
        ProjectData read;
        QVERIFY2(projectjson::load(path, read, &error), qPrintable(error));
        QCOMPARE(read.loop.mode, LoopSettings::Mode::None);
    }
    // The inverted crop (D-021): "invert" under crop, read back.
    void invertRoundTrip() {
        ProjectData d = sample(QStringLiteral("a.mp4"));
        d.crop.kind = CropShape::Kind::Ellipse;
        d.crop.rect = QRectF(10, 20, 100, 80);
        d.crop.invert = true;
        const QJsonObject json = projectjson::toJson(d);
        QCOMPARE(json.value(QStringLiteral("crop")).toObject().value(QStringLiteral("invert")).toBool(), true);
        ProjectData back;
        QString error;
        QVERIFY2(projectjson::fromJson(json, back, &error), qPrintable(error));
        QVERIFY(back.crop.invert);
        QCOMPARE(back.crop.rect, d.crop.rect);
    }
    // The shapes (D-022) under crop, read back in their order with every value.
    void shapesRoundTrip() {
        ProjectData d = sample(QStringLiteral("a.mp4"));
        MaskShape r;
        r.rect = QRectF(1, 2, 30, 40);
        r.name = QStringLiteral("Rectangle 1");
        MaskShape p;
        p.kind = MaskShape::Kind::Polygon;
        p.role = MaskShape::Role::Cut;
        p.points = {PathPoint::corner({0, 0}), PathPoint::corner({10, 0}), PathPoint::corner({5, 9})};
        d.crop.shapes = {r, p};
        ProjectData back;
        QString error;
        QVERIFY2(projectjson::fromJson(projectjson::toJson(d), back, &error), qPrintable(error));
        QCOMPARE(back.crop.shapes.size(), 2);
        QCOMPARE(back.crop.shapes[0], r);
        QCOMPARE(back.crop.shapes[1], p);
    }
    // The video layers (D-027) under "layers", read back; a file without them has none.
    void layersRoundTrip() {
        ProjectData d = sample(QStringLiteral("a.mp4"));
        VideoLayer l;
        l.name = QStringLiteral("b");
        l.path = QStringLiteral("D:/clips/b.mp4");
        l.size = QSizeF(640, 360);
        l.duration = 4;
        l.range = {0, 4};
        l.start = 1.5;
        l.opacity = 60;
        d.layers = {l};
        const QJsonObject json = projectjson::toJson(d);
        ProjectData back;
        QString error;
        QVERIFY2(projectjson::fromJson(json, back, &error), qPrintable(error));
        QCOMPARE(back.layers.size(), 1);
        QCOMPARE(back.layers.first(), l);
        QJsonObject without = json;
        without.remove(QStringLiteral("layers"));
        QVERIFY(projectjson::fromJson(without, back, &error));
        QVERIFY(back.layers.isEmpty());
    }
    void cropRoundTrip() {
        ProjectData d = sample(QStringLiteral("a.mp4"));
        d.crop.kind = CropShape::Kind::Rounded;
        d.crop.rect = QRectF(100, 50, 640, 360);
        d.crop.ratio = QStringLiteral("16:9");
        d.crop.radius = 24;
        d.crop.feather = 4;
        QJsonObject json = projectjson::toJson(d);
        const QJsonObject crop = json.value(QStringLiteral("crop")).toObject();
        QCOMPARE(crop.value(QStringLiteral("kind")).toString(), QStringLiteral("rounded"));
        QCOMPARE(crop.value(QStringLiteral("w")).toDouble(), 640.0);
        QCOMPARE(crop.value(QStringLiteral("ratio")).toString(), QStringLiteral("16:9"));
        ProjectData back;
        QVERIFY(projectjson::fromJson(json, back, nullptr));
        QCOMPARE(back.crop, d.crop);
        // No crop written, none read: the whole video (reading R-8).
        json.remove(QStringLiteral("crop"));
        ProjectData none;
        QVERIFY(projectjson::fromJson(json, none, nullptr));
        QVERIFY(none.crop.isEmpty());
    }
    void unknownKeysStay() {
        QJsonObject json = projectjson::toJson(sample(QStringLiteral("a.mp4")));
        json.insert(QStringLiteral("future"), QStringLiteral("kept"));
        QJsonObject t = json.value(QStringLiteral("transform")).toObject();
        t.insert(QStringLiteral("rotationLater"), 12);
        json.insert(QStringLiteral("transform"), t);
        ProjectData d;
        QVERIFY(projectjson::fromJson(json, d, nullptr));
        d.transform.x = 1; // a change, then a save
        const QJsonObject out = projectjson::toJson(d);
        QCOMPARE(out.value(QStringLiteral("future")).toString(), QStringLiteral("kept"));
        QCOMPARE(out.value(QStringLiteral("transform")).toObject().value(QStringLiteral("rotationLater")).toInt(), 12);
        QCOMPARE(out.value(QStringLiteral("transform")).toObject().value(QStringLiteral("x")).toDouble(), 1.0);
    }
    void refusals() {
        ProjectData d;
        QString error;
        QJsonObject newer = projectjson::toJson(sample(QStringLiteral("a.mp4")));
        newer.insert(QStringLiteral("version"), 7);
        QVERIFY(!projectjson::fromJson(newer, d, &error));
        QVERIFY2(error.contains(QStringLiteral("newer AvLoop (version 7)")), qPrintable(error));
        QVERIFY(!projectjson::fromJson(QJsonObject{{QStringLiteral("format"), QStringLiteral("x")}}, d, &error));
        QVERIFY(error.contains(QStringLiteral("not an AvLoop project")));
    }
    void relativePathFindsTheSource() {
        QTemporaryDir dir;
        QVERIFY(QDir(dir.path()).mkpath(QStringLiteral("clips")));
        QVERIFY(QDir(dir.path()).mkpath(QStringLiteral("projects")));
        const QString video = dir.filePath(QStringLiteral("clips/fire.mp4"));
        QFile f(video);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("not a video");
        f.close();
        const QString project = dir.filePath(QStringLiteral("projects/fire.avloop"));
        QString error;
        QVERIFY(projectjson::save(sample(video), project, &error));
        // The absolute path breaks (the folder was moved); the relative one still holds.
        QFile file(project);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QJsonObject json = QJsonDocument::fromJson(file.readAll()).object();
        file.close();
        QCOMPARE(json.value(QStringLiteral("source")).toObject().value(QStringLiteral("relativePath")).toString(),
                 QStringLiteral("../clips/fire.mp4"));
        QJsonObject source = json.value(QStringLiteral("source")).toObject();
        source.insert(QStringLiteral("path"), QStringLiteral("Z:/gone/fire.mp4"));
        json.insert(QStringLiteral("source"), source);
        ProjectData d;
        QVERIFY(projectjson::fromJson(json, d, &error, project));
        QCOMPARE(QFileInfo(d.source.path), QFileInfo(video));
    }
    void saveIsAtomicAndLoads() {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("p.avloop"));
        QString error;
        QVERIFY(projectjson::save(sample(QStringLiteral("a.mp4")), path, &error));
        ProjectData d;
        QVERIFY(projectjson::load(path, d, &error));
        QCOMPARE(d.transform.scale, 1.25);
        QCOMPARE(QDir(dir.path()).entryList(QDir::Files).size(), 1); // no temporary file left
    }
    void damagedFileSaysSo() {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("bad.avloop"));
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("{ not json");
        f.close();
        ProjectData d;
        QString error;
        QVERIFY(!projectjson::load(path, d, &error));
        QVERIFY(error.contains(QStringLiteral("damaged")));
    }
};

QTEST_GUILESS_MAIN(TestProjectJson)
#include "test-project-json.moc"
