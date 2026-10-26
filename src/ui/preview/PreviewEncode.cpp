// The Preview's Encoded mode (spec §9.3): the loop rendered and encoded with the current Output and Quality into a
// temporary WebM on the render thread (progress, cancelled by a newer job or the window's close), then that file's
// frames as the libvpx decoder gives them — alpha and compression as OBS will show them — played like Live's.
#include "media/Decoder.h"
#include "media/Encoder.h"
#include "media/MediaProbe.h"
#include "render/FrameRenderer.h"
#include "render/RenderService.h"
#include "ui/preview/PreviewPlayer.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>

namespace avloop {

void PreviewPlayer::encode(const QString& path) {
    if (cancelEncode_) *cancelEncode_ = true;
    cancelEncode_ = std::make_shared<std::atomic<bool>>(false);
    encodeState_ = Encode::Running;
    encodeDone_ = 0;
    encodedBytes_ = 0;
    encodedPath_ = path;
    error_.clear();
    auto renderer =
        std::make_shared<render::FrameRenderer>(job_, disk_, render::FrameRenderer::gpuSetting(), service_->gl());
    const render::RenderJob job = job_;
    auto cancel = cancelEncode_;
    std::weak_ptr<int> life = life_;
    emit stateChanged();
    service_->post([renderer, job, cancel, life, path, this] {
        QString error;
        bool ok = renderer->open(&error) && renderer->prepare({}, &error);
        media::EncodeSettings s;
        s.format = job.output.format;
        s.size = job.outputSize;
        s.fps = job.fps;
        s.frames = job.plan.length;
        s.alpha = job.alpha;
        s.quality = job.quality;
        media::Encoder encoder(s);
        ok = ok && encoder.open(path, &error);
        QImage image;
        for (int k = 0; ok && k < job.plan.length; ++k) {
            if (*cancel) {
                encoder.abort();
                return;
            }
            ok = renderer->render(k, image, &error) &&
                 encoder.write(image.constBits(), int(image.bytesPerLine()), &error);
            if (k % 5 == 4)
                QMetaObject::invokeMethod(qApp, [life, cancel, k, this] {
                    if (!life.lock() || *cancel) return;
                    encodeDone_ = k + 1;
                    emit stateChanged();
                });
        }
        if (ok)
            ok = encoder.finish(&error);
        else
            encoder.abort();
        QMetaObject::invokeMethod(qApp, [life, cancel, ok, error, path, this] {
            if (!life.lock() || *cancel) return; // the window removes the file when it closes
            if (!ok) {
                encodeState_ = Encode::Failed;
                error_ = error;
                emit stateChanged();
                return;
            }
            encodeState_ = Encode::Done;
            encodeDone_ = length();
            encodedBytes_ = QFileInfo(path).size();
            useEncoded();
            emit stateChanged();
        });
    });
}

void PreviewPlayer::useEncoded() {
    // The file read back by libvpx (alpha kept), frame k by its index — forward reads continue the stream.
    *encoded_ = Track{};
    encoded_->frames.resize(length());
    auto decoder = std::make_shared<std::unique_ptr<media::Decoder>>();
    encoded_->make = [decoder, path = encodedPath_](int k) {
        if (!*decoder) {
            const media::MediaProbe::Result probed = media::MediaProbe::probe(path);
            if (!probed.info) return QImage();
            media::Decoder::Options o;
            o.hardware = false;
            *decoder = std::make_unique<media::Decoder>(*probed.info, o);
            if (!(*decoder)->open()) {
                decoder->reset();
                return QImage();
            }
        }
        const media::Frame f = (*decoder)->frameAtIndex(k);
        if (f.isNull()) return QImage();
        return QImage(f.data.get(), f.width, f.height, f.stride, QImage::Format_RGBA8888_Premultiplied).copy();
    };
    if (mode_ == Mode::Encoded) {
        ++generation_;
        inFlight_ = false;
        ask();
    }
}

} // namespace avloop
