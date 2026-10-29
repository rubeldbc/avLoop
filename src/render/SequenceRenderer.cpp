#include "render/SequenceRenderer.h"

#include "core/Geometry.h"
#include "media/Decoder.h"
#include "media/FrameIndex.h"
#include "media/MediaProbe.h"
#include "render/CpuRenderer.h"
#include "render/MaskBuilder.h"

#include <QtConcurrent/QtConcurrentMap>

#include <algorithm>
#include <cmath>
#include <numeric>

namespace avloop::render {

namespace {

constexpr double kSameFrame = 1e-6; // seconds: two times this close name the same frame
constexpr size_t kMaxMasks = 4;

inline std::uint8_t unorm(float v) {
    return std::uint8_t(std::lround(std::clamp(v, 0.f, 1.f) * 255.f));
}

// A clip's masks from its own px to the sequence's: its rectangle's top-left + p × its scale (flips aside).
QList<MaskShape> placed(const QList<MaskShape>& masks, const QRectF& rect, const QSizeF& size) {
    const double k = size.width() > 0 ? rect.width() / size.width() : 1.0;
    const auto map = [&](const QPointF& p) { return rect.topLeft() + p * k; };
    QList<MaskShape> out;
    for (MaskShape m : masks) {
        m.rect = QRectF(map(m.rect.topLeft()), m.rect.size() * k);
        m.radius *= k;
        for (PathPoint& pt : m.points) {
            pt.p = map(pt.p);
            pt.in = map(pt.in);
            pt.out = map(pt.out);
        }
        out.append(m);
    }
    return out;
}

} // namespace

SequenceRenderer::SequenceRenderer(const Sequence& sequence, const QRectF& area, const QSize& outputSize,
                                   LoopSettings::Method method, bool alpha, const QColor& background)
    : sequence_(sequence), area_(area), size_(outputSize), method_(method), alpha_(alpha), background_(background) {
    if (!sequence_.matte.isEmpty() || maskshapes::anyVisible(sequence_.matte.shapes))
        matte_ = MaskBuilder::build(sequence_.matte, area_, size_);
    rows_.resize(size_.height());
    std::iota(rows_.begin(), rows_.end(), 0);
}

SequenceRenderer::~SequenceRenderer() = default;

void SequenceRenderer::setMotion(int clipId, std::function<media::Frame(int)> frameOf) {
    motionClip_ = clipId;
    motion_ = std::move(frameOf);
}

bool SequenceRenderer::plain(const Sequence& s) {
    if (s.clipCount() != 1 || s.tracks.isEmpty() || s.tracks[0].clips.size() != 1 || !s.tracks[0].visible) return false;
    const Clip& c = s.tracks[0].clips[0];
    const bool opaque = std::all_of(c.keys.begin(), c.keys.end(), [](const LayerKey& k) { return k.opacity >= 100; });
    return c.enabled && c.start == 0 && c.range.in == 0 && c.masks.isEmpty() && c.opacity >= 100 && opaque;
}

int SequenceRenderer::decodersOpen() const {
    return int(std::count_if(sources_.begin(), sources_.end(), [](const Source& s) { return bool(s.decoder); }));
}

QSize SequenceRenderer::workingSize(const Clip& clip, const QSizeF& display, const QRectF& area) {
    double largest = 0;
    const auto consider = [&](double scale) {
        VideoTransform t = clip.transform;
        t.scale = scale;
        largest = std::max(largest, geometry::effectiveScale(t, area));
    };
    if (clip.keys.isEmpty()) consider(clip.transform.scale);
    for (const LayerKey& k : clip.keys)
        consider(k.scale);
    const double scale = std::min(1.0, largest);
    QSize working(std::max(16, int(std::lround(display.width() * scale))),
                  std::max(16, int(std::lround(display.height() * scale))));
    if (std::max(working.width(), working.height()) > 1920) working = working.scaled(1920, 1920, Qt::KeepAspectRatio);
    return QSize(working.width() & ~1, working.height() & ~1);
}

const media::MediaInfo* SequenceRenderer::infoOf(const QString& path) {
    if (const auto it = infos_.constFind(path); it != infos_.constEnd()) return &it.value();
    if (failed_.contains(path)) return nullptr;
    const media::MediaProbe::Result probed = media::MediaProbe::probe(path);
    if (!probed.info) {
        failed_.insert(path, true);
        return nullptr;
    }
    return &infos_.insert(path, *probed.info).value();
}

int SequenceRenderer::sourceFor(const Clip& clip, int lane, bool hardware) {
    const media::MediaInfo* info = infoOf(clip.path);
    if (!info) return -1;
    const QSize working = workingSize(clip, QSizeF(info->width, info->height), area_);
    for (size_t i = 0; i < sources_.size(); ++i)
        if (sources_[i].path == clip.path && sources_[i].working == working && sources_[i].lane == lane) {
            sources_[i].used = ++clock_;
            return int(i);
        }
    size_t slot = sources_.size();
    if (sources_.size() >= size_t(kMaxDecoders)) {
        // The least recently used goes, its frames with it.
        slot = size_t(std::min_element(sources_.begin(), sources_.end(),
                                       [](const Source& a, const Source& b) { return a.used < b.used; }) -
                      sources_.begin());
        for (Recent& r : recent_)
            if (r.source == int(slot)) r = Recent{};
    } else {
        sources_.emplace_back();
    }
    Source& s = sources_[slot];
    media::Decoder::Options options;
    options.workingSize = working;
    options.hardware = hardware;
    // The tracks above V1: software, two threads — each thread holds frames, and a crossfade doubles the decoders.
    options.threads = hardware ? 0 : 2;
    s.path = clip.path;
    s.working = working;
    s.lane = lane;
    s.used = ++clock_;
    s.decoder = std::make_unique<media::Decoder>(*info, options);
    if (!s.decoder->open()) {
        s.decoder.reset();
        failed_.insert(clip.path, true);
        return -1;
    }
    return int(slot);
}

media::Frame SequenceRenderer::frameOf(const Clip& clip, double own, int lane, bool hardware) {
    const int source = sourceFor(clip, lane, hardware);
    if (source < 0) return {};
    const ClipEffects effects = clip.effects.clamped();
    const std::uint64_t look = effects.key();
    for (const Recent& r : recent_)
        if (r.source == source && r.look == look && std::abs(r.seconds - own) < kSameFrame) return r.frame;
    const media::MediaInfo& info = *infoOf(clip.path);
    const Source& s = sources_[size_t(source)];
    // The frame starting at `own` (a hair later, so rounding to the stream's time base cannot fall short), its effects
    // at its working size.
    media::Frame f = s.decoder->frameAt(own + kSameFrame);
    if (!f.isNull()) f = effects_.apply(effects, f, info.width > 0 ? double(s.working.width()) / info.width : 1.0);
    recent_[recentNext_] = {source, own, look, f};
    recentNext_ = (recentNext_ + 1) % 8;
    return f;
}

const FrameTimes& SequenceRenderer::timesOf(const Clip& clip) {
    if (const auto it = times_.constFind(clip.id); it != times_.constEnd()) return it.value();
    const media::MediaInfo& info = *infoOf(clip.path);
    const Rational rate = info.avgFps.isValid() ? info.avgFps : info.realFps;
    FrameTimes times;
    if (info.vfr) {
        // A variable rate by its packets, over the part of the clip In/Out shows (spec §8.3).
        const double from = clip.range.in + std::max(0.0, sequence_.inOut.in - clip.start);
        const double to = clip.range.in + std::max(0.0, sequence_.inOut.out - clip.start) + 1.0;
        times = FrameTimes::variable(media::frameTimes(info, from, std::min(to, clip.range.out + 1.0)));
    }
    if (!info.vfr || times.count() == 0) {
        const auto frames =
            info.frameCount > 0 ? info.frameCount : std::int64_t(std::ceil(info.duration * rate.value()));
        times = FrameTimes::constant(rate, std::max<std::int64_t>(1, frames));
    }
    return times_.insert(clip.id, times).value();
}

std::shared_ptr<const std::vector<float>> SequenceRenderer::maskOf(const Clip& clip, const QRectF& rect) {
    if (!maskshapes::anyVisible(clip.masks)) return {};
    for (const MaskEntry& e : masks_)
        if (e.id == clip.id && e.rect == rect) return e.coverage;
    if (masks_.size() >= kMaxMasks) masks_.erase(masks_.begin());
    auto coverage = std::make_shared<const std::vector<float>>(
        MaskBuilder::shapesCoverage(placed(clip.masks, rect, clip.size), area_, size_));
    masks_.push_back({clip.id, rect, coverage});
    return coverage;
}

bool SequenceRenderer::gather(const Term& term, QVector<Drawn>& drawn, QString* error) {
    drawn.clear();
    for (qsizetype track = 0; track < sequence_.tracks.size(); ++track) {
        const Track& tr = sequence_.tracks[track];
        if (!tr.visible) continue;
        for (const Clip& clip : tr.clips) {
            double own = 0;
            if (!clip.ownTime(term.t, &own)) continue;
            const VideoLayer::Look look = clip.look(own);
            if (look.opacity <= 0 || clip.size.isEmpty()) break;
            Drawn d;
            d.opacity = float(look.opacity / 100.0);
            if (clip.id == motionClip_ && motion_) {
                media::Frame f = motion_(term.index);
                if (f.isNull()) {
                    if (error) *error = QStringLiteral("the Motion Interpolation pre-pass has not run");
                    return false;
                }
                f = effects_.apply(clip.effects.clamped(), f, clip.size.width() > 0 ? f.width / clip.size.width() : 1);
                d.pieces.append({f, 1.f, Placement::of(area_, size_, look.transform, clip.size)});
            } else {
                // The FPS method by the clip's own frame times (Motion beyond its pre-pass's clip: blended).
                const auto method =
                    method_ == LoopSettings::Method::Motion ? LoopSettings::Method::FrameBlend : method_;
                const SourceMix mix = timemap::sources(method, own, timesOf(clip));
                for (int m = 0; m < mix.count; ++m) {
                    const media::Frame f = frameOf(clip, mix.seconds[m], term.lane, track == 0);
                    if (f.isNull()) continue; // media offline or past its end: nothing drawn (as AvLoop's layers)
                    const VideoTransform at = clip.look(mix.seconds[m]).transform;
                    d.pieces.append({f, float(mix.weight[m]), Placement::of(area_, size_, at, clip.size)});
                }
            }
            d.mask = maskOf(clip, look.transform.rect(clip.size));
            if (!d.pieces.isEmpty()) drawn.append(d);
            break; // one clip a track at a time
        }
    }
    return true;
}

bool SequenceRenderer::render(const QList<Term>& terms, QImage& out, QString* error) {
    QVector<QVector<Drawn>> all(terms.size());
    for (qsizetype j = 0; j < terms.size(); ++j)
        if (!gather(terms[j], all[j], error)) return false;
    if (out.size() != size_ || out.format() != QImage::Format_RGBA8888_Premultiplied)
        out = QImage(size_, QImage::Format_RGBA8888_Premultiplied);
    const int width = size_.width();
    const float bg[3] = {float(background_.redF()), float(background_.greenF()), float(background_.blueF())};
    QtConcurrent::blockingMap(rows_, [&](int y) {
        std::uint8_t* row = out.scanLine(y);
        const uchar* matteRow = matte_.isNull() ? nullptr : matte_.constScanLine(y);
        for (int x = 0; x < width; ++x) {
            float acc[4] = {0, 0, 0, 0};
            for (qsizetype j = 0; j < terms.size(); ++j) {
                const auto w = float(terms[j].weight);
                if (w <= 0) continue;
                float c[4] = {0, 0, 0, 0};
                for (const Drawn& d : all[j]) {
                    float s[4] = {0, 0, 0, 0};
                    for (const Piece& p : d.pieces) {
                        float t[4];
                        sampleLinear(p.frame, p.place.ax * (x + 0.5) + p.place.bx, p.place.ay * (y + 0.5) + p.place.by,
                                     t);
                        for (int k = 0; k < 4; ++k)
                            s[k] += p.weight * t[k];
                    }
                    const float k = d.opacity * (d.mask ? (*d.mask)[size_t(y) * width + x] : 1.f);
                    const float a = s[3] * k;
                    for (int ch = 0; ch < 4; ++ch)
                        c[ch] = s[ch] * k + c[ch] * (1 - a);
                }
                for (int ch = 0; ch < 4; ++ch)
                    acc[ch] += w * c[ch];
            }
            if (matteRow) {
                const float m = matteRow[x] / 255.f;
                for (float& ch : acc)
                    ch *= m;
            }
            if (!alpha_) {
                for (int ch = 0; ch < 3; ++ch)
                    acc[ch] += bg[ch] * (1 - acc[3]);
                acc[3] = 1;
            }
            std::uint8_t* p = row + std::ptrdiff_t(x) * 4;
            for (int ch = 0; ch < 4; ++ch)
                p[ch] = unorm(acc[ch]);
        }
    });
    return true;
}

} // namespace avloop::render
