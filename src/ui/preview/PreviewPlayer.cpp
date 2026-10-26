#include "ui/preview/PreviewPlayer.h"

#include "render/FrameRenderer.h"
#include "render/GlRenderer.h"
#include "render/RenderService.h"

#include <QCoreApplication>

#include <cmath>

namespace avloop {

PreviewPlayer::PreviewPlayer(render::RenderService* service, media::DiskCache* disk, std::int64_t cacheBytes,
                             QObject* parent)
    : QObject(parent), service_(service), disk_(disk), cacheBytes_(cacheBytes), live_(std::make_unique<Track>()),
      encoded_(std::make_unique<Track>()) {
    timer_.setTimerType(Qt::PreciseTimer);
    timer_.setInterval(4);
    connect(&timer_, &QTimer::timeout, this, &PreviewPlayer::tick);
}

PreviewPlayer::~PreviewPlayer() {
    life_.reset(); // results still on their way find nobody
    if (cancelEncode_) *cancelEncode_ = true;
}

PreviewPlayer::Track& PreviewPlayer::track() {
    return mode_ == Mode::Encoded ? *encoded_ : *live_;
}

void PreviewPlayer::setJob(const render::RenderJob& job) {
    job_ = job;
    ++generation_;
    inFlight_ = false;
    const int l = std::max(0, job_.plan.length);
    *live_ = Track{};
    live_->frames.resize(l);
    renderer_ =
        std::make_shared<render::FrameRenderer>(job_, disk_, render::FrameRenderer::gpuSetting(), service_->gl());
    auto renderer = renderer_;
    service_->post([renderer] {
        renderer->open();
        renderer->prepare(); // Motion Interpolation's pre-pass, when the loop uses it
    });
    live_->make = [renderer](int k) {
        QImage image;
        renderer->render(k, image);
        return image;
    };
    if (encodeState_ == Encode::Done || encodeState_ == Encode::Running) encodeState_ = Encode::OutOfDate;
    if (cancelEncode_) *cancelEncode_ = true;
    shown_ = std::clamp(shown_, 0, std::max(0, l - 1));
    step_ = std::clamp(step_, 0.0, double(std::max(0, passLength() - 1)));
    frame_ = {};
    emit stateChanged();
    ask();
}

bool PreviewPlayer::isCached(int k) const {
    const Track& t = mode_ == Mode::Encoded ? *encoded_ : *live_;
    return k >= 0 && k < t.frames.size() && !t.frames[k].isNull();
}

int PreviewPlayer::cachedCount() const {
    return (mode_ == Mode::Encoded ? *encoded_ : *live_).cached;
}

int PreviewPlayer::passLength() const {
    const int l = length();
    if (!aroundSeam_) return l;
    const int s = std::min(int(std::lround(2 * job_.fps.value())), l / 2); // the last 2 s + the first 2 s
    return std::max(1, 2 * s);
}

int PreviewPlayer::order(int i) const {
    const int l = length();
    if (l <= 0) return 0;
    const int pass = passLength();
    i = ((i % pass) + pass) % pass;
    if (!aroundSeam_ || pass >= l) return i;
    const int s = pass / 2;
    return i < s ? l - s + i : i - s;
}

void PreviewPlayer::store(Track& t, int k, QImage image) {
    if (k < 0 || k >= t.frames.size() || image.isNull() || !t.frames[k].isNull()) return;
    const std::int64_t size = std::int64_t(image.sizeInBytes());
    // Over budget: the cached frame that comes back last (farthest behind the playhead) makes room.
    while (t.bytes + size > cacheBytes_ && t.cached > 0) {
        const int l = int(t.frames.size());
        int worst = -1;
        for (int d = l - 1; d > 0 && worst < 0; --d) {
            const int j = (shown_ + d) % l;
            if (!t.frames[j].isNull() && j != k) worst = j;
        }
        if (worst < 0) break;
        t.bytes -= t.frames[worst].sizeInBytes();
        t.frames[worst] = {};
        --t.cached;
    }
    t.frames[k] = std::move(image);
    t.bytes += size;
    ++t.cached;
}

void PreviewPlayer::ask() {
    Track& t = track();
    if (inFlight_ || !t.make || length() <= 0) return;
    // The next frame playback needs, from where it is; none when the cache would only trade one for another.
    const int pass = passLength();
    const std::int64_t frameBytes = std::int64_t(job_.outputSize.width()) * job_.outputSize.height() * 4;
    const int room = int(std::max<std::int64_t>(1, cacheBytes_ / std::max<std::int64_t>(1, frameBytes)));
    int k = -1;
    for (int s = 0; s < std::min(pass, room); ++s) {
        const int candidate = order(int(step_) + s);
        if (t.frames[candidate].isNull()) {
            k = candidate;
            break;
        }
    }
    if (k < 0) return;
    inFlight_ = true;
    const int generation = generation_;
    const Mode mode = mode_;
    std::weak_ptr<int> life = life_;
    service_->post([make = t.make, k, generation, mode, life, this] {
        QImage image = make(k);
        QMetaObject::invokeMethod(qApp, [life, this, generation, mode, k, image = std::move(image)]() mutable {
            if (!life.lock() || mode != mode_) return;
            delivered(generation, k, std::move(image));
        });
    });
}

void PreviewPlayer::delivered(int generation, int k, QImage image) {
    if (generation != generation_) return;
    inFlight_ = false;
    store(track(), k, std::move(image));
    if (k == order(int(step_)) && !playing_) show(k);
    emit stateChanged();
    ask();
}

void PreviewPlayer::show(int k) {
    if (!isCached(k)) return;
    shown_ = k;
    frame_ = track().frames[k];
    emit frameChanged(k);
}

void PreviewPlayer::tick() {
    const double dt = clock_.nsecsElapsed() / 1e9;
    clock_.restart();
    const int here = order(int(step_));
    if (!isCached(here)) {
        // Never a dropped frame: the clock waits for it ("Caching…").
        if (!caching_) {
            caching_ = true;
            emit stateChanged();
        }
        ask();
        return;
    }
    if (caching_) {
        caching_ = false;
        emit stateChanged();
    }
    if (here != shown_ || frame_.isNull()) show(here);
    const double before = std::floor(step_);
    step_ += dt * job_.fps.value();
    // The next frame is shown only once it is there: the position stops on it until then.
    const int pass = passLength();
    if (std::floor(step_) > before && !isCached(order(int(before) + 1))) step_ = before + 1;
    if (step_ >= pass) step_ = std::fmod(step_, pass);
    ask();
}

void PreviewPlayer::play() {
    if (playing_ || length() <= 0) return;
    playing_ = true;
    clock_.start();
    timer_.start();
    emit stateChanged();
}

void PreviewPlayer::pause() {
    if (!playing_) return;
    playing_ = false;
    timer_.stop();
    caching_ = false;
    step_ = std::floor(step_);
    show(order(int(step_)));
    emit stateChanged();
}

void PreviewPlayer::toggle() {
    playing_ ? pause() : play();
}

void PreviewPlayer::seek(int k) {
    const int l = length();
    if (l <= 0) return;
    k = ((k % l) + l) % l;
    int at = -1;
    for (int i = 0; i < passLength() && at < 0; ++i)
        if (order(i) == k) at = i;
    if (at < 0) { // outside Around seam's window: the whole loop again
        aroundSeam_ = false;
        at = k;
    }
    step_ = at;
    if (isCached(k)) show(k);
    emit stateChanged();
    ask();
}

void PreviewPlayer::step(int n) {
    pause();
    seek(shown_ + n);
}

void PreviewPlayer::setAroundSeam(bool on) {
    const int k = shown_;
    aroundSeam_ = on;
    step_ = 0;
    if (!on) step_ = k;
    emit stateChanged();
    ask();
}

void PreviewPlayer::setMode(Mode mode) {
    if (mode == mode_) return;
    mode_ = mode;
    ++generation_;
    inFlight_ = false;
    frame_ = {};
    show(order(int(step_)));
    emit stateChanged();
    ask();
}

} // namespace avloop
