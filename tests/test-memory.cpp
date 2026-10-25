// Memory (spec §3.7, AVADR-0002): the frame cache never passes its budget, evicts the oldest, halves under pressure,
// spills marked frames to the session's disk cache and reads them back; the disk cache's session goes with it and
// stale sessions go at the next start; decoding with random seeks into a bounded cache keeps Private Bytes flat.
#include "Samples.h"
#include "media/Decoder.h"
#include "media/DiskCache.h"
#include "media/FrameCache.h"
#include "media/MediaProbe.h"
#include "media/SystemMemory.h"

#include <QDir>
#include <QRandomGenerator>
#include <QTemporaryDir>
#include <QTest>

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
};

QTEST_GUILESS_MAIN(TestMemory)
#include "test-memory.moc"
