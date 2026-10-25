// MediaProbe (spec §3.9, §12.1): the source cases read right — rotation, SAR, VFR, HDR, interlacing, alpha — and a
// file that cannot be opened says why.
#include "Samples.h"
#include "media/MediaProbe.h"

#include <QTest>

using namespace avloop;
using avloop::test::samplePath;

class TestProbe : public QObject {
    Q_OBJECT
private:
    media::MediaInfo info(const QString& name) {
        const auto r = media::MediaProbe::probe(samplePath(name));
        if (!r.info) qWarning() << name << r.error;
        return r.info.value_or(media::MediaInfo{});
    }

private slots:
    void counter() {
        AVLOOP_NEED_SAMPLE("counter-640p30.mp4");
        const auto i = info(QStringLiteral("counter-640p30.mp4"));
        QCOMPARE(i.codec, QStringLiteral("h264"));
        QCOMPARE(QSize(i.width, i.height), QSize(640, 360));
        QCOMPARE(i.avgFps.value(), 30.0);
        QVERIFY(!i.vfr);
        QVERIFY(qAbs(i.duration - 10.0) < 0.05);
        QCOMPARE(i.statusLine(), QStringLiteral("640×360 · 30.00 fps · H.264"));
        QVERIFY(i.hardwareCandidate);
    }
    void rotation() {
        AVLOOP_NEED_SAMPLE("rotate90.mp4");
        const auto i = info(QStringLiteral("rotate90.mp4"));
        QCOMPARE(QSize(i.width, i.height), QSize(360, 640)); // upright: the long side vertical
        QCOMPARE(i.rotation, 270);                           // 90° counter-clockwise
    }
    void anamorphic() {
        AVLOOP_NEED_SAMPLE("sar43.mp4");
        const auto i = info(QStringLiteral("sar43.mp4"));
        QCOMPARE(i.sar.num, 4);
        QCOMPARE(i.sar.den, 3);
        QCOMPARE(QSize(i.width, i.height), QSize(854, 480)); // 640 × 4/3, square pixels, even
    }
    void variableRate() {
        AVLOOP_NEED_SAMPLE("vfr.mp4");
        const auto i = info(QStringLiteral("vfr.mp4"));
        QVERIFY(i.vfr);
        QVERIFY(i.statusLine().endsWith(QStringLiteral("VFR → CFR")));
    }
    void hdr() {
        AVLOOP_NEED_SAMPLE("hdr-pq.mkv");
        const auto i = info(QStringLiteral("hdr-pq.mkv"));
        QVERIFY(i.hdr);
        QCOMPARE(i.transfer, QStringLiteral("smpte2084"));
        QCOMPARE(i.bitDepth, 10);
    }
    void interlaced() {
        AVLOOP_NEED_SAMPLE("interlaced.mpg");
        QVERIFY(info(QStringLiteral("interlaced.mpg")).interlaced);
    }
    void alpha() {
        AVLOOP_NEED_SAMPLE("alpha-vp9.webm");
        const auto vp9 = info(QStringLiteral("alpha-vp9.webm"));
        QVERIFY(vp9.hasAlpha);
        QVERIFY(!vp9.hardwareCandidate); // alpha needs libvpx, never the GPU decoder
        AVLOOP_NEED_SAMPLE("prores4444.mov");
        QVERIFY(info(QStringLiteral("prores4444.mov")).hasAlpha);
        QVERIFY(!info(QStringLiteral("counter-640p30.mp4")).hasAlpha);
    }
    void unicodePath() {
        AVLOOP_NEED_SAMPLE(QStringLiteral("বাংলা নমুনা/ভিডিও.mp4"));
        QCOMPARE(info(QStringLiteral("বাংলা নমুনা/ভিডিও.mp4")).width, 640);
    }
    void refusals() {
        AVLOOP_NEED_SAMPLE("audio-only.m4a");
        const auto audio = media::MediaProbe::probe(samplePath(QStringLiteral("audio-only.m4a")));
        QVERIFY(!audio.info);
        QVERIFY2(audio.error.contains(QStringLiteral("no video")), qPrintable(audio.error));
        AVLOOP_NEED_SAMPLE("broken.mp4");
        const auto broken = media::MediaProbe::probe(samplePath(QStringLiteral("broken.mp4")));
        QVERIFY(!broken.info);
        QVERIFY2(broken.error.contains(QStringLiteral("broken.mp4")), qPrintable(broken.error));
        const auto missing = media::MediaProbe::probe(samplePath(QStringLiteral("nothing-here.mp4")));
        QVERIFY(!missing.info);
        QVERIFY(missing.error.contains(QStringLiteral("does not exist")));
    }
};

QTEST_GUILESS_MAIN(TestProbe)
#include "test-probe.moc"
