// The encoder (spec §10.1–§10.3): ten frames of a loop with alpha written as VP9 and as VP8 WebM, read back through the
// libvpx decoder — size, rate, frame count, the codec, the alpha kept (clear corners, an opaque middle), the colour
// close, the first packet a keyframe, one GOP; without alpha, no alpha stream; a cancelled encode leaves no file.
#include "media/Decoder.h"
#include "media/Encoder.h"
#include "media/FfmpegPtr.h"
#include "media/MediaProbe.h"

#include <QFileInfo>
#include <QTemporaryDir>
#include <QTest>

#include <cmath>
#include <vector>

using namespace avloop;
using namespace avloop::media;

namespace {

// Frame k: a disc of colour on transparency, the colour moving with k (premultiplied).
std::vector<std::uint8_t> frameOf(int w, int h, int k) {
    std::vector<std::uint8_t> px(std::size_t(w) * h * 4);
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) {
            const double d = std::hypot(x + 0.5 - w / 2.0, y + 0.5 - h / 2.0);
            const int a = d < h * 0.35 ? 255 : 0;
            std::uint8_t* p = px.data() + (std::size_t(y) * w + x) * 4;
            p[0] = std::uint8_t((200 + k * 5) * a / 255);
            p[1] = std::uint8_t(80 * a / 255);
            p[2] = std::uint8_t((40 + k * 10) * a / 255);
            p[3] = std::uint8_t(a);
        }
    return px;
}

int keyframesIn(const QString& path, int* packets) {
    AVFormatContext* raw = nullptr;
    if (avformat_open_input(&raw, path.toUtf8().constData(), nullptr, nullptr) < 0) return -1;
    FormatContextPtr fmt(raw);
    PacketPtr p = makePacket();
    int keys = 0;
    *packets = 0;
    bool first = true;
    while (av_read_frame(fmt.get(), p.get()) >= 0) {
        if (first && !(p->flags & AV_PKT_FLAG_KEY)) keys = -100; // the first must be a keyframe
        first = false;
        keys += (p->flags & AV_PKT_FLAG_KEY) ? 1 : 0;
        ++*packets;
        av_packet_unref(p.get());
    }
    return keys;
}

} // namespace

class TestExport : public QObject {
    Q_OBJECT
private slots:
    void writesAlpha_data() {
        QTest::addColumn<int>("format");
        QTest::newRow("VP9") << int(OutputFormat::WebmVp9);
        QTest::newRow("VP8") << int(OutputFormat::WebmVp8);
    }
    void writesAlpha() {
        QFETCH(int, format);
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("loop.webm"));
        EncodeSettings s;
        s.format = OutputFormat(format);
        s.size = {96, 64};
        s.fps = {30, 1};
        s.frames = 10;
        s.alpha = true;
        Encoder enc(s);
        QString error;
        QVERIFY2(enc.open(path, &error), qPrintable(error));
        for (int k = 0; k < 10; ++k) {
            const auto px = frameOf(96, 64, k);
            QVERIFY2(enc.write(px.data(), 96 * 4, &error), qPrintable(error));
        }
        QVERIFY2(enc.finish(&error), qPrintable(error));
        QVERIFY(QFileInfo(path).size() > 100);

        const MediaProbe::Result r = MediaProbe::probe(path);
        QVERIFY2(r.info, qPrintable(r.error));
        QCOMPARE(r.info->width, 96);
        QCOMPARE(r.info->height, 64);
        QCOMPARE(r.info->codec, s.format == OutputFormat::WebmVp8 ? QStringLiteral("vp8") : QStringLiteral("vp9"));
        QVERIFY(r.info->hasAlpha);
        QCOMPARE(r.info->fps(), 30.0);
        int packets = 0;
        QCOMPARE(keyframesIn(path, &packets), 1); // one GOP, the first packet its keyframe
        QCOMPARE(packets, 10);

        Decoder::Options o;
        o.hardware = false;
        Decoder d(*r.info, o);
        QVERIFY(d.open());
        const Frame f = d.frameAtIndex(4);
        QVERIFY(!f.isNull());
        auto at = [&](int x, int y, int c) { return int(f.data.get()[std::ptrdiff_t(y) * f.stride + x * 4 + c]); };
        QCOMPARE(at(1, 1, 3), 0);     // a clear corner
        QCOMPARE(at(48, 32, 3), 255); // the opaque middle
        // The colour of frame 4, close after compression.
        const QString seen = QStringLiteral("%1,%2,%3").arg(at(48, 32, 0)).arg(at(48, 32, 1)).arg(at(48, 32, 2));
        QVERIFY2(std::abs(at(48, 32, 0) - 220) <= 12 && std::abs(at(48, 32, 2) - 80) <= 12, qPrintable(seen));
    }
    void withoutAlpha() {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("opaque.webm"));
        EncodeSettings s;
        s.size = {64, 64};
        s.frames = 3;
        s.alpha = false;
        Encoder enc(s);
        QVERIFY(enc.open(path));
        for (int k = 0; k < 3; ++k) {
            const auto px = frameOf(64, 64, k);
            QVERIFY(enc.write(px.data(), 64 * 4));
        }
        QVERIFY(enc.finish());
        const MediaProbe::Result r = MediaProbe::probe(path);
        QVERIFY(r.info);
        QVERIFY(!r.info->hasAlpha);
    }
    void cancelLeavesNoFile() {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("cancelled.webm"));
        EncodeSettings s;
        s.size = {64, 64};
        s.frames = 5;
        {
            Encoder enc(s);
            QVERIFY(enc.open(path));
            const auto px = frameOf(64, 64, 0);
            QVERIFY(enc.write(px.data(), 64 * 4));
            enc.abort();
        }
        QVERIFY(!QFileInfo::exists(path));
    }
};

QTEST_GUILESS_MAIN(TestExport)
#include "test-export.moc"
