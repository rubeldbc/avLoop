// The CPU back end's answers (AVADR-0001, spec §3.4): copied 1:1, averaged at half size, mirrored, half a texel
// between neighbours, transparent outside the frame, a 0.7/0.3 blend, a mask multiplied in, opaque over a colour —
// each worked out here by hand, not by the renderer's code.
#include "RenderCases.h"

#include <QTest>

#include <cmath>

using namespace avloop;
using namespace avloop::test;

namespace {

QImage run(const render::Composite& c) {
    QImage out(c.size, QImage::Format_RGBA8888_Premultiplied);
    render::CpuRenderer cpu;
    cpu.render(c, out.bits(), int(out.bytesPerLine()));
    return out;
}

int px(const QImage& img, int x, int y, int k) {
    return img.constScanLine(y)[x * 4 + k];
}

int tex(const media::Frame& f, int x, int y, int k) {
    if (x < 0 || y < 0 || x >= f.width || y >= f.height) return 0;
    return f.data.get()[std::ptrdiff_t(y) * f.stride + x * 4 + k];
}

const Case& find(const std::vector<Case>& list, const char* name) {
    for (const Case& c : list)
        if (QByteArray(c.name) == name) return c;
    qFatal("no case %s", name);
    return list.front(); // not reached
}

} // namespace

class TestRendererCpu : public QObject {
    Q_OBJECT
private slots:
    void identity() {
        const auto list = cases();
        const Case& c = find(list, "identity");
        const QImage out = run(c.composite);
        const media::Frame& g = c.composite.layers[0].frame;
        for (int y = 0; y < 12; ++y)
            for (int x = 0; x < 16; ++x)
                for (int k = 0; k < 4; ++k)
                    QCOMPARE(px(out, x, y, k), tex(g, x, y, k));
    }
    void halfSizeAverages() {
        const auto list = cases();
        const Case& c = find(list, "half size");
        const QImage out = run(c.composite);
        const media::Frame& g = c.composite.layers[0].frame;
        for (int y = 0; y < 6; ++y)
            for (int x = 0; x < 8; ++x)
                for (int k = 0; k < 4; ++k) {
                    const double avg = (tex(g, 2 * x, 2 * y, k) + tex(g, 2 * x + 1, 2 * y, k) +
                                        tex(g, 2 * x, 2 * y + 1, k) + tex(g, 2 * x + 1, 2 * y + 1, k)) /
                                       4.0;
                    QVERIFY(std::abs(px(out, x, y, k) - avg) <= 0.5 + 1e-9);
                }
    }
    void mirrored() {
        const auto list = cases();
        const Case& c = find(list, "flip H");
        const QImage out = run(c.composite);
        const media::Frame& g = c.composite.layers[0].frame;
        for (int y = 0; y < 12; ++y)
            for (int x = 0; x < 16; ++x)
                QCOMPARE(px(out, x, y, 0), tex(g, 15 - x, y, 0));
    }
    void halfTexelBetweenNeighbours() {
        const auto list = cases();
        const Case& c = find(list, "half-texel offset");
        const QImage out = run(c.composite);
        const media::Frame& g = c.composite.layers[0].frame;
        for (int x = 0; x < 16; ++x) {
            const double want = (tex(g, x, 3, 1) + tex(g, x + 1, 3, 1)) / 2.0; // the last one half transparent
            QVERIFY(std::abs(px(out, x, 3, 1) - want) <= 0.5 + 1e-9);
        }
        QCOMPARE(px(out, 15, 3, 3), 128); // half of 255, rounded
    }
    void outsideTransparent() {
        const auto list = cases();
        const QImage out = run(find(list, "outside is transparent").composite);
        QCOMPARE(px(out, 0, 0, 3), 0);
        QCOMPARE(px(out, 15, 11, 3), 0);
        QCOMPARE(px(out, 8, 6, 3), 255);
    }
    void blend() {
        const auto list = cases();
        const QImage out = run(find(list, "blend 0.3").composite);
        const int want[4] = {int(std::lround(0.7 * 200)), int(std::lround(0.7 * 20 + 0.3 * 40)),
                             int(std::lround(0.7 * 10 + 0.3 * 180)), 255};
        for (int k = 0; k < 4; ++k)
            QCOMPARE(px(out, 3, 3, k), want[k]);
    }
    void mask() {
        const auto list = cases();
        const Case& c = find(list, "mask");
        const QImage out = run(c.composite);
        const media::Frame& g = c.composite.layers[0].frame;
        for (int y = 0; y < 12; ++y)
            for (int x = 0; x < 16; ++x) {
                const double m = c.mask->constScanLine(y)[x] / 255.0;
                QCOMPARE(px(out, x, y, 3), int(std::lround(255 * m)));
                QVERIFY(std::abs(px(out, x, y, 0) - tex(g, x, y, 0) * m) <= 0.5 + 1e-6);
            }
    }
    void opaque() {
        const auto list = cases();
        const Case& c = find(list, "opaque over a colour");
        const QImage out = run(c.composite);
        for (int x = 0; x < 8; ++x) {
            const double a = x * 32 / 255.0;
            QCOMPARE(px(out, x, 2, 3), 255);
            QVERIFY(std::abs(px(out, x, 2, 0) - (x * 16 + 30 * (1 - a))) <= 0.5 + 1e-6);
            QVERIFY(std::abs(px(out, x, 2, 2) - (90 * (1 - a))) <= 0.5 + 1e-6);
        }
    }
};

QTEST_GUILESS_MAIN(TestRendererCpu)
#include "test-renderer-cpu.moc"
