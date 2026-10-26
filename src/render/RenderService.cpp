#include "render/RenderService.h"

#include "media/WorkerThread.h"
#include "render/FrameRenderer.h"
#include "render/GlRenderer.h"

#include <QMetaObject>

namespace avloop::render {

RenderService::RenderService(QObject* parent) : QObject(parent) {
    thread_ = std::make_unique<media::WorkerThread>(QStringLiteral("avloop-render"));
    context_ = new QObject;
    context_->moveToThread(thread_.get());
    if (FrameRenderer::gpuSetting()) gl_ = std::make_unique<GlRenderer>(); // the surface: on the GUI thread
}

RenderService::~RenderService() {
    // The GPU objects and the context go on the thread that made them, then the thread; the offscreen surface last,
    // here on the GUI thread that made it.
    run([this] {
        if (gl_) gl_->shutdown();
    });
    context_->deleteLater();
    thread_.reset();
    gl_.reset();
}

void RenderService::trim() {
    post([this] {
        if (gl_) gl_->trim();
    });
}

void RenderService::post(std::function<void()> work) {
    QMetaObject::invokeMethod(context_, std::move(work));
}

void RenderService::run(std::function<void()> work) {
    QMetaObject::invokeMethod(context_, std::move(work), Qt::BlockingQueuedConnection);
}

} // namespace avloop::render
