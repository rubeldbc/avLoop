#pragma once

// The canvas's OpenGL objects (CanvasGl.cpp), complete wherever the canvas is made or destroyed.

#include "ui/canvas/Canvas.h"

#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>

#include <memory>

namespace avloop {

struct Canvas::Gl {
    bool ok = false;
    QOpenGLShaderProgram texture;
    QOpenGLShaderProgram checker;
    QOpenGLVertexArrayObject vao;
    QOpenGLBuffer vbo{QOpenGLBuffer::VertexBuffer};
    std::unique_ptr<QOpenGLTexture> tex;
};

} // namespace avloop
