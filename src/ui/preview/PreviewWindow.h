#pragma once

#include "render/RenderJob.h"

#include <QWidget>

class QComboBox;
class QLabel;
class QPushButton;
class QToolButton;

namespace avloop::media {
class DiskCache;
}
namespace avloop::render {
class RenderService;
}

namespace avloop {

class PreviewPlayer;
class PreviewSlider;
class PreviewView;

// The Preview window (spec §9): non-modal, its own window, one at a time; shows only the exported part, exactly as it
// will be exported. Top: Live | Encoded, Background, Zoom and what it is doing (Caching…, Encoding n/L, Out of date,
// the encoded size). Middle: the frame. Bottom: Play, the slider with the seam, the time, Around seam; the info row.
// Space plays and pauses, ←/→ step a frame. Its position and size are remembered; closing it releases its player,
// its cache and the encoded temporary file.
class PreviewWindow : public QWidget {
    Q_OBJECT
public:
    PreviewWindow(render::RenderService* service, media::DiskCache* disk, std::int64_t cacheBytes,
                  QWidget* parent = nullptr);
    ~PreviewWindow() override;

    void setJob(const render::RenderJob& job);
    PreviewPlayer* player() const { return player_; }
    PreviewView* view() const { return view_; }
    QString info() const;
    QString status() const;

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void refresh();
    void startEncode();

    render::RenderService* service_;
    media::DiskCache* disk_;
    PreviewPlayer* player_ = nullptr;
    PreviewView* view_ = nullptr;
    PreviewSlider* slider_ = nullptr;
    QPushButton* live_ = nullptr;
    QPushButton* encoded_ = nullptr;
    QComboBox* background_ = nullptr;
    QComboBox* zoom_ = nullptr;
    QLabel* status_ = nullptr;
    QToolButton* play_ = nullptr;
    QLabel* time_ = nullptr;
    QToolButton* aroundSeam_ = nullptr;
    QLabel* info_ = nullptr;
    QString tempFile_;
    int encodes_ = 0;
};

} // namespace avloop
