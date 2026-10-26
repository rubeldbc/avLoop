// The clip effects (D-017): none leaves the frame untouched, byte for byte; each effect moves a pixel the way its name
// says (exposure in linear light, contrast about the middle, the four tones, white balance, hue, saturation, sepia,
// invert, posterize, the vignette); the pass keeps transparent pixels clear, halves a pixel at Opacity 50 %, spreads a
// point by Blur keeping its sum and holding the edges, overshoots an edge by Sharpen; the file's keys, clamped; the
// project — one Undo step a field, a new video without effects, Opacity under 100 % asking for alpha.
#include "core/ClipEffects.h"
#include "core/Project.h"
#include "core/ProjectJson.h"
#include "render/EffectsPass.h"

#include <QTest>

#include <array>
#include <cmath>
#include <memory>
#include <numeric>
#include <vector>

using namespace avloop;

namespace {

// A w × h premultiplied RGBA8 frame, every pixel `px`.
media::Frame frame(int w, int h, std::array<std::uint8_t, 4> px) {
    auto data = std::make_shared<std::vector<std::uint8_t>>(std::size_t(w) * h * 4);
    for (std::size_t i = 0; i < data->size(); ++i)
        (*data)[i] = px[i % 4];
    media::Frame f;
    f.width = w;
    f.height = h;
    f.stride = w * 4;
    f.pts = 7;
    f.data = std::shared_ptr<const std::uint8_t>(data, data->data());
    return f;
}

std::uint8_t* pixels(media::Frame& f) {
    return const_cast<std::uint8_t*>(f.data.get());
}

const std::uint8_t* at(const media::Frame& f, int x, int y) {
    return f.data.get() + std::ptrdiff_t(y) * f.stride + x * 4;
}

std::array<float, 3> coloured(const ClipEffects& e, float r, float g, float b) {
    float rgb[3] = {r, g, b};
    e.colour(rgb);
    return {rgb[0], rgb[1], rgb[2]};
}

} // namespace

class TestEffects : public QObject {
    Q_OBJECT
private slots:
    void noneIsUntouched() {
        render::EffectsPass pass;
        const media::Frame f = frame(8, 4, {10, 20, 30, 255});
        QCOMPARE(pass.apply(ClipEffects{}, f, 1).data, f.data);
        ClipEffects off;
        off.exposure = 2;
        off.on = false; // kept, not applied
        QVERIFY(off.isNeutral());
        QCOMPARE(pass.apply(off, f, 1).data, f.data);
        QCOMPARE(ClipEffects{}.key(), std::uint64_t(0));
        off.on = true;
        QVERIFY(off.key() != 0);
        ClipEffects other = off;
        other.exposure = 2.1;
        QVERIFY(other.key() != off.key());
    }
    void light() {
        ClipEffects e;
        e.exposure = 1; // one stop: twice the light
        const float mid = coloured(e, 0.5f, 0.5f, 0.5f)[0];
        QVERIFY(std::abs(mid - effects::toSrgb(2 * effects::toLinear(0.5f))) < 1e-4f);
        QVERIFY(std::abs(mid - 0.686f) < 0.003f);
        e = {};
        e.contrast = 50; // twice the slope about 0.5
        QVERIFY(std::abs(coloured(e, 0.25f, 0.5f, 0.75f)[0] - 0.0f) < 1e-5f);
        QVERIFY(std::abs(coloured(e, 0.25f, 0.5f, 0.75f)[1] - 0.5f) < 1e-5f);
        QVERIFY(std::abs(coloured(e, 0.25f, 0.5f, 0.75f)[2] - 1.0f) < 1e-5f);
        e = {};
        e.shadows = 100; // the darks more than the lights
        const auto lifted = coloured(e, 0.25f, 0.75f, 0.0f);
        QVERIFY(lifted[0] - 0.25f > lifted[1] - 0.75f);
        QCOMPARE(lifted[2], 0.0f); // black stays
        e = {};
        e.highlights = -100; // the lights down more than the darks
        const auto lowered = coloured(e, 0.25f, 0.75f, 1.0f);
        QVERIFY(0.75f - lowered[1] > 0.25f - lowered[0]);
        QCOMPARE(lowered[2], 1.0f); // white stays
        e = {};
        e.blacks = 100;
        QVERIFY(coloured(e, 0, 0, 0)[0] > 0.2f);
        e = {};
        e.whites = -100;
        QVERIFY(coloured(e, 1, 1, 1)[0] < 0.8f);
    }
    void colour() {
        ClipEffects e;
        e.temperature = 100; // warmer: red up, blue down
        const auto warm = coloured(e, 0.5f, 0.5f, 0.5f);
        QVERIFY(warm[0] > 0.5f && warm[2] < 0.5f);
        e = {};
        e.tint = 100; // more magenta: green down
        QVERIFY(coloured(e, 0.5f, 0.5f, 0.5f)[1] < 0.5f);
        e = {};
        e.saturation = 0; // grey of the same luma
        const auto grey = coloured(e, 1, 0, 0);
        QVERIFY(std::abs(grey[0] - 0.2126f) < 1e-5f && grey[0] == grey[1] && grey[1] == grey[2]);
        e = {};
        e.hue = 180; // red turns cyan
        const auto cyan = coloured(e, 1, 0, 0);
        QCOMPARE(cyan[0], 0.0f);
        QVERIFY(std::abs(cyan[1] - cyan[2]) < 1e-5f && cyan[1] > 0.4f);
        e = {};
        e.vibrance = 100; // a dull colour more than a strong one
        const auto dull = coloured(e, 0.5f, 0.4f, 0.4f);
        const auto strong = coloured(e, 1.0f, 0.0f, 0.0f);
        QVERIFY(dull[0] - dull[1] > 0.1f + 0.05f);
        QCOMPARE(strong[0], 1.0f);
        e = {};
        e.sepia = 100;
        const auto old = coloured(e, 0.5f, 0.5f, 0.5f);
        QVERIFY(old[0] > old[1] && old[1] > old[2]);
        e = {};
        e.invert = true;
        const auto inverted = coloured(e, 0.2f, 0.4f, 0.6f);
        QVERIFY(std::abs(inverted[0] - 0.8f) < 1e-6f && std::abs(inverted[2] - 0.4f) < 1e-6f);
        e = {};
        e.posterize = 2;
        QCOMPARE(e.posterized(0.3f), 0.0f);
        QCOMPARE(e.posterized(0.6f), 1.0f);
        e.posterize = 3;
        QCOMPARE(e.posterized(0.4f), 0.5f);
    }
    void vignette() {
        ClipEffects e;
        e.vignette = 100;
        QCOMPARE(e.vignetteWeight(0.5f, 0.5f), 0.0f);
        QCOMPARE(e.vignetteWeight(0.0f, 0.0f), 1.0f);
        float rgb[3] = {0.8f, 0.8f, 0.8f};
        e.vignetteOn(rgb, 1);
        QCOMPARE(rgb[0], 0.0f);
        e.vignette = -50; // half way to white
        float light[3] = {0.2f, 0.2f, 0.2f};
        e.vignetteOn(light, 1);
        QVERIFY(std::abs(light[0] - 0.6f) < 1e-6f);
    }
    void passPerPixel() {
        render::EffectsPass pass;
        ClipEffects e;
        e.invert = true;
        // Clear pixels stay clear; an opaque one takes colour() exactly, rounded once.
        media::Frame f = frame(4, 2, {0, 0, 0, 0});
        std::uint8_t* p = pixels(f);
        p[0] = 51, p[1] = 102, p[2] = 153, p[3] = 255;
        const media::Frame out = pass.apply(e, f, 1);
        QVERIFY(out.data != f.data);
        QCOMPARE(out.pts, f.pts);
        QCOMPARE(int(at(out, 0, 0)[0]), 204);
        QCOMPARE(int(at(out, 0, 0)[2]), 102);
        QCOMPARE(int(at(out, 1, 0)[3]), 0);
        QCOMPARE(int(at(out, 1, 0)[0]), 0); // inverted black would be white: a clear pixel paints nothing
        // Opacity 50 %: alpha and the premultiplied colour halved.
        e = {};
        e.opacity = 50;
        const media::Frame half = pass.apply(e, frame(2, 2, {200, 100, 50, 255}), 1);
        QCOMPARE(int(at(half, 1, 1)[3]), 128);
        QCOMPARE(int(at(half, 1, 1)[0]), 100);
        QCOMPARE(int(at(half, 1, 1)[2]), 25);
    }
    void passBlur() {
        render::EffectsPass pass;
        ClipEffects e;
        e.blur = 6; // σ 3
        // A flat picture stays flat (the edges held, not faded).
        const media::Frame flat = pass.apply(e, frame(40, 30, {90, 60, 30, 255}), 1);
        for (int y : {0, 15, 29})
            for (int x : {0, 20, 39})
                QCOMPARE(int(at(flat, x, y)[0]) * 1000 + int(at(flat, x, y)[3]), 90 * 1000 + 255);
        // A white 5 × 5 square in black spreads, its sum kept within rounding.
        media::Frame dot = frame(41, 41, {0, 0, 0, 255});
        std::uint8_t* d = pixels(dot);
        for (int y = 18; y <= 22; ++y)
            for (int x = 18; x <= 22; ++x)
                std::fill(d + (y * 41 + x) * 4, d + (y * 41 + x) * 4 + 3, std::uint8_t(255));
        const media::Frame spread = pass.apply(e, dot, 1);
        QVERIFY(at(spread, 20, 20)[0] < 255 && at(spread, 20, 20)[0] > 0);
        QVERIFY(at(spread, 25, 20)[0] > 0);
        QCOMPARE(int(at(spread, 0, 0)[0]), 0);
        int sum = 0;
        for (int y = 0; y < 41; ++y)
            for (int x = 0; x < 41; ++x)
                sum += at(spread, x, y)[0];
        QVERIFY2(std::abs(sum - 25 * 255) <= 25 * 255 / 30, qPrintable(QString::number(sum)));
        // The scale: half the pixels a source pixel, half the spread.
        const media::Frame small = pass.apply(e, dot, 0.5);
        QVERIFY(at(small, 20, 20)[0] > at(spread, 20, 20)[0]);
    }
    void passSharpen() {
        render::EffectsPass pass;
        ClipEffects e;
        e.sharpen = 100;
        // A step 64 | 192: each side pushed away from the other at the edge.
        media::Frame f = frame(16, 4, {64, 64, 64, 255});
        std::uint8_t* p = pixels(f);
        for (int y = 0; y < 4; ++y)
            for (int x = 8; x < 16; ++x)
                std::fill(p + (y * 16 + x) * 4, p + (y * 16 + x) * 4 + 3, std::uint8_t(192));
        const media::Frame out = pass.apply(e, f, 1);
        QVERIFY(at(out, 7, 1)[0] < 64);
        QVERIFY(at(out, 8, 1)[0] > 192);
        QCOMPARE(int(at(out, 1, 1)[0]), 64); // far from the edge, unchanged
        QCOMPARE(int(at(out, 8, 1)[3]), 255);
    }
    void theFile() {
        ClipEffects e;
        e.exposure = 1.5;
        e.hue = -30;
        e.invert = true;
        e.posterize = 4;
        e.blur = 2.5;
        e.on = false;
        QCOMPARE(ClipEffects::fromJson(e.toJson()), e);
        QJsonObject wild = e.toJson();
        wild.insert(QStringLiteral("exposure"), 99);
        wild.insert(QStringLiteral("posterize"), 1);
        wild.insert(QStringLiteral("saturation"), -5);
        const ClipEffects clamped = ClipEffects::fromJson(wild);
        QCOMPARE(clamped.exposure, 4.0);
        QCOMPARE(clamped.posterize, 0);
        QCOMPARE(clamped.saturation, 0.0);
        QCOMPARE(ClipEffects::fromJson({}), ClipEffects{}); // a file without them: none
        ProjectData d;
        d.source.path = QStringLiteral("D:/a.mp4");
        d.effects = e;
        ProjectData back;
        QString error;
        QVERIFY(projectjson::fromJson(projectjson::toJson(d), back, &error));
        QCOMPARE(back.effects, e);
    }
    void theProject() {
        Project p;
        SourceInfo s;
        s.path = QStringLiteral("x.mp4");
        s.width = 640;
        s.height = 360;
        s.fps = {30, 1};
        s.duration = 10;
        p.setSource(s);
        QVERIFY(!p.outputAlpha());
        ClipEffects e;
        for (double v : {10.0, 20.0, 30.0}) { // typing in one field: one step
            e.contrast = v;
            p.changeEffects(e, QStringLiteral("field:effects.contrast"));
        }
        QCOMPARE(p.undoStack()->count(), 1);
        QCOMPARE(p.effects().contrast, 30.0);
        e.opacity = 50;
        p.changeEffects(e);
        QVERIFY(p.outputAlpha()); // see-through: Auto keeps alpha
        p.undoStack()->undo();
        QVERIFY(!p.outputAlpha());
        p.undoStack()->undo();
        QCOMPARE(p.effects(), ClipEffects{});
        p.undoStack()->redo();
        p.setSource(s); // a new video: none
        QCOMPARE(p.effects(), ClipEffects{});
    }
};

QTEST_GUILESS_MAIN(TestEffects)
#include "test-effects.moc"
