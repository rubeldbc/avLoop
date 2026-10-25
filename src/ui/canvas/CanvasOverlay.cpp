// What is painted over the frame (spec §6.1, §6.2): the active tool's overlay — the Selection's video border and
// handles, the Zoom tool's rectangle — and, from AV-004, the crop's dim and outline.
#include "ui/canvas/Canvas.h"
#include "ui/tools/Tool.h"

#include <QPainter>

namespace avloop {

void Canvas::paintOverlay() {
    if (!tool_) return;
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    tool_->paintOverlay(p);
}

} // namespace avloop
