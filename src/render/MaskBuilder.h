#pragma once

#include "core/CropShape.h"

#include <QImage>
#include <QSize>

namespace avloop::render {

// The crop's alpha mask at the output size (spec §6.1, §3.4 step 6; AVADR-0001): 255 inside, 0 outside, the edge
// anti-aliased over one pixel at feather 0 and softened inward over `feather` output pixels. Rect, rounded rect (the
// radius scaled to output px) and ellipse by their signed distance. Inverted (D-021), the shape is a hole in the
// export frame — the whole video — at its own place: 0 inside, 255 outside, the feather softening outward over
// `feather` px. Both renderers use this one mask; it is built once and kept until the crop, the frame or the size
// changes. With shapes (D-022) each is drawn at the output size in turn into one running coverage — Add, Cut, Keep
// only, Exclude by its role (`maskshapes::join`) — and the crop's own coverage multiplies the result; no shapes, the
// crop's mask byte for byte as before.
class MaskBuilder {
public:
    // One coverage value 0…1 at the pixel centre (x + ½, y + ½) of an output of `size`.
    static double coverage(CropShape::Kind kind, const QSize& size, double radius, double feather, int x, int y);
    // The whole mask (QImage::Format_Grayscale8).
    static QImage build(CropShape::Kind kind, const QSize& size, double radius, double feather);

    // The mask of a crop whose export frame is `frame` (scene px) at an output of `size`: the crop's own shape filling
    // the output, or, inverted, the shape a hole at its place in the frame.
    static QImage build(const CropShape& crop, const QRectF& frame, const QSize& size);
    // A shape's own coverage at the output size (QImage::Format_Grayscale8): its outline anti-aliased.
    static QImage shapeCoverage(const MaskShape& shape, const QRectF& frame, const QSize& size);

    // The mask from the cache when nothing changed.
    QImage mask(const CropShape& crop, const QRectF& frame, const QSize& outputSize);
    int builds() const { return builds_; }

private:
    CropShape crop_;
    QRectF frame_;
    QSize size_;
    QImage cached_;
    int builds_ = 0;
};

} // namespace avloop::render
