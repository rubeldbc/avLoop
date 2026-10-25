#include "media/FrameCache.h"

#include "media/DiskCache.h"

#include <QFileInfo>
#include <QHashFunctions>

namespace avloop::media {

size_t qHash(const FrameCache::Key& key, size_t seed) noexcept {
    return qHashMulti(seed, key.source, key.width, key.height, key.pts);
}

FrameCache::FrameCache(std::int64_t budgetBytes) {
    setBudget(budgetBytes);
}

FrameCache::~FrameCache() = default;

Frame FrameCache::find(const Key& key) {
    QMutexLocker lock(&mutex_);
    if (auto it = index_.find(key); it != index_.end()) {
        lru_.splice(lru_.begin(), lru_, *it);
        ++hits_;
        return lru_.front().frame;
    }
    if (disk_) {
        Frame f = disk_->get(diskKey(key));
        if (!f.isNull()) {
            ++hits_;
            return f;
        }
    }
    ++misses_;
    return {};
}

void FrameCache::insert(const Key& key, const Frame& frame, bool spill) {
    if (frame.isNull()) return;
    QMutexLocker lock(&mutex_);
    if (frame.bytes() > budget_) return; // a frame larger than the whole budget is never kept
    if (auto it = index_.find(key); it != index_.end()) {
        used_ -= (*it)->frame.bytes();
        lru_.erase(*it);
        index_.erase(it);
    }
    evictTo(budget_ - frame.bytes());
    lru_.push_front(Entry{key, frame, spill});
    index_.insert(key, lru_.begin());
    used_ += frame.bytes();
}

void FrameCache::evictTo(std::int64_t limit) {
    while (used_ > limit && !lru_.empty()) {
        Entry& last = lru_.back();
        if (last.spill && disk_) disk_->put(diskKey(last.key), last.frame);
        used_ -= last.frame.bytes();
        index_.remove(last.key);
        lru_.pop_back();
    }
}

void FrameCache::removeSource(quint64 source) {
    QMutexLocker lock(&mutex_);
    for (auto it = lru_.begin(); it != lru_.end();) {
        if (it->key.source == source) {
            used_ -= it->frame.bytes();
            index_.remove(it->key);
            it = lru_.erase(it);
        } else {
            ++it;
        }
    }
}

void FrameCache::clear() {
    QMutexLocker lock(&mutex_);
    lru_.clear();
    index_.clear();
    used_ = 0;
}

std::int64_t FrameCache::used() const {
    QMutexLocker lock(&mutex_);
    return used_;
}

std::int64_t FrameCache::budget() const {
    QMutexLocker lock(&mutex_);
    return budget_;
}

void FrameCache::setBudget(std::int64_t bytes) {
    QMutexLocker lock(&mutex_);
    fullBudget_ = qBound(kMinBudget, bytes, kMaxBudget);
    budget_ = fullBudget_;
    evictTo(budget_);
}

void FrameCache::halve() {
    QMutexLocker lock(&mutex_);
    budget_ = fullBudget_ / 2;
    evictTo(budget_);
}

void FrameCache::restore() {
    QMutexLocker lock(&mutex_);
    budget_ = fullBudget_;
}

bool FrameCache::halved() const {
    QMutexLocker lock(&mutex_);
    return budget_ < fullBudget_;
}

int FrameCache::count() const {
    QMutexLocker lock(&mutex_);
    return int(lru_.size());
}

qint64 FrameCache::hits() const {
    QMutexLocker lock(&mutex_);
    return hits_;
}

qint64 FrameCache::misses() const {
    QMutexLocker lock(&mutex_);
    return misses_;
}

void FrameCache::setDiskCache(DiskCache* disk) {
    QMutexLocker lock(&mutex_);
    disk_ = disk;
}

quint64 FrameCache::sourceId(const QString& path) {
    const QFileInfo fi(path);
    return quint64(qHashMulti(0, fi.absoluteFilePath(), fi.size(), fi.lastModified().toMSecsSinceEpoch()));
}

QString FrameCache::diskKey(const Key& key) {
    return QStringLiteral("%1-%2x%3-%4")
        .arg(key.source, 16, 16, QLatin1Char('0'))
        .arg(key.width)
        .arg(key.height)
        .arg(key.pts);
}

} // namespace avloop::media
