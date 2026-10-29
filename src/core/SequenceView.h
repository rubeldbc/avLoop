#pragma once

#include "core/Keyframes.h"
#include "core/Sequence.h"

#include <QList>
#include <QRectF>

namespace avloop {

// AvLoop's model as a view of the sequence (D-033, RS-14): the main video is the main clip — on V1, its media the
// project's source —, its keyframes are the clip's keys over sequence time, its layers are the clips on V2 …, its crop
// is the matte. Each view has its way back, so AvLoop's panels edit the sequence itself. Pure.
namespace sequenceview {

// The main clip: `s.main` when it is on V1, else V1's first; null when V1 is empty.
const Clip* mainClip(const Sequence& s);
Clip* mainClip(Sequence& s);
// `s.main` made valid (the rule above), the frame made from the matte and the main clip (frameOf).
void settle(Sequence& s);

// A clip's keys as AvLoop's keyframes, their times on the sequence (start + own − In); back, each key's opacity kept
// where a key stays, else the clip's opacity there.
Keyframes keyframesOf(const Clip& c);
void setKeyframes(Clip& c, const Keyframes& keys);

// The clips on V2 … as AvLoop's layers (their ids, ordered by id: the order they were added in); back by id — kept,
// changed (a new track moves it), added (no id, or one seen twice), removed. A clip's masks stay with it.
QList<VideoLayer> layersOf(const Sequence& s);
void setLayers(Sequence& s, const QList<VideoLayer>& layers);

// The export frame (AvLoop's): the matte's rectangle, else the main clip where it stands at In, else the old frame.
QRectF frameOf(const Sequence& s);

} // namespace sequenceview

} // namespace avloop
