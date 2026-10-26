#pragma once

#include <QRectF>
#include <QSizeF>
#include <QString>
#include <QStringList>

namespace avloop {

// The crop's aspect ratios (spec §6.1): Free, Original (the video's), 1:1, 4:3, 3:4, 16:9, 9:16, 21:9, 3:2, 2:3, 4:5
// and Custom W:H. A ratio is written as its id: "free", "original" or "W:H" (a W:H outside the list is a Custom one).
namespace aspect {

// The ids of the list, in order (Custom is any other "W:H").
const QStringList& presets();
bool isCustom(const QString& id);
// "W:H" of two numbers; empty when either is not positive.
QString custom(double w, double h);
// A size's ratio in lowest terms, rounded to whole px: 180 × 320 → "9:16". Empty for an empty size.
QString reduced(double w, double h);
// The ratio w/h an id stands for: 0 for Free; the video's for Original.
double value(const QString& id, const QSizeF& video);
// The rectangle with the ratio, the same centre and an area as close as possible (spec §6.1).
QRectF apply(const QRectF& rect, double ratio);

} // namespace aspect
} // namespace avloop
