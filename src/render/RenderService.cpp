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
    // The GPU objects go on the thread whose context made them, then the thread.
    run([this] { gl_.reset(); });
    context_->deleteLater();
    thread_.reset();
}

void RenderService::post(std::function<void()> work) {
    QMetaObject::invokeMethod(context_, std::move(work));
}

void RenderService::run(std::function<void()> work) {
    QMetaObject::invokeMethod(context_, std::move(work), Qt::BlockingQueuedConnection);
}

} // namespace avloop::render
