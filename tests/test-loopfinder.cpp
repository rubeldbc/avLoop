// The Loop Finder (spec §11.5, S14): fingerprints with a period of 4 s rank the 4 s loop first (±2 frames) — before
// the 8 s one that ties it; the five answers are never two within a second at both ends; the score runs 0–100 with
// the best near 100; Cancel stops it. On `loop4s.mp4` (tools/media/make-samples.mjs) the whole search finds 4 s too.
#include "Samples.h"
#include "export/LoopFinder.h"
#include "media/MediaProbe.h"

#include <QRandomGenerator>
#include <QTest>

#include <cmath>
#include <vector>

using namespace avloop;
using exporting::LoopFinder;

namespace {

// Frame i of a pattern repeating every `period` frames: a bright bar sweeping across a gradient, with a little noise.
std::vector<std::uint8_t> periodic(int frames, int period, quint32 seed) {
    std::vector<std::uint8_t> prints(std::size_t(frames) * LoopFinder::kPrint);
    QRandomGenerator noise(seed);
    for (int i = 0; i < frames; ++i) {
        const double phase = double(i % period) / period;
        const int bar = int(phase * LoopFinder::kSide);
        for (int y = 0; y < LoopFinder::kSide; ++y)
            for (int x = 0; x < LoopFinder::kSide; ++x) {
                int v = x * 2 + y + (std::abs(x - bar) < 4 ? 150 : 0);
                v += int(noise.bounded(3)) - 1;
                prints[std::size_t(i) * LoopFinder::kPrint + std::size_t(y) * LoopFinder::kSide + x] =
                    std::uint8_t(std::clamp(v, 0, 255));
            }
    }
    return prints;
}

} // namespace

class TestLoopFinder : public QObject {
    Q_OBJECT
private slots:
    void thePeriodRanksFirst() {
        const int frames = 300; // 10 s at 30 fps
        const auto prints = periodic(frames, 120, 7);
        std::atomic<bool> cancel{false};
        const QList<LoopFinder::Candidate> best = LoopFinder::best(prints, frames, 60, 300, 30, 5, cancel);
        QCOMPARE(best.size(), 5);
        const int length = int(best.front().out - best.front().in); // frame indices from best()
        QVERIFY2(std::abs(length - 120) <= 2, qPrintable(QStringLiteral("best length %1 frames").arg(length)));
        QVERIFY(best.front().score >= 95);
        for (int i = 0; i < best.size(); ++i) {
            QVERIFY(best[i].score >= 0 && best[i].score <= 100);
            if (i > 0) QVERIFY(best[i].raw >= best[i - 1].raw);
            for (int j = 0; j < i; ++j) // never two within a second at both ends
                QVERIFY(std::abs(best[i].in - best[j].in) >= 30 || std::abs(best[i].out - best[j].out) >= 30);
        }
    }
    void lengthsBoundTheAnswer() {
        const int frames = 300;
        const auto prints = periodic(frames, 120, 9);
        std::atomic<bool> cancel{false};
        // 2–3 s: the period is out of reach; every answer keeps to the bounds.
        for (const LoopFinder::Candidate& c : LoopFinder::best(prints, frames, 60, 90, 30, 5, cancel)) {
            QVERIFY(c.out - c.in >= 60);
            QVERIFY(c.out - c.in <= 90);
        }
    }
    void cancelStops() {
        const int frames = 300;
        const auto prints = periodic(frames, 120, 11);
        std::atomic<bool> cancel{true};
        QVERIFY(LoopFinder::best(prints, frames, 60, 300, 30, 5, cancel).isEmpty());
    }
    void theSample() {
        const QString video = test::samplePath(QStringLiteral("loop4s.mp4"));
        const media::MediaProbe::Result probed = media::MediaProbe::probe(video);
        if (!probed.info) QSKIP("loop4s.mp4 is missing (tools/media/make-samples.mjs)");
        LoopFinder::Input in;
        in.source = *probed.info;
        in.transform = VideoTransform::initial(QSizeF(in.source.width, in.source.height));
        in.frame = in.transform.rect(QSizeF(in.source.width, in.source.height));
        in.fps = {30, 1};
        in.from = 1;
        in.to = 15;
        std::atomic<bool> cancel{false};
        QString error;
        const QList<LoopFinder::Candidate> best = LoopFinder::find(in, cancel, {}, &error);
        QVERIFY2(!best.isEmpty(), qPrintable(error));
        const double length = best.front().out - best.front().in;
        QVERIFY2(std::abs(length - 4.0) <= 2.0 / 30, qPrintable(QStringLiteral("best length %1 s").arg(length)));
        QVERIFY(!best.front().first.isNull() && !best.front().last.isNull());
    }
};

QTEST_GUILESS_MAIN(TestLoopFinder)
#include "test-loopfinder.moc"
