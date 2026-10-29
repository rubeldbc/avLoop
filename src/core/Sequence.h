#pragma once

#include "core/ClipEffects.h"
#include "core/CropShape.h"
#include "core/MaskShape.h"
#include "core/Rational.h"
#include "core/TimeRange.h"
#include "core/VideoLayer.h"
#include "core/VideoTransform.h"

#include <QJsonObject>
#include <QList>
#include <QRectF>
#include <QSizeF>
#include <QString>

namespace avloop {

struct ProjectData;

// A clip on a track (AvLoop Studio, D-031): its media, where it starts on the sequence, the part of it used, its motion
// and opacity — keyed over its own seconds (D-029's maths) —, its clip effects and its masks (AvLoop's shapes, in its
// own pixels: an Add keeps, a Cut makes a hole). Every clip is equal: there is no main video.
struct Clip {
    int id = 0;
    QString name;
    QString path;
    QSizeF size;         // its display size, px
    double duration = 0; // the media's length, s
    double start = 0;    // sequence seconds where its In shows
    TimeRange range;     // In/Out in its own seconds
    VideoTransform transform;
    QList<LayerKey> keys;
    double opacity = 100;
    ClipEffects effects;
    QList<MaskShape> masks;
    bool enabled = true;
    bool locked = false; // AvLoop's layer lock (D-027): the canvas and the lanes leave it alone

    double length() const { return range.out - range.in; }
    double end() const { return start + length(); }
    // Its own time at sequence time `t`; false when it does not show then.
    bool ownTime(double t, double* own) const;
    VideoLayer::Look look(double own) const;
    QRectF rectAt(double own) const { return look(own).transform.rect(size); }

    QJsonObject toJson() const;
    static Clip fromJson(const QJsonObject& json);
    friend bool operator==(const Clip&, const Clip&) = default;
};

// A video track (V1 the lowest): its clips sorted by start, never overlapping.
struct Track {
    QString name;
    bool visible = true;
    bool locked = false;
    QList<Clip> clips;
    friend bool operator==(const Track&, const Track&) = default;
};

// The sequence (D-031): its frame (the output's area, a rectangle of the scene — AvLoop's scene px, where the clips
// stand; D-033), its rate, its In/Out, its tracks bottom to top. At most 8 tracks and 64 clips (AVADR-0002).
struct Sequence {
    static constexpr int kMaxTracks = 8;
    static constexpr int kMaxClips = 64;

    QRectF frame{0, 0, 1920, 1080};
    Rational fps{30, 1};
    TimeRange inOut;
    QList<Track> tracks;
    // The matte (D-032): a shape over the whole composite in scene px — kept inside, or Inverted a hole — with its
    // shapes joined (AvLoop's crop, D-021 … D-024, as its MaskBuilder makes it); empty and no shapes: none.
    CropShape matte;
    int main = 0; // the clip AvLoop's panels act on (D-033): one of V1's, its media the project's source
    int nextId = 1;

    int clipCount() const;
    double duration() const; // the last clip's end
    QJsonObject toJson() const;
    static Sequence fromJson(const QJsonObject& json);
    // An AvLoop file (versions 1 … 3) as a sequence (RS-5): the main video → V1's clip, the main one (its transform,
    // keyframes as keys, effects), the layers → clips on V2 …; the crop's rectangle (else the video where it stands at
    // In) → the frame, the crop with its shapes → the matte (RS-10); the range → In/Out; all in the scene's px.
    static Sequence fromAvLoop(const ProjectData& d);
    friend bool operator==(const Sequence&, const Sequence&) = default;
};

} // namespace avloop
