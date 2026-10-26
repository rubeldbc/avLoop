#pragma once

#include "render/CpuRenderer.h"

#include <QSize>
#include <QString>

#include <cstdint>
#include <memory>

class QOffscreenSurface;

namespace avloop::render {

// The GPU back end (spec §3.6, AVADR-0001): its own OpenGL 3.3 core context on an offscreen surface, an RGBA8 FBO of
// the output size, the one output pass of `Shaders.cpp`, textures from a pool (LRU, the last two frames' layers, never
// over 512 MB or a quarter of the adapter's memory), and the readback through two PBOs taking turns. Construct it on
// the GUI thread (the surface must be made there); init() and the rest on the thread that renders. Without OpenGL 3.3
// init() fails and the caller uses the CPU back end.
class GlRenderer {
public:
    GlRenderer();
    ~GlRenderer();
    GlRenderer(const GlRenderer&) = delete;
    GlRenderer& operator=(const GlRenderer&) = delete;

    bool init(QString* error = nullptr);
    bool ready() const;
    QString name() const; // GL_RENDERER

    // Draws the composite and starts reading it back; take() returns the oldest frame submitted (one may be in
    // flight while the next is drawn). render() is both for one frame.
    bool submit(const Composite& c);
    bool take(std::uint8_t* out, int stride);
    bool render(const Composite& c, std::uint8_t* out, int stride);
    // A lost context (driver reset): made again once; false when it cannot be.
    bool recover();

    std::int64_t textureBytes() const;
    std::int64_t textureBudget() const;
    int uploads() const; // textures uploaded so far (a frame used again is not uploaded again)

    struct State;

private:
    std::unique_ptr<QOffscreenSurface> surface_;
    std::unique_ptr<State> s_;
};

} // namespace avloop::render
