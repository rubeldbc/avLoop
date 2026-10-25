#pragma once

#include "media/MediaInfo.h"

#include <QMutex>
#include <QString>

#include <cstdint>

namespace avloop::media {

// The disk spill of the frame cache (spec §3.5; AVADR-0002): raw RGBA frames in
// %LOCALAPPDATA%\AvLoop\cache\<session>\, at most 20 GB, the oldest deleted first. The session folder is deleted when
// the cache goes; folders of sessions whose process is gone are deleted at the next start. A full disk stops the
// writing and the cache falls back to RAM (spec §12.3).
class DiskCache {
public:
    static constexpr std::int64_t kDefaultBudget = 20LL << 30;

    explicit DiskCache(const QString& root = defaultRoot(), std::int64_t budgetBytes = kDefaultBudget);
    ~DiskCache();
    DiskCache(const DiskCache&) = delete;
    DiskCache& operator=(const DiskCache&) = delete;

    static QString defaultRoot();
    // Deletes the folders of sessions whose process is no longer running; returns how many.
    static int removeStaleSessions(const QString& root = defaultRoot());

    bool put(const QString& key, const Frame& frame);
    Frame get(const QString& key);
    void clear();

    QString sessionDir() const { return dir_; }
    std::int64_t used() const;
    bool writable() const;

private:
    void trimTo(std::int64_t limit);

    mutable QMutex mutex_;
    QString dir_;
    std::int64_t budget_;
    std::int64_t used_ = 0;
    bool writable_ = true;
};

} // namespace avloop::media
