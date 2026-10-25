#pragma once

#include "media/MediaInfo.h"

#include <QHash>
#include <QMutex>
#include <QString>

#include <cstdint>
#include <list>

namespace avloop::media {

class DiskCache;

// The RAM frame cache (spec §3.5, §3.7; AVADR-0002): an LRU of normalized frames bounded in bytes (1536 MB by default,
// 256–8192 in Settings). An entry marked to spill goes to the session's disk cache when evicted and comes back from
// there. Thread-safe.
class FrameCache {
public:
    struct Key {
        quint64 source = 0;
        int width = 0;
        int height = 0;
        std::int64_t pts = 0;
        friend bool operator==(const Key& a, const Key& b) {
            return a.source == b.source && a.width == b.width && a.height == b.height && a.pts == b.pts;
        }
    };

    static constexpr std::int64_t kDefaultBudget = 1536LL << 20;
    static constexpr std::int64_t kMinBudget = 256LL << 20;
    static constexpr std::int64_t kMaxBudget = 8192LL << 20;

    explicit FrameCache(std::int64_t budgetBytes = kDefaultBudget);
    ~FrameCache();
    FrameCache(const FrameCache&) = delete;
    FrameCache& operator=(const FrameCache&) = delete;

    Frame find(const Key& key);
    void insert(const Key& key, const Frame& frame, bool spill = false);
    void removeSource(quint64 source);
    void clear();

    std::int64_t used() const;
    std::int64_t budget() const;
    void setBudget(std::int64_t bytes);
    // Memory pressure (spec §3.7.6): the budget halved until restore().
    void halve();
    void restore();
    bool halved() const;
    int count() const;
    qint64 hits() const;
    qint64 misses() const;

    void setDiskCache(DiskCache* disk);
    // A source's identity: its path, size and modification time.
    static quint64 sourceId(const QString& path);

private:
    struct Entry {
        Key key;
        Frame frame;
        bool spill = false;
    };
    void evictTo(std::int64_t limit);
    static QString diskKey(const Key& key);

    mutable QMutex mutex_;
    std::list<Entry> lru_; // front = most recent
    QHash<Key, std::list<Entry>::iterator> index_;
    std::int64_t used_ = 0;
    std::int64_t budget_ = kDefaultBudget;
    std::int64_t fullBudget_ = kDefaultBudget;
    qint64 hits_ = 0;
    qint64 misses_ = 0;
    DiskCache* disk_ = nullptr;
};

size_t qHash(const FrameCache::Key& key, size_t seed = 0) noexcept;

} // namespace avloop::media
