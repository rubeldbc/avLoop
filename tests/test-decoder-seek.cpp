// Frame-accurate decoding (spec §7.3, S3): over 200 random seeks the decoded frame's burned counter is the frame asked
// for — in software and with D3D11VA; a VFR source answers by PTS; sequential decoding counts up without a gap.
#include "Samples.h"
#include "media/Decoder.h"
#include "media/MediaProbe.h"

#include <QRandomGenerator>
#include <QTest>

using namespace avloop;
using avloop::test::readCounter;
using avloop::test::samplePath;

class TestDecoderSeek : public QObject {
    Q_OBJECT
private:
    std::unique_ptr<media::Decoder> open(const QString& name, bool hardware) {
        const auto r = media::MediaProbe::probe(samplePath(name));
        if (!r.info) return nullptr;
        media::Decoder::Options o;
        o.hardware = hardware;
        auto d = std::make_unique<media::Decoder>(*r.info, o);
        return d->open() ? std::move(d) : nullptr;
    }

    void randomSeeks(const QString& name, bool hardware, int count, quint32 seed) {
        auto d = open(name, hardware);
        QVERIFY(d);
        const int frames = int(d->info().frameCount);
        QVERIFY(frames > 100);
        QRandomGenerator rng(seed);
        int wrong = 0;
        int at = 0;
        for (int i = 0; i < count; ++i) {
            // Mostly jumps anywhere, some short steps forward and back (the GOP's own cases).
            const int kind = rng.bounded(4);
            if (kind == 0)
                at = qBound(0, at + rng.bounded(1, 8), frames - 1);
            else if (kind == 1)
                at = qBound(0, at - rng.bounded(1, 8), frames - 1);
            else
                at = rng.bounded(frames);
            const media::Frame f = d->frameAtIndex(at);
            QVERIFY2(!f.isNull(), qPrintable(QStringLiteral("frame %1 is null").arg(at)));
            const int got = readCounter(f);
            if (got != at) {
                ++wrong;
                qWarning() << name << (hardware ? "hw" : "sw") << "asked" << at << "got" << got;
            }
        }
        qInfo().noquote() << QStringLiteral("%1 %2: %3 seeks, %4 wrong, %5 real seeks, hardware %6")
                                 .arg(name, hardware ? QStringLiteral("hw") : QStringLiteral("sw"))
                                 .arg(count)
                                 .arg(wrong)
                                 .arg(d->seeks())
                                 .arg(d->hardwareActive() ? QStringLiteral("on") : QStringLiteral("off"));
        QCOMPARE(wrong, 0);
    }

private slots:
    void sequentialCountsUp() {
        AVLOOP_NEED_SAMPLE("counter-640p30.mp4");
        auto d = open(QStringLiteral("counter-640p30.mp4"), false);
        QVERIFY(d);
        int n = 0;
        for (media::Frame f = d->next(); !f.isNull(); f = d->next(), ++n)
            QCOMPARE(readCounter(f), n);
        QCOMPARE(n, 300);
    }
    void twoHundredSeeksSoftware() {
        AVLOOP_NEED_SAMPLE("counter-640p30.mp4");
        randomSeeks(QStringLiteral("counter-640p30.mp4"), false, 200, 2026);
    }
    void twoHundredSeeksHardware() {
        AVLOOP_NEED_SAMPLE("counter-1080p30.mp4");
        randomSeeks(QStringLiteral("counter-1080p30.mp4"), true, 200, 1026);
    }
    void variableRateByPts() {
        AVLOOP_NEED_SAMPLE("vfr.mp4");
        auto seq = open(QStringLiteral("vfr.mp4"), false);
        QVERIFY(seq);
        QList<QPair<double, int>> shown; // (seconds, counter) in display order
        for (media::Frame f = seq->next(); !f.isNull(); f = seq->next())
            shown.append({f.seconds, readCounter(f)});
        QVERIFY(shown.size() > 100);
        auto d = open(QStringLiteral("vfr.mp4"), false);
        QRandomGenerator rng(7);
        for (int i = 0; i < 100; ++i) {
            const double t = rng.bounded(shown.last().first);
            int expected = shown.first().second;
            for (const auto& [s, c] : shown)
                if (s <= t + 1e-9) expected = c;
            QCOMPARE(readCounter(d->frameAt(t)), expected);
        }
    }
    void pastTheEndGivesTheLastFrame() {
        AVLOOP_NEED_SAMPLE("counter-640p30.mp4");
        auto d = open(QStringLiteral("counter-640p30.mp4"), false);
        QCOMPARE(readCounter(d->frameAt(99.0)), 299);
        QCOMPARE(readCounter(d->frameAt(-1.0)), 0);
    }
};

QTEST_GUILESS_MAIN(TestDecoderSeek)
#include "test-decoder-seek.moc"
