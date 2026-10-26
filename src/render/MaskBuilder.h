#pragma once

#include "core/CropShape.h"

#include <QImage>
#include <QSize>

namespace avloop::render {

// The crop's alpha mask at the output size (spec §6.1, §3.4 step 6; AVADR-0001): 255 inside, 0 outside, the edge
// anti-aliased over one pixel at feather 0 and softened inward over `feather` output pixels. Rect, rounded rect (the
// radius scaled to output px) and ellipse by their signed distance. Both renderers use this one mask; it is built once
// and kept until the crop or the size changes.
class MaskBuilder {
public:
    // One coverage value 0…1 at the pixel centre (x + ½, y + ½) of an output of `size`.
    static double coverage(CropShape::Kind kind, const QSize& size, double radius, double feather, int x, int y);
    // The whole mask (QImage::Format_Grayscale8).
    static QImage build(CropShape::Kind kind, const QSize& size, double radius, double feather);

    // The mask of a crop at an output size, from the cache when nothing changed.
    QImage mask(const CropShape& crop, const QSize& outputSize);
    int builds() const { return builds_; }

private:
    CropShape::Kind kind_ = CropShape::Kind::Rect;
    QSize size_;
    double radius_ = -1;
    double feather_ = -1;
    QImage cached_;
    int builds_ = 0;
};

} // namespace avloop::render
