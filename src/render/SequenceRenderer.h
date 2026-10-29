#pragma once

#include "core/LoopSettings.h"
#include "core/Sequence.h"
#include "core/TimeMap.h"
#include "media/MediaInfo.h"
#include "render/EffectsPass.h"
#include "render/RenderJob.h"

#include <QColor>
#include <QHash>
#include <QImage>
#include <QList>
#include <QRectF>
#include <QSize>
#include <QString>
#include <QVector>

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

namespace avloop::media {
class Decoder;
} // namespace avloop::media

namespace avloop::render {

// The composite of a sequence (D-032; AVADR-0001's maths, on the CPU): at a sequence time every shown track bottom to
// top, the clip present → its media's frames at its own time (the FPS method by its own frame times; one decoder a
// file, size and lane, at most 8 open, AVADR-0002) → its clip effects → placed by its keyed motion, at its keyed
// opacity, inside its masks → over: acc = L·k + acc·(1 − αL·k). The loop's blend sums whole composites by weight; the
// matte (AvLoop's crop) over everything; with Alpha Off the background under last; rounded once.
class SequenceRenderer {
public:
    static constexpr int kMaxDecoders = 8;

    // A sequence time and its share of the output frame (the loop's blend), its segment frame (Motion's) and its lane
    // (the loop's partner frames walk forward on a decoder of their own).
    struct Term {
        double t = 0;
        double weight = 1;
        int index = 0;
        int lane = 0;
    };

    // `area`: the sequence's frame (its px) that becomes the output of `outputSize`.
    SequenceRenderer(const Sequence& sequence, const QRectF& area, const QSize& outputSize, LoopSettings::Method method,
                     bool alpha, const QColor& background);
    ~SequenceRenderer();

    // A clip whose frames come from elsewhere by segment frame — Motion Interpolation's pre-pass (its effects not yet
    // applied).
    void setMotion(int clipId, std::function<media::Frame(int)> frameOf);
    // The output frame of `terms` into `out` (premultiplied RGBA8 of the output size).
    bool render(const QList<Term>& terms, QImage& out, QString* error = nullptr);
    int decodersOpen() const;
    // One clip on V1 at its media's own time (start 0, In 0), on, without masks, at 100 % throughout: AvLoop's one
    // video — the FrameRenderer's own path draws it (the GPU back end), the same maths (RS-10).
    static bool plain(const Sequence& sequence);
    // A clip's decode size: its media's display size × min(1, its largest effective scale), long side ≤ 1920, even.
    static QSize workingSize(const Clip& clip, const QSizeF& display, const QRectF& area);

private:
    struct Source {
        QString path;
        QSize working;
        int lane = 0;
        std::unique_ptr<media::Decoder> decoder;
        std::int64_t used = 0;
    };
    struct Piece {
        media::Frame frame;
        float weight = 1;
        Placement place;
    };
    struct Drawn {
        QVector<Piece> pieces;
        float opacity = 1;
        std::shared_ptr<const std::vector<float>> mask;
    };
    struct MaskEntry {
        int id = 0;
        QRectF rect;
        std::shared_ptr<const std::vector<float>> coverage;
    };
    struct Recent {
        int source = -1;
        double seconds = -1;
        std::uint64_t look = 0;
        media::Frame frame;
    };

    const media::MediaInfo* infoOf(const QString& path);
    int sourceFor(const Clip& clip, int lane, bool hardware);
    media::Frame frameOf(const Clip& clip, double own, int lane, bool hardware);
    const FrameTimes& timesOf(const Clip& clip);
    std::shared_ptr<const std::vector<float>> maskOf(const Clip& clip, double own, const QRectF& rect);
    bool gather(const Term& term, QVector<Drawn>& drawn, QString* error);

    Sequence sequence_;
    QRectF area_;
    QSize size_;
    LoopSettings::Method method_;
    bool alpha_;
    QColor background_;
    QImage matte_;
    QHash<QString, media::MediaInfo> infos_;
    QHash<QString, bool> failed_;
    std::vector<Source> sources_;
    std::int64_t clock_ = 0;
    QHash<int, FrameTimes> times_;
    std::vector<MaskEntry> masks_; // the last few clips' masks at their places (at most 4)
    Recent recent_[8];
    int recentNext_ = 0;
    EffectsPass effects_;
    int motionClip_ = 0;
    std::function<media::Frame(int)> motion_;
    QVector<int> rows_;
};

} // namespace avloop::render
