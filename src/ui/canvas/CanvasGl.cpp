// The canvas's OpenGL 3.3 drawing (spec §3.6; AVADR-0001): the scene's background, a checkerboard under the video
// where it is transparent, the video as a mipmapped premultiplied texture through the shared transform shader. Without
// OpenGL 3.3 the same picture is drawn by QPainter.
#include "core/Project.h"
#include "render/Shaders.h"
#include "ui/canvas/Canvas.h"
#include "ui/canvas/CanvasGlState.h"
#include "ui/theme/Theme.h"

#include <QDebug>
#include <QImage>
#include <QMatrix4x4>
#include <QOpenGLBuffer>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLPixelTransferOptions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <QPainter>

#include <array>

namespace avloop {

namespace {

// The transparency checkerboard (the owner's mock: 16 px squares in two dark greys).
const QColor kCheckDark(0x2B, 0x2B, 0x2B);
const QColor kCheckLight(0x3A, 0x3A, 0x3A);

QVector4D rgba(const QColor& c) {
    return QVector4D(c.redF(), c.greenF(), c.blueF(), c.alphaF());
}

} // namespace

Canvas::~Canvas() {
    releaseGl();
}

bool Canvas::glReady() const {
    return gl_ && gl_->ok;
}

void Canvas::initializeGL() {
    gl_ = std::make_unique<Gl>();
    const QSurfaceFormat fmt = context()->format();
    const bool versionOk = fmt.majorVersion() > 3 || (fmt.majorVersion() == 3 && fmt.minorVersion() >= 3);
    if (!versionOk) {
        qWarning().noquote() << "[canvas] OpenGL" << fmt.majorVersion() << "." << fmt.minorVersion()
                             << "is below 3.3: the canvas draws with QPainter";
        return;
    }
    namespace sh = render::shaders;
    bool ok =
        gl_->texture.addShaderFromSourceCode(QOpenGLShader::Vertex, sh::transformVertex()) &&
        gl_->texture.addShaderFromSourceCode(QOpenGLShader::Fragment, sh::textureFragment()) && gl_->texture.link() &&
        gl_->checker.addShaderFromSourceCode(QOpenGLShader::Vertex, sh::transformVertex()) &&
        gl_->checker.addShaderFromSourceCode(QOpenGLShader::Fragment, sh::checkerFragment()) && gl_->checker.link();
    ok = ok && gl_->vao.create() && gl_->vbo.create();
    if (!ok) {
        qWarning().noquote() << "[canvas] shaders:" << gl_->texture.log() << gl_->checker.log();
        return;
    }
    gl_->vao.bind();
    gl_->vbo.bind();
    gl_->vbo.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    gl_->vbo.allocate(int(4 * 4 * sizeof(float)));
    for (QOpenGLShaderProgram* p : {&gl_->texture, &gl_->checker}) {
        p->bind();
        p->enableAttributeArray(0);
        p->enableAttributeArray(1);
        p->setAttributeBuffer(0, GL_FLOAT, 0, 2, 4 * sizeof(float));
        p->setAttributeBuffer(1, GL_FLOAT, 2 * sizeof(float), 2, 4 * sizeof(float));
        p->release();
    }
    gl_->vbo.release();
    gl_->vao.release();
    gl_->ok = true;
    frameDirty_ = true;
}

void Canvas::resizeGL(int, int) {
    if (fitMode_) fitView();
}

void Canvas::releaseGl() {
    if (!gl_) return;
    makeCurrent();
    gl_.reset();
    doneCurrent();
}

void Canvas::paintGL() {
    QOpenGLFunctions* f = context()->functions();
    const QColor bg = Theme::instance().tokens().bg;
    f->glClearColor(bg.redF(), bg.greenF(), bg.blueF(), 1.0f);
    f->glClear(GL_COLOR_BUFFER_BIT);
    if (glReady())
        paintScene();
    else
        paintFallback();
    paintOverlay();
}

void Canvas::paintScene() {
    if (!project_->source().isValid()) return;
    QOpenGLFunctions* f = context()->functions();
    if (frameDirty_ && !frame_.isNull()) {
        if (!gl_->tex || gl_->tex->width() != frame_.width || gl_->tex->height() != frame_.height) {
            gl_->tex = std::make_unique<QOpenGLTexture>(QOpenGLTexture::Target2D);
            gl_->tex->setFormat(QOpenGLTexture::RGBA8_UNorm);
            gl_->tex->setSize(frame_.width, frame_.height);
            gl_->tex->setMipLevels(gl_->tex->maximumMipLevels());
            gl_->tex->allocateStorage(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8);
            gl_->tex->setWrapMode(QOpenGLTexture::ClampToEdge);
            gl_->tex->setMinMagFilters(QOpenGLTexture::LinearMipMapLinear, QOpenGLTexture::Linear);
        }
        QOpenGLPixelTransferOptions opts;
        opts.setRowLength(frame_.stride / 4);
        opts.setAlignment(4);
        gl_->tex->setData(0, QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, frame_.data.get(), &opts);
        gl_->tex->generateMipMaps();
        frameDirty_ = false;
    }
    const qreal dpr = devicePixelRatioF();
    f->glViewport(0, 0, int(width() * dpr), int(height() * dpr));
    QMatrix4x4 m; // scene → clip: the same mapping as sceneToView
    m.scale(float(2.0 * zoom_ / width()), float(-2.0 * zoom_ / height()));
    m.translate(float(-pan_.x()), float(-pan_.y()));

    const VideoTransform& t = project_->transform();
    const QRectF r = videoRect();
    const float u0 = t.flipH ? 1.f : 0.f, u1 = 1.f - u0, v0 = t.flipV ? 1.f : 0.f, v1 = 1.f - v0;
    const std::array<float, 16> quad = {
        float(r.left()), float(r.top()),    u0, v0, float(r.right()), float(r.top()),    u1, v0,
        float(r.left()), float(r.bottom()), u0, v1, float(r.right()), float(r.bottom()), u1, v1};
    gl_->vao.bind();
    gl_->vbo.bind();
    gl_->vbo.write(0, quad.data(), int(quad.size() * sizeof(float)));
    f->glDisable(GL_BLEND);
    gl_->checker.bind();
    gl_->checker.setUniformValue("uMatrix", m);
    gl_->checker.setUniformValue("uCell", float(8.0 * dpr));
    gl_->checker.setUniformValue("uDark", rgba(kCheckDark));
    gl_->checker.setUniformValue("uLight", rgba(kCheckLight));
    f->glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    gl_->checker.release();
    if (gl_->tex) {
        f->glEnable(GL_BLEND);
        f->glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // premultiplied
        gl_->texture.bind();
        gl_->texture.setUniformValue("uMatrix", m);
        gl_->texture.setUniformValue("uTex", 0);
        gl_->texture.setUniformValue("uOpacity", 1.0f);
        gl_->tex->bind(0);
        f->glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        gl_->tex->release(0);
        gl_->texture.release();
        f->glDisable(GL_BLEND);
    }
    gl_->vbo.release();
    gl_->vao.release();
}

void Canvas::paintFallback() {
    if (!project_->source().isValid() || frame_.isNull()) return;
    QPainter p(this);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    const VideoTransform& t = project_->transform();
    const QRectF target = sceneToView(videoRect());
    const QImage image(frame_.data.get(), frame_.width, frame_.height, frame_.stride,
                       QImage::Format_RGBA8888_Premultiplied);
    p.translate(target.center());
    p.scale(t.flipH ? -1 : 1, t.flipV ? -1 : 1);
    p.drawImage(QRectF(-target.width() / 2, -target.height() / 2, target.width(), target.height()), image);
}

} // namespace avloop
