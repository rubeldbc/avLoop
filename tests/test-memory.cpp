// Memory (spec §3.7, AVADR-0002): the frame cache never passes its budget, evicts the oldest, halves under pressure,
// spills marked frames to the session's disk cache and reads them back; the disk cache's session goes with it and
// stale sessions go at the next start; decoding with random seeks into a bounded cache keeps Private Bytes flat.
#include "Samples.h"
#include "media/DecodeWorker.h"
#include "media/Decoder.h"
#include "media/DiskCache.h"
#include "media/FrameCache.h"
#include "media/MediaProbe.h"
#include "media/SystemMemory.h"
#include "media/ThumbnailWorker.h"

#include <QDir>
#include <QRandomGenerator>
#include <QTemporaryDir>
#include <QTest>

#include <limits>

using namespace avloop;
using avloop::test::samplePath;

namespace {

media::Frame synthetic(int w, int h, std::uint8_t fill, std::int64_t pts) {
    media::Frame f;
    f.width = w;
    f.height = h;
    f.stride = w * 4;
    f.pts = pts;
    auto pixels = std::make_shared_for_overwrite<std::uint8_t[]>(size_t(f.stride) * h);
    std::memset(pixels.get(), fill, size_t(f.stride) * h);
    f.data = std::shared_ptr<const std::uint8_t>(pixels, pixels.get());
    return f;
}

constexpr std::int64_t MB = 1 << 20;

} // namespace

class TestMemory : public QObject {
    Q_OBJECT
private slots:
    void cacheStaysWithinBudget() {
        media::FrameCache cache(256 * MB);
        for (int i = 0; i < 400; ++i) {
            cache.insert({1, 512, 512, i}, synthetic(512, 512, std::uint8_t(i), i)); // 1 MB each
            QVERIFY(cache.used() <= cache.budget());
        }
        QCOMPARE(cache.count(), 256);
        QVERIFY(cache.find({1, 512, 512, 0}).isNull());    // the oldest went
        QVERIFY(!cache.find({1, 512, 512, 399}).isNull()); // the newest stays
    }
    void budgetIsClamped() {
        media::FrameCache cache(1);
        QCOMPARE(cache.budget(), media::FrameCache::kMinBudget);
        cache.setBudget(1LL << 40);
        QCOMPARE(cache.budget(), media::FrameCache::kMaxBudget);
    }
    void halvesUnderPressure() {
        media::FrameCache cache(256 * MB);
        for (int i = 0; i < 256; ++i)
            cache.insert({1, 512, 512, i}, synthetic(512, 512, 0, i));
        cache.halve();
        QVERIFY(cache.halved());
        QVERIFY(cache.used() <= 128 * MB);
        cache.restore();
        QCOMPARE(cache.budget(), 256 * MB);
    }
    void removesASource() {
        media::FrameCache cache(256 * MB);
        for (int i = 0; i < 10; ++i) {
            cache.insert({1, 64, 64, i}, synthetic(64, 64, 0, i));
            cache.insert({2, 64, 64, i}, synthetic(64, 64, 0, i));
        }
        cache.removeSource(1);
        QCOMPARE(cache.count(), 10);
        QVERIFY(cache.find({1, 64, 64, 3}).isNull());
    }
    void spillsToDiskAndBack() {
        QTemporaryDir root;
        QString session;
        {
            media::DiskCache disk(root.path());
            session = disk.sessionDir();
            QVERIFY(QDir(session).exists());
            media::FrameCache cache(256 * MB);
            cache.setDiskCache(&disk);
            for (int i = 0; i < 300; ++i)
                cache.insert({7, 512, 512, i}, synthetic(512, 512, std::uint8_t(i), i), true);
            QVERIFY(disk.used() > 0);
            const media::Frame back = cache.find({7, 512, 512, 3}); // evicted from RAM, read from disk
            QVERIFY(!back.isNull());
            QCOMPARE(back.data.get()[100], std::uint8_t(3));
        }
        QVERIFY(!QDir(session).exists()); // the session's folder goes with the cache
    }
    void staleSessionsAreRemoved() {
        QTemporaryDir root;
        QVERIFY(QDir(root.path()).mkpath(QStringLiteral("999999-1000"))); // no such process
        media::DiskCache live(root.path());
        QCOMPARE(media::DiskCache::removeStaleSessions(root.path()), 1);
        QVERIFY(QDir(live.sessionDir()).exists()); // this process's own session stays
    }
    void decodingKeepsMemoryFlat() {
        AVLOOP_NEED_SAMPLE("counter-1080p30.mp4");
        const auto probe = media::MediaProbe::probe(samplePath(QStringLiteral("counter-1080p30.mp4")));
        QVERIFY(probe.info);
        media::FrameCache cache(256 * MB);
        QRandomGenerator rng(42);
        std::int64_t growth = 0;
        for (bool hardware : {false, true}) {
            std::int64_t warm = 0;
            std::int64_t peak = 0;
            media::Decoder::Options o;
            o.hardware = hardware;
            media::Decoder d(*probe.info, o);
            QVERIFY(d.open());
            for (int i = 0; i < 300; ++i) {
                const media::Frame f = d.frameAtIndex(rng.bounded(300));
                QVERIFY(!f.isNull());
                cache.insert({1, f.width, f.height, f.pts}, f);
                if (i == 50) warm = media::SystemMemory::read().privateBytes;
                if (warm) peak = qMax(peak, media::SystemMemory::read().privateBytes);
            }
            growth = qMax(growth, peak - warm);
            qInfo().noquote() << QStringLiteral("%1: after warm-up %2 MB, peak %3 MB, growth %4 MB, cache %5/%6 MB")
                                     .arg(hardware ? QStringLiteral("hw") : QStringLiteral("sw"))
                                     .arg(warm / MB)
                                     .arg(peak / MB)
                                     .arg((peak - warm) / MB)
                                     .arg(cache.used() / MB)
                                     .arg(cache.budget() / MB);
        }
        QVERIFY(cache.used() <= cache.budget());
        QVERIFY2(growth < 50 * MB, qPrintable(QStringLiteral("grew %1 MB").arg(growth / MB)));
    }
    void decodersComeAndGo_data() {
        QTest::addColumn<QString>("sample");
        QTest::addColumn<bool>("hardware");
        QTest::addColumn<bool>("thumbnails");
        QTest::addColumn<int>("what"); // 0 decoder or thumbnails, 1 probe, 2 decode worker with a frame cache
        QTest::newRow("4K, hardware") << QStringLiteral("uhd-4k.mp4") << true << false << 0;
        QTest::newRow("4K, software") << QStringLiteral("uhd-4k.mp4") << false << false << 0;
        QTest::newRow("4K, thumbnails") << QStringLiteral("uhd-4k.mp4") << false << true << 0;
        QTest::newRow("HDR, thumbnails") << QStringLiteral("hdr-pq.mkv") << false << true << 0;
        QTest::newRow("4K, probe") << QStringLiteral("uhd-4k.mp4") << false << false << 1;
        QTest::newRow("HDR, probe") << QStringLiteral("hdr-pq.mkv") << false << false << 1;
        QTest::newRow("4K, decode worker") << QStringLiteral("uhd-4k.mp4") << true << false << 2;
        QTest::newRow("HDR, decode worker") << QStringLiteral("hdr-pq.mkv") << true << false << 2;
        QTest::newRow("640p, decode worker") << QStringLiteral("counter-640p30.mp4") << true << false << 2;
        QTest::newRow("640p, software worker") << QStringLiteral("counter-640p30.mp4") << false << false << 2;
        QTest::newRow("640p, thumbnails") << QStringLiteral("counter-640p30.mp4") << false << true << 0;
        QTest::newRow("640p, probe") << QStringLiteral("counter-640p30.mp4") << false << false << 1;
        QTest::newRow("640p, software worker opened") << QStringLiteral("counter-640p30.mp4") << false << false << 3;
    }
    void decodersComeAndGo() {
        // Every video opened makes a decoder (and the timeline's thumbnail worker) and every one closed frees it:
        // over twenty, the floor of the last five rounds stays within 20 MB of the early rounds' peak (a leak lifts
        // the floor; the heap's own timing only the peaks).
        QFETCH(QString, sample);
        QFETCH(bool, hardware);
        QFETCH(bool, thumbnails);
        QFETCH(int, what);
        media::FrameCache cache(256 * MB);
        AVLOOP_NEED_SAMPLE(sample);
        const auto probe = media::MediaProbe::probe(samplePath(sample));
        QVERIFY(probe.info);
        std::int64_t early = 0;
        std::int64_t late = std::numeric_limits<std::int64_t>::max();
        for (int round = 0; round < 20; ++round) {
            if (what == 1) {
                QVERIFY(media::MediaProbe::probe(samplePath(sample)).info);
            } else if (what == 3) {
                media::Decoder::Options o;
                o.hardware = hardware;
                media::DecodeWorker worker(*probe.info, o, &cache);
                bool opened = false;
                connect(&worker, &media::DecodeWorker::opened, this, [&](bool ok) { opened = ok; });
                worker.start();
                QTRY_VERIFY_WITH_TIMEOUT(opened, 10000);
            } else if (what == 2) {
                media::Decoder::Options o;
                o.hardware = hardware;
                media::DecodeWorker worker(*probe.info, o, &cache);
                int got = 0;
                connect(&worker, &media::DecodeWorker::frameReady, this,
                        [&](double, const media::Frame& f) { got += !f.isNull(); });
                worker.start();
                worker.requestFrame(0);
                QTRY_VERIFY_WITH_TIMEOUT(got >= 1, 10000);
                cache.removeSource(worker.sourceId());
            } else if (thumbnails) {
                media::ThumbnailWorker w(*probe.info);
                QList<int> all;
                for (int i = 0; i < w.count(); ++i)
                    all << i;
                w.request(all);
                QTRY_VERIFY_WITH_TIMEOUT(w.made() >= std::min(12, w.count()), 10000);
            } else {
                media::Decoder::Options o;
                o.hardware = hardware;
                media::Decoder d(*probe.info, o);
                QVERIFY(d.open());
                for (int i = 0; i < 12; ++i)
                    QVERIFY(!d.frameAtIndex(i * 7).isNull());
            }
            const std::int64_t now = media::SystemMemory::read().privateBytes;
            if (round >= 2 && round <= 4) early = std::max(early, now);
            if (round >= 15) late = std::min(late, now);
        }
        qInfo().noquote() << QStringLiteral("%1: early peak %2 MB, late floor %3 MB")
                                 .arg(QString::fromLatin1(QTest::currentDataTag()))
                                 .arg(early / MB)
                                 .arg(late / MB);
        QVERIFY2(late - early < 20 * MB, qPrintable(QStringLiteral("the floor rose %1 MB").arg((late - early) / MB)));
    }
};

QTEST_GUILESS_MAIN(TestMemory)
#include "test-memory.moc"
