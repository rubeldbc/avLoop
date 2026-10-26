#pragma once

// The canvas's OpenGL objects (CanvasGl.cpp), complete wherever the canvas is made or destroyed.

#include "ui/canvas/Canvas.h"

#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>

#include <cstdint>
#include <memory>

namespace avloop {

struct Canvas::Gl {
    bool ok = false;
    QOpenGLShaderProgram texture;
    QOpenGLShaderProgram checker;
    QOpenGLVertexArrayObject vao;
    QOpenGLBuffer vbo{QOpenGLBuffer::VertexBuffer};
    std::unique_ptr<QOpenGLTexture> tex;
    std::unique_ptr<QOpenGLTexture> lut; // ClipEffects::colour as a 3D table
    std::uint64_t lutKey = 0;
};

} // namespace avloop
