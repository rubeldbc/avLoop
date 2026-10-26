#include "export/SizeEstimator.h"

#include "media/Encoder.h"
#include "render/FrameRenderer.h"
#include "render/RenderService.h"

#include <QCoreApplication>

#include <algorithm>
#include <functional>

namespace avloop::exporting {

namespace {

constexpr int kPartFrames = 10;
constexpr int kParts = 3;

// One measurement, a frame at a time: made on the render thread, encoded on the export thread — one frame in flight.
struct Measure {
    render::RenderService* service = nullptr;
    QObject* encodeContext = nullptr;
    std::weak_ptr<int> alive; // expires when a newer measurement starts or the estimator goes
    std::function<void(std::int64_t)> deliver;
    std::shared_ptr<render::FrameRenderer> renderer; // used on the render thread
    std::unique_ptr<media::Encoder> encoder;         // used on the export thread: the part under way
    render::RenderJob job;
    QList<int> starts;
    QList<SizeEstimator::Part> parts;
    bool opened = false;

    int partFrames(int i) const {
        return starts.size() == 1 ? job.plan.length : std::min(kPartFrames, job.plan.length - starts[i]);
    }
};

void deliver(const std::shared_ptr<Measure>& m, std::int64_t bytes) {
    QMetaObject::invokeMethod(qApp, [m, bytes] {
        if (!m->alive.expired()) m->deliver(bytes);
    });
}

void encodeFrame(const std::shared_ptr<Measure>& m, int i, int n, const QImage& image);

// On the render thread: frame n of part i.
void renderFrame(const std::shared_ptr<Measure>& m, int i, int n) {
    auto stop = [&](bool tell) {
        m->renderer.reset(); // its decoders let go here, where they were used
        if (tell) deliver(m, -1);
    };
    if (m->alive.expired()) return stop(false);
    if (!m->opened) {
        if (!m->renderer->open() || !m->renderer->prepare([m](int, int) { return !m->alive.expired(); }))
            return stop(true);
        m->opened = true;
    }
    QImage image;
    if (!m->renderer->render(m->starts[i] + n, image)) return stop(true);
    if (i + 1 == m->starts.size() && n + 1 == m->partFrames(i)) { // the last frame made: the renderer and its
        stop(false);                                              // textures let go
        m->service->trim();
    }
    QMetaObject::invokeMethod(m->encodeContext, [m, i, n, image] { encodeFrame(m, i, n, image); });
}

// On the export thread: frame n into its part's encoder; the part's end counts its packets.
void encodeFrame(const std::shared_ptr<Measure>& m, int i, int n, const QImage& image) {
    auto stop = [&](bool tell) {
        m->encoder.reset();
        m->service->post([m] { m->renderer.reset(); });
        if (tell) deliver(m, -1);
    };
    if (m->alive.expired()) return stop(false);
    if (n == 0) {
        media::EncodeSettings s;
        s.format = m->job.output.format;
        s.size = m->job.outputSize;
        s.fps = m->job.fps;
        s.frames = m->job.plan.length;
        s.alpha = m->job.alpha;
        s.quality = m->job.quality;
        s.lagInFrames = std::min(25, m->partFrames(i)); // the same encode for so few frames, half the buffers
        m->encoder = std::make_unique<media::Encoder>(s);
        if (!m->encoder->open(QString())) return stop(true);
    }
    if (!m->encoder->write(image.constBits(), int(image.bytesPerLine()))) return stop(true);
    if (n + 1 < m->partFrames(i)) {
        m->service->post([m, i, n] { renderFrame(m, i, n + 1); });
        return;
    }
    if (!m->encoder->finish()) return stop(true);
    m->parts << SizeEstimator::Part{m->partFrames(i), m->encoder->bytes(), m->encoder->keyBytes()};
    m->encoder.reset();
    if (i + 1 < m->starts.size())
        m->service->post([m, i] { renderFrame(m, i + 1, 0); });
    else
        deliver(m, SizeEstimator::extrapolate(m->parts, m->job.plan.length));
}

} // namespace

SizeEstimator::SizeEstimator(render::RenderService* service, QObject* encodeContext, media::DiskCache* disk,
                             QObject* parent)
    : QObject(parent), service_(service), encodeContext_(encodeContext), disk_(disk) {
    timer_.setSingleShot(true);
    timer_.setInterval(1000);
    connect(&timer_, &QTimer::timeout, this, &SizeEstimator::measure);
}

SizeEstimator::~SizeEstimator() = default; // the measurement in flight sees its generation expire

void SizeEstimator::request(const render::RenderJob& job) {
    job_ = job;
    generation_.reset();
    busy_ = true;
    timer_.start();
    emit changed();
}

void SizeEstimator::clear() {
    timer_.stop();
    generation_.reset();
    job_ = {};
    busy_ = false;
    bytes_ = -1;
    emit changed();
}

QString SizeEstimator::text() const {
    if (busy_) return QStringLiteral("…");
    return bytes_ < 0 ? QStringLiteral("—") : QStringLiteral("~") + sizeText(bytes_);
}

QString SizeEstimator::sizeText(std::int64_t bytes) {
    const double mb = bytes / 1048576.0;
    return QStringLiteral("%1 MB").arg(mb, 0, 'f', mb < 1 ? 2 : 1);
}

QList<int> SizeEstimator::partStarts(int length) {
    if (length <= kParts * kPartFrames) return {0};
    // The start, the middle and the end of the loop.
    return {0, length / 2 - kPartFrames / 2, length - kPartFrames};
}

std::int64_t SizeEstimator::extrapolate(const QList<Part>& parts, int length) {
    if (parts.isEmpty() || length <= 0) return -1;
    std::int64_t key = 0, inter = 0;
    int interFrames = 0;
    for (const Part& p : parts) {
        key += p.keyBytes;
        inter += p.bytes - p.keyBytes;
        interFrames += p.frames - 1;
    }
    key /= parts.size();
    const double perFrame = interFrames > 0 ? double(inter) / interFrames : 0.0;
    // The WebM around it: its header and cues, and a few bytes a block.
    constexpr int kHeader = 1024;
    constexpr int kPerBlock = 16;
    return key + std::int64_t(perFrame * (length - 1)) + kHeader + std::int64_t(kPerBlock) * length;
}

void SizeEstimator::measure() {
    const int L = job_.plan.length;
    const QualitySettings& q = job_.quality;
    if (L <= 0) return clear();
    if (q.preset == QualitySettings::Preset::Custom && q.targetMB > 0) {
        busy_ = false;
        bytes_ = std::int64_t(q.targetMB * 1048576.0);
        emit changed();
        return;
    }
    generation_ = std::make_shared<int>(0);
    auto m = std::make_shared<Measure>();
    m->service = service_;
    m->encodeContext = encodeContext_;
    m->alive = generation_;
    m->deliver = [this](std::int64_t bytes) {
        busy_ = false;
        bytes_ = bytes;
        emit changed();
    };
    m->job = job_;
    m->starts = partStarts(L);
    m->renderer =
        std::make_shared<render::FrameRenderer>(job_, disk_, render::FrameRenderer::gpuSetting(), service_->gl());
    service_->post([m] { renderFrame(m, 0, 0); });
}

} // namespace avloop::exporting
