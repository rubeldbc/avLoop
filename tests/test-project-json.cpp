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
        QCOMPARE(json.value(QStringLiteral("version")).toInt(), 1);
        QCOMPARE(json.value(QStringLiteral("source")).toObject().value(QStringLiteral("mtime")).toString(),
                 QStringLiteral("2026-10-26T10:12:00Z"));
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
