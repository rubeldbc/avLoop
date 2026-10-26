#pragma once

#include "media/MediaInfo.h"

#include <QVector>

namespace avloop::media {

// The times of a stream's frames between two moments (spec §8.3: a variable rate is mapped by PTS, never by index),
// read from the packets alone — nothing decoded. Seconds from the stream's start, sorted, each once.
QVector<double> frameTimes(const MediaInfo& info, double from, double to);

} // namespace avloop::media
