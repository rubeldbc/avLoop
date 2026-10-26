#include "export/LoopFinder.h"

#include "core/Timecode.h"
#include "media/Decoder.h"

#include <QCoreApplication>
#include <QtConcurrent>

#include <algorithm>
#include <array>
#include <cmath>
#include <numeric>

namespace avloop::exporting {

namespace {

constexpr int kKeepPerRow = 8;      // the best pairs a row keeps for the suppression
constexpr int kRowsAChunk = 64;     // rows scored in parallel between two progress reports
constexpr int kBins = 5110;         // the raw score's histogram: 0 … 511 grey levels in tenths
constexpr double kSameFrame = 1e-6; // as the FrameRenderer asks for a frame
constexpr QSize kThumb(120, 68);

QString tr(const char* text) {
    return QCoreApplication::translate("LoopFinder", text);
}

// Where scene point p falls in the decoded frame f (pixels), or false outside the video.
bool locate(const QPointF& p, const QRectF& video, bool flipH, bool flipV, const media::Frame& f, double& x,
            double& y) {
    double u = (p.x() - video.left()) / video.width();
    double v = (p.y() - video.top()) / video.height();
    if (u < 0 || u >= 1 || v < 0 || v >= 1) return false;
    if (flipH) u = 1 - u;
    if (flipV) v = 1 - v;
    x = u * f.width - 0.5;
    y = v * f.height - 0.5;
    return true;
}

// The premultiplied RGBA of f at (x, y), bilinear.
std::array<double, 4> sample(const media::Frame& f, double x, double y) {
    const int x0 = std::clamp(int(std::floor(x)), 0, f.width - 1), y0 = std::clamp(int(std::floor(y)), 0, f.height - 1);
    const int x1 = std::min(x0 + 1, f.width - 1), y1 = std::min(y0 + 1, f.height - 1);
    const double fx = std::clamp(x - x0, 0.0, 1.0), fy = std::clamp(y - y0, 0.0, 1.0);
    const std::uint8_t* d = f.data.get();
    auto at = [&](int px, int py, int c) { return double(d[std::ptrdiff_t(py) * f.stride + px * 4 + c]); };
    std::array<double, 4> out{};
    for (int c = 0; c < 4; ++c)
        out[c] = (at(x0, y0, c) * (1 - fx) + at(x1, y0, c) * fx) * (1 - fy) +
                 (at(x0, y1, c) * (1 - fx) + at(x1, y1, c) * fx) * fy;
    return out;
}

// The export's frame at time t drawn into w × h: grey (one byte a pixel) or premultiplied RGBA.
void drawFrame(const LoopFinder::Input& in, const media::Frame& f, double t, int w, int h, bool grey,
               std::uint8_t* out) {
    const VideoTransform place = keyframes::valueAt(in.keyframes, in.transform, t);
    const QRectF video = place.rect(QSizeF(in.source.width, in.source.height));
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) {
            const QPointF p(in.frame.left() + (x + 0.5) / w * in.frame.width(),
                            in.frame.top() + (y + 0.5) / h * in.frame.height());
            double sx = 0, sy = 0;
            std::array<double, 4> c{};
            if (locate(p, video, place.flipH, place.flipV, f, sx, sy)) c = sample(f, sx, sy);
            if (grey) {
                out[y * w + x] = std::uint8_t(std::lround((77 * c[0] + 150 * c[1] + 29 * c[2]) / 256));
            } else {
                for (int k = 0; k < 4; ++k)
                    out[(y * w + x) * 4 + k] = std::uint8_t(std::lround(c[k]));
            }
        }
}

media::Decoder::Options smallDecoder() {
    media::Decoder::Options o;
    o.hardware = false;
    o.maxLongSide = 640; // fingerprints and thumbnails need no more
    o.threads = 2;
    return o;
}

struct Pair {
    int a = 0;
    int b = 0;
    double raw = 0;
};

} // namespace

bool LoopFinder::fingerprints(const Input& in, std::vector<std::uint8_t>& prints, int& frames,
                              const std::atomic<bool>& cancel, const Progress& progress, QString* error) {
    const std::int64_t first = timecode::nearestFrame(in.from, in.fps);
    frames = int(std::max<std::int64_t>(0, timecode::nearestFrame(in.to, in.fps) - first));
    prints.clear();
    if (frames < 2) {
        if (error) *error = tr("The search range is shorter than two frames.");
        return false;
    }
    media::Decoder decoder(in.source, smallDecoder());
    if (!decoder.open(error)) return false;
    prints.assign(std::size_t(frames) * kPrint, 0);
    for (int i = 0; i < frames; ++i) {
        if (cancel) return false;
        const double t = timecode::secondsOf(first + i, in.fps);
        const media::Frame f = decoder.frameAt(t + kSameFrame);
        if (f.isNull()) {
            if (error) *error = tr("Frame %1 did not decode.").arg(timecode::format(t, in.fps));
            return false;
        }
        drawFrame(in, f, t, kSide, kSide, true, prints.data() + std::size_t(i) * kPrint);
        if (progress && (i % 8 == 7 || i + 1 == frames)) progress(i + 1, frames);
    }
    return true;
}

QList<LoopFinder::Candidate> LoopFinder::best(const std::vector<std::uint8_t>& prints, int frames, int minFrames,
                                              int maxFrames, int apart, int count, const std::atomic<bool>& cancel,
                                              const Progress& progress) {
    minFrames = std::max(1, minFrames);
    // The motion images: frame a minus frame a − 1, signed.
    std::vector<std::int16_t> motion(std::size_t(frames) * kPrint, 0);
    for (int a = 1; a < frames; ++a)
        for (int i = 0; i < kPrint; ++i)
            motion[std::size_t(a) * kPrint + i] =
                std::int16_t(prints[std::size_t(a) * kPrint + i] - prints[std::size_t(a - 1) * kPrint + i]);
    auto score = [&](int a, int b) {
        const std::uint8_t* pa = prints.data() + std::size_t(a) * kPrint;
        const std::uint8_t* pb = prints.data() + std::size_t(b) * kPrint;
        const std::int16_t* ma = motion.data() + std::size_t(a) * kPrint;
        const std::int16_t* mb = motion.data() + std::size_t(b) * kPrint;
        int frame = 0, move = 0;
        for (int i = 0; i < kPrint; ++i) {
            frame += std::abs(int(pa[i]) - int(pb[i]));
            move += std::abs(int(ma[i]) - int(mb[i]));
        }
        return double(frame + move) / kPrint;
    };
    struct Row {
        QList<Pair> keep;         // its best few
        std::vector<double> raws; // all of its scores, for the scale
    };
    auto scoreRow = [&](int a) {
        Row row;
        if (cancel) return row;
        for (int b = a + minFrames; b <= std::min(frames - 1, a + maxFrames); ++b) {
            const double raw = score(a, b);
            row.raws.push_back(raw);
            row.keep.append({a, b, raw});
        }
        std::sort(row.keep.begin(), row.keep.end(), [](const Pair& x, const Pair& y) { return x.raw < y.raw; });
        if (row.keep.size() > kKeepPerRow) row.keep.resize(kKeepPerRow);
        return row;
    };
    QList<Pair> pairs;
    std::vector<std::int64_t> histogram(kBins + 1, 0);
    std::int64_t scored = 0;
    const int rows = std::max(0, frames - 1 - minFrames); // a from 1 (it needs a frame before it for its motion)
    for (int from = 1; from <= rows; from += kRowsAChunk) {
        if (cancel) return {};
        QList<int> chunk(std::min(kRowsAChunk, rows - from + 1));
        std::iota(chunk.begin(), chunk.end(), from);
        const QList<Row> done = QtConcurrent::blockingMapped(chunk, std::function<Row(const int&)>(scoreRow));
        for (const Row& row : done) {
            pairs += row.keep;
            for (double raw : row.raws)
                ++histogram[std::min(kBins, int(raw * 10))];
            scored += std::int64_t(row.raws.size());
        }
        if (progress) progress(std::min(rows, from + kRowsAChunk - 1), rows);
    }
    if (cancel || pairs.isEmpty()) return {};
    // The scale: 0 the best possible, 100 at the 95th percentile of every pair's score (stable against outliers).
    double rawMax = 0;
    for (std::int64_t seen = 0, bin = 0; bin <= kBins; ++bin) {
        seen += histogram[bin];
        if (seen >= scored * 95 / 100) {
            rawMax = (bin + 1) / 10.0;
            break;
        }
    }
    // Lower first; at a tie the shorter loop, then the earlier one.
    std::sort(pairs.begin(), pairs.end(), [](const Pair& x, const Pair& y) {
        if (x.raw != y.raw) return x.raw < y.raw;
        if (x.b - x.a != y.b - y.a) return x.b - x.a < y.b - y.a;
        return x.a < y.a;
    });
    QList<Pair> chosen;
    for (const Pair& p : pairs) {
        const bool near = std::any_of(chosen.begin(), chosen.end(), [&](const Pair& c) {
            return std::abs(c.a - p.a) < apart && std::abs(c.b - p.b) < apart;
        });
        if (!near) chosen.append(p);
        if (chosen.size() == count) break;
    }
    QList<Candidate> answer;
    for (const Pair& p : chosen) {
        Candidate c;
        c.in = p.a; // frame indices here; find() turns them into seconds
        c.out = p.b;
        c.raw = p.raw;
        c.score = rawMax > 0 ? std::clamp(int(std::lround(100 * (1 - p.raw / rawMax))), 0, 100) : 100;
        answer.append(c);
    }
    return answer;
}

QList<LoopFinder::Candidate> LoopFinder::find(const Input& in, const std::atomic<bool>& cancel,
                                              const Progress& progress, QString* error) {
    std::vector<std::uint8_t> prints;
    int frames = 0;
    // Progress: the fingerprints are the first half, the scoring the second.
    auto half = [&](int base) {
        return [&, base](int done, int total) {
            if (progress) progress(base + done * 500 / std::max(1, total), 1000);
        };
    };
    if (!fingerprints(in, prints, frames, cancel, half(0), error)) return {};
    const double fps = in.fps.value();
    QList<Candidate> answer = best(prints, frames, int(std::lround(in.minLength * fps)),
                                   int(std::lround(in.maxLength * fps)), int(std::lround(fps)), 5, cancel, half(500));
    prints = {}; // the fingerprints go as soon as the scores are made
    if (answer.isEmpty()) {
        if (error && !cancel) *error = tr("No loop fits these lengths in the search range.");
        return {};
    }
    // The thumbnails of both ends: the first frame and the last of each loop.
    media::Decoder decoder(in.source, smallDecoder());
    const bool open = decoder.open();
    const std::int64_t first = timecode::nearestFrame(in.from, in.fps);
    const QSize thumb = in.frame.size().scaled(QSizeF(kThumb), Qt::KeepAspectRatio).toSize().expandedTo(QSize(8, 8));
    auto picture = [&](int frame) {
        const double t = timecode::secondsOf(first + frame, in.fps);
        const media::Frame f = open ? decoder.frameAt(t + kSameFrame) : media::Frame{};
        QImage image(thumb, QImage::Format_RGBA8888_Premultiplied);
        image.fill(Qt::transparent);
        if (!f.isNull()) drawFrame(in, f, t, thumb.width(), thumb.height(), false, image.bits());
        return image;
    };
    for (Candidate& c : answer) {
        const int a = int(c.in), b = int(c.out);
        c.in = timecode::secondsOf(first + a, in.fps);
        c.out = timecode::secondsOf(first + b, in.fps);
        c.first = picture(a);
        c.last = picture(b - 1);
    }
    return answer;
}

} // namespace avloop::exporting
