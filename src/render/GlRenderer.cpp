#include "render/GlRenderer.h"

#include "media/SystemMemory.h"
#include "render/Shaders.h"

#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLVersionFunctionsFactory>

#include <algorithm>
#include <cstring>
#include <list>

namespace avloop::render {

namespace {

constexpr std::int64_t kPoolCap = 512LL << 20;
// Frames rarely come back (a turn, a repeated frame of a lower rate): the last two output frames' layers are kept, the
// byte budget only a ceiling.
constexpr std::size_t kPoolFrames = 8;

struct Texture {
    GLuint id = 0;
    std::int64_t key = 0; // the frame's pts
    QSize size;
};

} // namespace

struct GlRenderer::State {
    QOpenGLContext context;
    QOpenGLFunctions_3_3_Core* gl = nullptr;
    QOpenGLShaderProgram program;
    GLuint vao = 0;
    GLuint fbo = 0;
    GLuint target = 0; // the FBO's colour texture
    QSize size;
    GLuint pbo[2] = {0, 0};
    int next = 0;     // the PBO the next submit reads into
    int inFlight = 0; // frames submitted, not yet taken
    GLuint mask = 0;
    qint64 maskKey = -1;
    std::list<Texture> pool; // most recently used first
    std::int64_t poolBytes = 0;
    std::int64_t budget = kPoolCap;
    int uploads = 0;

    void release() {
        if (!gl) return;
        for (const Texture& t : pool)
            gl->glDeleteTextures(1, &t.id);
        pool.clear();
        poolBytes = 0;
        if (mask) gl->glDeleteTextures(1, &mask);
        if (target) gl->glDeleteTextures(1, &target);
        if (fbo) gl->glDeleteFramebuffers(1, &fbo);
        if (pbo[0]) gl->glDeleteBuffers(2, pbo);
        if (vao) gl->glDeleteVertexArrays(1, &vao);
        mask = target = fbo = vao = 0;
        pbo[0] = pbo[1] = 0;
        maskKey = -1;
        size = {};
        inFlight = 0;
    }

    // The frame's texture: from the pool when it was uploaded before, else uploaded (the oldest dropped over budget).
    GLuint texture(const media::Frame& f) {
        const QSize sz(f.width, f.height);
        for (auto it = pool.begin(); it != pool.end(); ++it)
            if (it->key == f.pts && it->size == sz) {
                pool.splice(pool.begin(), pool, it);
                return pool.front().id;
            }
        const std::int64_t bytes = std::int64_t(f.width) * f.height * 4;
        while (!pool.empty() && (pool.size() >= kPoolFrames || poolBytes + bytes > budget)) {
            gl->glDeleteTextures(1, &pool.back().id);
            poolBytes -= std::int64_t(pool.back().size.width()) * pool.back().size.height() * 4;
            pool.pop_back();
        }
        Texture t{0, f.pts, sz};
        gl->glGenTextures(1, &t.id);
        gl->glBindTexture(GL_TEXTURE_2D, t.id);
        gl->glPixelStorei(GL_UNPACK_ROW_LENGTH, f.stride / 4);
        gl->glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, f.width, f.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, f.data.get());
        gl->glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        const GLfloat clear[4] = {0, 0, 0, 0};
        gl->glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, clear);
        pool.push_front(t);
        poolBytes += bytes;
        ++uploads;
        return t.id;
    }

    bool resize(const QSize& s) {
        if (s == size) return true;
        if (target) gl->glDeleteTextures(1, &target);
        if (fbo) gl->glDeleteFramebuffers(1, &fbo);
        if (pbo[0]) gl->glDeleteBuffers(2, pbo);
        gl->glGenTextures(1, &target);
        gl->glBindTexture(GL_TEXTURE_2D, target);
        gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, s.width(), s.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        gl->glGenFramebuffers(1, &fbo);
        gl->glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        gl->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, target, 0);
        const bool complete = gl->glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
        gl->glGenBuffers(2, pbo);
        for (GLuint b : pbo) {
            gl->glBindBuffer(GL_PIXEL_PACK_BUFFER, b);
            gl->glBufferData(GL_PIXEL_PACK_BUFFER, GLsizeiptr(s.width()) * s.height() * 4, nullptr, GL_STREAM_READ);
        }
        gl->glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
        size = s;
        inFlight = 0;
        return complete;
    }
};

GlRenderer::GlRenderer() : surface_(std::make_unique<QOffscreenSurface>()), s_(std::make_unique<State>()) {
    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    surface_->setFormat(format);
    surface_->create();
}

GlRenderer::~GlRenderer() {
    if (s_->gl && s_->context.makeCurrent(surface_.get())) {
        s_->release();
        s_->context.doneCurrent();
    }
}

bool GlRenderer::init(QString* error) {
    auto fail = [error](const QString& why) {
        if (error) *error = why;
        return false;
    };
    s_->context.setFormat(surface_->requestedFormat());
    if (!surface_->isValid() || !s_->context.create() || !s_->context.makeCurrent(surface_.get()))
        return fail(QStringLiteral("no OpenGL context"));
    const QSurfaceFormat got = s_->context.format();
    if (got.version() < qMakePair(3, 3))
        return fail(QStringLiteral("OpenGL %1.%2, not 3.3").arg(got.majorVersion()).arg(got.minorVersion()));
    s_->gl = QOpenGLVersionFunctionsFactory::get<QOpenGLFunctions_3_3_Core>(&s_->context);
    if (!s_->gl || !s_->gl->initializeOpenGLFunctions()) return fail(QStringLiteral("no OpenGL 3.3 core functions"));
    if (!s_->program.addShaderFromSourceCode(QOpenGLShader::Vertex, shaders::outputVertex()) ||
        !s_->program.addShaderFromSourceCode(QOpenGLShader::Fragment, shaders::outputFragment()) || !s_->program.link())
        return fail(QStringLiteral("shaders: %1").arg(s_->program.log()));
    s_->gl->glGenVertexArrays(1, &s_->vao);
    const std::int64_t vram = media::SystemMemory::adapterMemory();
    s_->budget = vram > 0 ? std::min(kPoolCap, vram / 4) : kPoolCap;
    return true;
}

bool GlRenderer::ready() const {
    return s_->gl && s_->context.isValid();
}

QString GlRenderer::name() const {
    if (!ready()) return {};
    return QString::fromLatin1(reinterpret_cast<const char*>(s_->gl->glGetString(GL_RENDERER)));
}

bool GlRenderer::submit(const Composite& c) {
    State& s = *s_;
    if (!ready() || !s.context.makeCurrent(surface_.get()) || c.layers.size() > 4) return false;
    if (!s.resize(c.size)) return false;
    QOpenGLFunctions_3_3_Core* gl = s.gl;
    gl->glBindFramebuffer(GL_FRAMEBUFFER, s.fbo);
    gl->glViewport(0, 0, c.size.width(), c.size.height());
    gl->glDisable(GL_BLEND);
    gl->glClearColor(0, 0, 0, 0);
    gl->glClear(GL_COLOR_BUFFER_BIT);
    s.program.bind();
    for (int i = 0; i < int(c.layers.size()); ++i) {
        const Layer& l = c.layers[i];
        gl->glActiveTexture(GL_TEXTURE0 + i);
        gl->glBindTexture(GL_TEXTURE_2D, s.texture(l.frame));
        s.program.setUniformValue(QStringLiteral("uTex%1").arg(i).toLatin1().constData(), i);
        s.program.setUniformValue(
            QStringLiteral("uPlace[%1]").arg(i).toLatin1().constData(),
            QVector4D(float(l.place.ax), float(l.place.bx), float(l.place.ay), float(l.place.by)));
        s.program.setUniformValue(QStringLiteral("uWeight[%1]").arg(i).toLatin1().constData(), float(l.weight));
    }
    s.program.setUniformValue("uCount", int(c.layers.size()));
    s.program.setUniformValue("uHasMask", c.mask != nullptr);
    if (c.mask) {
        gl->glActiveTexture(GL_TEXTURE4);
        if (!s.mask) gl->glGenTextures(1, &s.mask);
        gl->glBindTexture(GL_TEXTURE_2D, s.mask);
        if (s.maskKey != c.mask->cacheKey()) {
            gl->glPixelStorei(GL_UNPACK_ROW_LENGTH, int(c.mask->bytesPerLine()));
            gl->glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, c.mask->width(), c.mask->height(), 0, GL_RED, GL_UNSIGNED_BYTE,
                             c.mask->constBits());
            gl->glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
            gl->glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
            gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            s.maskKey = c.mask->cacheKey();
        }
        s.program.setUniformValue("uMask", 4);
    }
    s.program.setUniformValue("uOpaque", !c.alpha);
    s.program.setUniformValue("uBackground", QVector3D(float(c.background.redF()), float(c.background.greenF()),
                                                       float(c.background.blueF())));
    gl->glBindVertexArray(s.vao);
    gl->glDrawArrays(GL_TRIANGLES, 0, 3);
    // Read back into a PBO: the copy runs while the next frame is drawn.
    gl->glBindBuffer(GL_PIXEL_PACK_BUFFER, s.pbo[s.next]);
    gl->glPixelStorei(GL_PACK_ALIGNMENT, 4);
    gl->glReadPixels(0, 0, c.size.width(), c.size.height(), GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    gl->glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    s.next ^= 1;
    s.inFlight = std::min(2, s.inFlight + 1);
    return gl->glGetError() == GL_NO_ERROR;
}

bool GlRenderer::take(std::uint8_t* out, int stride) {
    State& s = *s_;
    if (!ready() || s.inFlight == 0 || !s.context.makeCurrent(surface_.get())) return false;
    // The oldest in flight: two back when two are, else the one just read.
    const int which = s.inFlight == 2 ? s.next : (s.next ^ 1);
    QOpenGLFunctions_3_3_Core* gl = s.gl;
    gl->glBindBuffer(GL_PIXEL_PACK_BUFFER, s.pbo[which]);
    const auto* data = static_cast<const std::uint8_t*>(gl->glMapBufferRange(
        GL_PIXEL_PACK_BUFFER, 0, GLsizeiptr(s.size.width()) * s.size.height() * 4, GL_MAP_READ_BIT));
    bool ok = data != nullptr;
    if (ok)
        for (int y = 0; y < s.size.height(); ++y)
            std::memcpy(out + std::ptrdiff_t(y) * stride, data + std::ptrdiff_t(y) * s.size.width() * 4,
                        std::size_t(s.size.width()) * 4);
    if (data) gl->glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
    gl->glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    --s.inFlight;
    return ok;
}

bool GlRenderer::render(const Composite& c, std::uint8_t* out, int stride) {
    // Nothing older may be waiting: one frame in, the same frame out.
    while (s_->inFlight > 0) {
        std::vector<std::uint8_t> drop(std::size_t(s_->size.width()) * s_->size.height() * 4);
        take(drop.data(), s_->size.width() * 4);
    }
    return submit(c) && take(out, stride);
}

bool GlRenderer::recover() {
    if (s_->context.isValid()) return true;
    s_->gl = nullptr;
    s_->pool.clear();
    s_->poolBytes = 0;
    s_->maskKey = -1;
    s_->size = {};
    s_->mask = s_->target = s_->fbo = s_->vao = 0;
    s_->pbo[0] = s_->pbo[1] = 0;
    s_->program.removeAllShaders();
    return init();
}

std::int64_t GlRenderer::textureBytes() const {
    return s_->poolBytes;
}

std::int64_t GlRenderer::textureBudget() const {
    return s_->budget;
}

int GlRenderer::uploads() const {
    return s_->uploads;
}

} // namespace avloop::render
