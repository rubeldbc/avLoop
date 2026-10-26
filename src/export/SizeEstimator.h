#pragma once

#include "render/RenderJob.h"

#include <QList>
#include <QObject>
#include <QString>
#include <QTimer>

#include <cstdint>
#include <memory>

namespace avloop::media {
class DiskCache;
}
namespace avloop::render {
class RenderService;
}

namespace avloop::exporting {

// The size an export will have (spec §10.3): three parts of the loop spread across it, ten frames each, made on the
// render thread and encoded alone on the export thread with the job's settings, then extrapolated — one keyframe, the
// other frames at the parts' mean. Measured again 1 s after the job last changed; Target size is its own estimate.
class SizeEstimator : public QObject {
    Q_OBJECT
public:
    struct Part {
        int frames = 0;
        std::int64_t bytes = 0;
        std::int64_t keyBytes = 0;
    };

    SizeEstimator(render::RenderService* service, QObject* encodeContext, media::DiskCache* disk = nullptr,
                  QObject* parent = nullptr);
    ~SizeEstimator() override;

    void request(const render::RenderJob& job); // measured 1 s later
    void clear();
    bool busy() const { return busy_; }
    std::int64_t bytes() const { return bytes_; } // -1: none yet
    QString text() const;                         // "~1.8 MB"; "…" while measuring; "—" with none

    static QList<int> partStarts(int length); // where the parts begin
    static std::int64_t extrapolate(const QList<Part>& parts, int length);
    static QString sizeText(std::int64_t bytes); // "1.8 MB", "0.28 MB"

signals:
    void changed();

private:
    void measure();

    render::RenderService* service_;
    QObject* encodeContext_;
    media::DiskCache* disk_;
    QTimer timer_;
    render::RenderJob job_;
    bool busy_ = false;
    std::int64_t bytes_ = -1;
    std::shared_ptr<int> generation_;
};

} // namespace avloop::exporting
