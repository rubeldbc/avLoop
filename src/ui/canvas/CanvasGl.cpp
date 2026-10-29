// The canvas's OpenGL 3.3 drawing (spec §3.6; AVADR-0001): the scene's background, a checkerboard under the video
// where it is transparent, the video as a mipmapped premultiplied texture through the shared transform shader, the
// clip effects on it (D-017: the colour by a 3D table of ClipEffects::colour). Without OpenGL 3.3 the same picture is
// drawn by QPainter, the effects made on the CPU.
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

#include <QVector2D>

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace avloop {

namespace {

// The transparency checkerboard (the owner's mock: 16 px squares in two dark greys).
const QColor kCheckDark(0x2B, 0x2B, 0x2B);
const QColor kCheckLight(0x3A, 0x3A, 0x3A);

QVector4D rgba(const QColor& c) {
    return QVector4D(c.redF(), c.greenF(), c.blueF(), c.alphaF());
}

constexpr int kLut = 64; // the colour table's points along each axis

// ClipEffects::colour on a kLut³ grid: red along x, green along y, blue along z.
std::vector<float> colourTable(const ClipEffects& e) {
    const effects::Colour colour(e);
    std::vector<float> t(std::size_t(kLut) * kLut * kLut * 4);
    std::size_t i = 0;
    for (int b = 0; b < kLut; ++b)
        for (int g = 0; g < kLut; ++g)
            for (int r = 0; r < kLut; ++r) {
                float rgb[3] = {float(r) / (kLut - 1), float(g) / (kLut - 1), float(b) / (kLut - 1)};
                colour(rgb);
                t[i++] = rgb[0];
                t[i++] = rgb[1];
                t[i++] = rgb[2];
                t[i++] = 1;
            }
    return t;
}

// What of the effects the table holds: a change to the others leaves it as it is.
std::uint64_t colourKey(const ClipEffects& e) {
    ClipEffects c = e;
    c.posterize = 0;
    c.vignette = 0;
    c.opacity = 100;
    c.sharpen = 0;
    c.blur = 0;
    return c.key();
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
    if (!project_->source().isValid() || videoHidden_) return;
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
        setEffectUniforms();
        gl_->tex->bind(0);
        f->glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        gl_->tex->release(0);
        if (gl_->lut) gl_->lut->release(1);
        gl_->texture.release();
        f->glDisable(GL_BLEND);
    }
    gl_->vbo.release();
    gl_->vao.release();
}

void Canvas::setEffectUniforms() {
    QOpenGLShaderProgram& p = gl_->texture;
    const ClipEffects& e = project_->effects();
    const bool on = !e.isNeutral();
    p.setUniformValue("uLut", 1); // a unit of its own even when unused: two sampler types never share one
    p.setUniformValue("uEffects", GLint(on));
    p.setUniformValue("uOpacity", on ? float(e.opacity / 100) : 1.0f);
    if (!on) return;
    const bool colour = e.changesColour();
    if (colour && (!gl_->lut || gl_->lutKey != colourKey(e))) {
        if (!gl_->lut) {
            gl_->lut = std::make_unique<QOpenGLTexture>(QOpenGLTexture::Target3D);
            gl_->lut->setFormat(QOpenGLTexture::RGBA16F);
            gl_->lut->setSize(kLut, kLut, kLut);
            gl_->lut->setMipLevels(1);
            gl_->lut->allocateStorage(QOpenGLTexture::RGBA, QOpenGLTexture::Float32);
            gl_->lut->setWrapMode(QOpenGLTexture::ClampToEdge);
            gl_->lut->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
        }
        const std::vector<float> table = colourTable(e);
        gl_->lut->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, table.data());
        gl_->lutKey = colourKey(e);
    }
    if (colour) gl_->lut->bind(1);
    p.setUniformValue("uColour", GLint(colour));
    p.setUniformValue("uLutSize", float(kLut));
    p.setUniformValue("uLevels", GLint(e.posterize));
    p.setUniformValue("uVignette", float(e.vignette / 100));
    p.setUniformValue("uVignetteInner", float(0.7 * e.vignetteSize / 100));
    p.setUniformValue("uTexel", QVector2D(1.f / float(frame_.width), 1.f / float(frame_.height)));
    // Blur and sharpen in the texture's pixels; sharpen's soft picture is of the blurred one: the two σ together.
    const double px = project_->source().width > 0 ? double(frame_.width) / project_->source().width : 1.0;
    const double blur = e.blurSigma() * px;
    const double sharp = std::max(0.5, ClipEffects::kSharpenSigma * px);
    p.setUniformValue("uBlur", float(blur));
    p.setUniformValue("uSoft", float(std::sqrt(blur * blur + sharp * sharp)));
    p.setUniformValue("uSharpen", float(e.sharpen / 100 * 2));
}

void Canvas::paintFallback() {
    if (!project_->source().isValid() || frame_.isNull() || videoHidden_) return;
    const ClipEffects& e = project_->effects();
    if (fallbackFrom_.data != frame_.data || fallbackKey_ != e.key()) {
        const double px = double(frame_.width) / std::max(1, project_->source().width);
        fallbackFrame_ = fallbackPass_.apply(e, frame_, px);
        fallbackFrom_ = frame_;
        fallbackKey_ = e.key();
    }
    const media::Frame& shown = fallbackFrame_;
    QPainter p(this);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    const VideoTransform& t = project_->transform();
    const QRectF target = sceneToView(videoRect());
    const QImage image(shown.data.get(), shown.width, shown.height, shown.stride,
                       QImage::Format_RGBA8888_Premultiplied);
    p.translate(target.center());
    p.scale(t.flipH ? -1 : 1, t.flipV ? -1 : 1);
    p.drawImage(QRectF(-target.width() / 2, -target.height() / 2, target.width(), target.height()), image);
}

} // namespace avloop
