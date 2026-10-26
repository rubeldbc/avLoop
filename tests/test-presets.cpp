// Presets (spec §11.3): the three built-in OBS presets; a preset saved, listed after them by name, read back the same,
// deleted; a built-in name refused; names made safe file names.
#include "core/Presets.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

using namespace avloop;

class TestPresets : public QObject {
    Q_OBJECT
private slots:
    void builtIn() {
        const QList<Preset> shipped = presets::builtIn();
        QCOMPARE(shipped.size(), 3);
        QCOMPARE(shipped[0].name, QStringLiteral("OBS Overlay 1:1 512"));
        QCOMPARE(shipped[0].output.sizeId(), QStringLiteral("long-512"));
        QCOMPARE(shipped[1].name, QStringLiteral("OBS Lower Third 16:9 1280"));
        QCOMPARE(shipped[1].output.sizeId(), QStringLiteral("long-1280"));
        QCOMPARE(shipped[2].name, QStringLiteral("OBS Vertical 9:16 1080"));
        QCOMPARE(shipped[2].output.sizeId(), QStringLiteral("long-1080"));
        for (const Preset& p : shipped) {
            QVERIFY(p.builtIn);
            QCOMPARE(p.output.format, OutputFormat::WebmVp9);
            QCOMPARE(p.output.alpha, OutputSettings::Alpha::Auto);
            QCOMPARE(p.quality.preset, QualitySettings::Preset::Medium);
        }
    }
    void saveListDelete() {
        QTemporaryDir dir;
        Preset mine;
        mine.name = QStringLiteral("Zebra stripes");
        mine.loop.mode = LoopSettings::Mode::PingPong;
        mine.loop.speed = 0.5;
        mine.output.format = OutputFormat::WebmVp8;
        mine.output.alpha = OutputSettings::Alpha::Off;
        mine.output.background = QStringLiteral("#123456");
        mine.output.sourceFps = false;
        mine.output.fps = {24, 1};
        mine.quality.preset = QualitySettings::Preset::Custom;
        mine.quality.crf = 20;
        mine.quality.twoPass = true;
        Preset other;
        other.name = QStringLiteral("Apple");
        QVERIFY(presets::save(dir.path(), mine));
        QVERIFY(presets::save(dir.path(), other));
        const QList<Preset> all = presets::all(dir.path());
        QCOMPARE(all.size(), 5);
        QCOMPARE(all[3].name, QStringLiteral("Apple")); // after the built-in ones, by name
        QCOMPARE(all[4].name, QStringLiteral("Zebra stripes"));
        QVERIFY(all[4].matches(mine.loop, mine.output, mine.quality));
        QVERIFY(!all[4].builtIn);
        QVERIFY(presets::remove(dir.path(), QStringLiteral("Apple")));
        QCOMPARE(presets::all(dir.path()).size(), 4);
    }
    void refusals() {
        QTemporaryDir dir;
        Preset p;
        p.name = QStringLiteral("OBS Overlay 1:1 512");
        QString error;
        QVERIFY(!presets::save(dir.path(), p, &error));
        QVERIFY(!error.isEmpty());
        QVERIFY(!presets::remove(dir.path(), p.name));
        p.name = QStringLiteral("  ");
        QVERIFY(!presets::save(dir.path(), p));
        // A file that is not a preset is not listed.
        QFile junk(QDir(dir.path()).filePath(QStringLiteral("junk.json")));
        QVERIFY(junk.open(QIODevice::WriteOnly));
        junk.write("{\"format\": \"something\"}");
        junk.close();
        QCOMPARE(presets::all(dir.path()).size(), 3);
    }
    void fileNames() {
        QCOMPARE(presets::fileName(QStringLiteral("A/B: c?")), QStringLiteral("A_B_ c_.json"));
        QCOMPARE(presets::fileName(QStringLiteral("Lower third")), QStringLiteral("Lower third.json"));
    }
};

QTEST_GUILESS_MAIN(TestPresets)
#include "test-presets.moc"
