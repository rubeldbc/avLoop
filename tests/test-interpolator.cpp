// Motion Interpolation's pre-pass (spec §8.3): a segment of the 30 fps counter sample made at 60 fps — every other
// frame is a source frame (its burned-in counter reads back), the ones between are new; Speed 0.5 stretches it; a
// source with alpha keeps its alpha; progress counts up and a cancel stops it; another setting is another key.
#include "Samples.h"
#include "media/DiskCache.h"
#include "media/Interpolator.h"
#include "media/MediaProbe.h"

#include <QTemporaryDir>
#include <QTest>

using namespace avloop;
using namespace avloop::media;

namespace {

Interpolator::Job jobFor(const QString& name, double tIn, int segment, Rational fps, double speed = 1.0) {
    Interpolator::Job job;
    job.info = *MediaProbe::probe(test::samplePath(name)).info;
    job.tIn = tIn;
    job.segment = segment;
    job.fps = fps;
    job.speed = speed;
    return job;
}

} // namespace

class TestInterpolator : public QObject {
    Q_OBJECT
private slots:
    void doublesTheRate() {
        AVLOOP_NEED_SAMPLE(QStringLiteral("counter-640p30.mp4"));
        QTemporaryDir dir;
        DiskCache cache(dir.path());
        Interpolator mi(&cache);
        const Interpolator::Job job = jobFor(QStringLiteral("counter-640p30.mp4"), 1.0, 20, {60, 1});
        int last = 0;
        QString error;
        QVERIFY2(mi.run(
                     job,
                     [&](int done, int total) {
                         last = done;
                         return total == 20;
                     },
                     &error),
                 qPrintable(error));
        QCOMPARE(last, 20);
        for (int i = 0; i < 20; i += 2) {
            const Frame f = mi.frame(job, i);
            QVERIFY2(!f.isNull(), qPrintable(QString::number(i)));
            QCOMPARE(test::readCounter(f), 30 + i / 2); // τ(i) = 1 + i/60 s: source frame 30 + i/2
        }
        QVERIFY(!mi.frame(job, 1).isNull());
        QVERIFY(mi.frame(job, 20).isNull());
    }
    void slowMotion() {
        AVLOOP_NEED_SAMPLE(QStringLiteral("counter-640p30.mp4"));
        QTemporaryDir dir;
        DiskCache cache(dir.path());
        Interpolator mi(&cache);
        // Speed 0.5 at 30 fps: τ(i) = 2 + i/60 — every other frame a source frame.
        const Interpolator::Job job = jobFor(QStringLiteral("counter-640p30.mp4"), 2.0, 12, {30, 1}, 0.5);
        QVERIFY(mi.run(job, {}));
        for (int i = 0; i < 12; i += 2)
            QCOMPARE(test::readCounter(mi.frame(job, i)), 60 + i / 2);
        QVERIFY(Interpolator::key(job, 0) !=
                Interpolator::key(jobFor(QStringLiteral("counter-640p30.mp4"), 2.0, 12, {30, 1}), 0));
    }
    void keepsAlpha() {
        AVLOOP_NEED_SAMPLE(QStringLiteral("alpha-vp9.webm"));
        QTemporaryDir dir;
        DiskCache cache(dir.path());
        Interpolator mi(&cache);
        const Interpolator::Job job = jobFor(QStringLiteral("alpha-vp9.webm"), 0.5, 6, {60, 1});
        QString error;
        QVERIFY2(mi.run(job, {}, &error), qPrintable(error));
        const Frame f = mi.frame(job, 1); // an interpolated frame
        QVERIFY(!f.isNull());
        int clear = 0;
        int solid = 0;
        for (int y = 0; y < f.height; y += 4)
            for (int x = 0; x < f.width; x += 4) {
                const int a = f.data.get()[std::ptrdiff_t(y) * f.stride + std::ptrdiff_t(x) * 4 + 3];
                clear += a == 0;
                solid += a == 255;
            }
        QVERIFY2(clear > 0 && solid > 0, qPrintable(QStringLiteral("clear %1 solid %2").arg(clear).arg(solid)));
    }
    void cancels() {
        AVLOOP_NEED_SAMPLE(QStringLiteral("counter-640p30.mp4"));
        QTemporaryDir dir;
        DiskCache cache(dir.path());
        Interpolator mi(&cache);
        const Interpolator::Job job = jobFor(QStringLiteral("counter-640p30.mp4"), 0.0, 60, {60, 1});
        QString error;
        QVERIFY(!mi.run(job, [](int done, int) { return done < 3; }, &error));
        QCOMPARE(error, QStringLiteral("cancelled"));
        QVERIFY(mi.frame(job, 30).isNull());
    }
};

QTEST_GUILESS_MAIN(TestInterpolator)
#include "test-interpolator.moc"
