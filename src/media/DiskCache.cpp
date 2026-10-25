#include "media/DiskCache.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QStandardPaths>

#include <windows.h>

#include <cstring>
#include <memory>

namespace avloop::media {

namespace {

constexpr char kMagic[4] = {'A', 'V', 'L', 'F'};

struct Header {
    char magic[4];
    std::int32_t width;
    std::int32_t height;
    std::int32_t stride;
    std::int64_t pts;
    double seconds;
};

// Is the process that made a session folder still the one running under that PID?
bool sessionAlive(const QString& name) {
    const QStringList parts = name.split(QLatin1Char('-'));
    if (parts.size() != 2) return false;
    const DWORD pid = parts[0].toULong();
    const qint64 stamp = parts[1].toLongLong();
    HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!h) return false;
    DWORD code = 0;
    FILETIME created{}, exited{}, kernel{}, user{};
    const bool running = GetExitCodeProcess(h, &code) && code == STILL_ACTIVE;
    const bool timed = GetProcessTimes(h, &created, &exited, &kernel, &user);
    CloseHandle(h);
    if (!running || !timed) return false;
    // FILETIME: 100 ns since 1601; the folder's stamp: ms since 1970. A reused PID started after the folder was made.
    const qint64 ticks = (qint64(created.dwHighDateTime) << 32) | created.dwLowDateTime;
    const qint64 createdMs = ticks / 10000 - 11644473600000LL;
    return createdMs <= stamp + 1000;
}

} // namespace

QString DiskCache::defaultRoot() {
    const QString local = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation);
    return QDir(local.isEmpty() ? QDir::tempPath() : local).filePath(QStringLiteral("AvLoop/cache"));
}

int DiskCache::removeStaleSessions(const QString& root) {
    int removed = 0;
    const QDir dir(root);
    for (const QString& name : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        if (sessionAlive(name)) continue;
        if (QDir(dir.filePath(name)).removeRecursively()) ++removed;
    }
    return removed;
}

DiskCache::DiskCache(const QString& root, std::int64_t budgetBytes) : budget_(budgetBytes) {
    const QString name =
        QStringLiteral("%1-%2").arg(QCoreApplication::applicationPid()).arg(QDateTime::currentMSecsSinceEpoch());
    dir_ = QDir(root).filePath(name);
    writable_ = QDir().mkpath(dir_);
}

DiskCache::~DiskCache() {
    QDir(dir_).removeRecursively();
}

bool DiskCache::put(const QString& key, const Frame& frame) {
    QMutexLocker lock(&mutex_);
    if (!writable_ || frame.isNull()) return false;
    const std::int64_t size = std::int64_t(sizeof(Header)) + frame.bytes();
    trimTo(budget_ - size);
    QFile file(QDir(dir_).filePath(key + QStringLiteral(".raw")));
    if (!file.open(QIODevice::WriteOnly)) {
        writable_ = false;
        return false;
    }
    Header h{};
    std::memcpy(h.magic, kMagic, 4);
    h.width = frame.width;
    h.height = frame.height;
    h.stride = frame.stride;
    h.pts = frame.pts;
    h.seconds = frame.seconds;
    const bool ok = file.write(reinterpret_cast<const char*>(&h), sizeof h) == qint64(sizeof h) &&
                    file.write(reinterpret_cast<const char*>(frame.data.get()), frame.bytes()) == frame.bytes();
    file.close();
    if (!ok) {
        file.remove();
        writable_ = false; // a full disk: RAM only from here (spec §12.3)
        return false;
    }
    used_ += size;
    return true;
}

Frame DiskCache::get(const QString& key) {
    QMutexLocker lock(&mutex_);
    QFile file(QDir(dir_).filePath(key + QStringLiteral(".raw")));
    if (!file.open(QIODevice::ReadOnly)) return {};
    Header h{};
    if (file.read(reinterpret_cast<char*>(&h), sizeof h) != qint64(sizeof h) || std::memcmp(h.magic, kMagic, 4) != 0)
        return {};
    const std::int64_t bytes = std::int64_t(h.stride) * h.height;
    std::shared_ptr<std::uint8_t[]> pixels;
    try {
        pixels = std::make_shared_for_overwrite<std::uint8_t[]>(size_t(bytes));
    } catch (const std::bad_alloc&) {
        return {}; // under memory pressure the frame is decoded again instead
    }
    if (file.read(reinterpret_cast<char*>(pixels.get()), bytes) != bytes) return {};
    Frame f;
    f.width = h.width;
    f.height = h.height;
    f.stride = h.stride;
    f.pts = h.pts;
    f.seconds = h.seconds;
    f.data = std::shared_ptr<const std::uint8_t>(pixels, pixels.get());
    return f;
}

void DiskCache::clear() {
    QMutexLocker lock(&mutex_);
    QDir dir(dir_);
    for (const QString& f : dir.entryList(QDir::Files))
        dir.remove(f);
    used_ = 0;
    writable_ = true;
}

std::int64_t DiskCache::used() const {
    QMutexLocker lock(&mutex_);
    return used_;
}

bool DiskCache::writable() const {
    QMutexLocker lock(&mutex_);
    return writable_;
}

void DiskCache::trimTo(std::int64_t limit) {
    if (used_ <= limit) return;
    QDir dir(dir_);
    // The oldest files first.
    const QFileInfoList files = dir.entryInfoList(QDir::Files, QDir::Time | QDir::Reversed);
    for (const QFileInfo& fi : files) {
        if (used_ <= limit) break;
        const qint64 size = fi.size();
        if (QFile::remove(fi.absoluteFilePath())) used_ -= size;
    }
}

} // namespace avloop::media
