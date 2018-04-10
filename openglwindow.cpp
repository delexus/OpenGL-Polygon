/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the documentation of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "openglwindow.h"

#include <QtCore/QCoreApplication>

#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLPaintDevice>
#include <QtGui/QPainter>

static const char *vertexShaderSource =
    "attribute highp vec4 a_position;           \n"
    "attribute lowp vec2 a_texcoord;            \n"
    "varying lowp vec2 v_texcoord;              \n"
    "void main() {                              \n"
    "   gl_Position = a_position;               \n"
    "   v_texcoord = a_texcoord;                \n"
    "}                                          \n";

static const char *fragmentShaderSource =
    "uniform highp sampler2D texture;                           \n"
    "varying lowp vec4 v_color;                                 \n"
    "varying lowp vec2 v_texcoord;                              \n"
    "void main() {                                              \n"
     "   gl_FragColor = texture2D(texture, v_texcoord);         \n"
    "}                                                          \n";

static GLfloat button_vertices[] = {
    0.0f, 0.0f, 0.1f, 0.1f,
    1.0f, 0.0f, 0.9f, 0.1f,
    0.0f, 1.0f, 0.1f, 0.9f,
    1.0f, 1.0f, 0.9f, 0.9f
};

static GLushort indices[] = {
    0, 1, 2, 3
};


//! [1]
OpenGLWindow::OpenGLWindow(QWindow *parent)
    : QWindow(parent)
    , m_update_pending(false)
    , m_animating(false)
    , m_context(0)
    , m_device(0)
    , m_program(0)
    , m_frame(0)
    , arrayBuf(QOpenGLBuffer::VertexBuffer)
    , indexBuf(QOpenGLBuffer::IndexBuffer)
    , texture(0)
{
    setSurfaceType(QWindow::OpenGLSurface);
    QSurfaceFormat format;
    format.setAlphaBufferSize(8);
    setFormat(format);
    create();
}
//! [1]

OpenGLWindow::~OpenGLWindow()
{
    arrayBuf.destroy();
    indexBuf.destroy();
    delete m_device;

}
//! [2]
void OpenGLWindow::render(QPainter *painter)
{

    m_program->bind();

    const qreal retinaScale = devicePixelRatio();
    glViewport(0, 0, width() * retinaScale, height() * retinaScale);

    texture->bind();

    m_vao.bind();

    glDrawElements(GL_TRIANGLE_FAN, 83, GL_UNSIGNED_SHORT, 0);

    m_vao.release();

    m_program->release();
    m_frame++;

}

void OpenGLWindow::initialize()
{
    const GLfloat PAI = 3.1415926535f;
    const GLint N = 80;
    const GLfloat W = 0.00f;            // round corner rectangle width
    const GLfloat H = 0.00f;           // round corner rectangle height
    const GLfloat rad = 2*PAI/N;
    const GLfloat radius = 0.05f;       // round corner radius
    const GLfloat texture_radius = 0.46f;  // texture map radius
    const GLfloat texture_W = 0.0f;
    const GLfloat texture_H = 0.0f;
    const GLfloat origin_x = 0.0f;  // x > -1-R*cos(rad) && x < 1-R*cos(rad)
    const GLfloat origin_y = 0.0f;
    const GLfloat scale_factor = 1.0f;
    GLfloat circle_vertices[N*8+24];
    GLushort circle_indices[N+3];

    circle_vertices[0] = origin_x;
    circle_vertices[1] = origin_y;
    circle_vertices[2] = 0;
    circle_vertices[3] = scale_factor;
    circle_vertices[4] = 0.5f;          // texture map original coordinate
    circle_vertices[5] = 0.5f;
    circle_vertices[6] = 0;
    circle_vertices[7] = scale_factor;

    circle_vertices[8] = origin_x - W;
    circle_vertices[9] = origin_y + radius + H;
    circle_vertices[10] = 0;
    circle_vertices[11] = scale_factor;
    circle_vertices[12] = 0.5f - texture_W;
    circle_vertices[13] = 0.5f + texture_radius + texture_H;
    circle_vertices[14] = 0;
    circle_vertices[15] = scale_factor;

    for (int i=0; i<N/4; i++) {
        circle_vertices[i*8+16] = origin_x + radius*cos(rad*(i+N/4)) - W;
        circle_vertices[i*8+17] = (origin_y + radius*sin(rad*(i+N/4)) + H)*4/3;
        circle_vertices[i*8+18] = 0;
        circle_vertices[i*8+19] = scale_factor;
        circle_vertices[i*8+20] = texture_radius*cos(rad*(i+N/4))+0.5f - texture_W;
        circle_vertices[i*8+21] = texture_radius*sin(rad*(i+N/4))+0.5f + texture_H;
        circle_vertices[i*8+22] = 0;
        circle_vertices[i*8+23] = scale_factor;
    }
    for (int i=0; i<N/4; i++) {
        circle_vertices[i*8+16+2*N] = origin_x + radius*cos(rad*(i+N/2)) - W;
        circle_vertices[i*8+17+2*N] = (origin_y + radius*sin(rad*(i+N/2)) - H)*4/3;
        circle_vertices[i*8+18+2*N] = 0;
        circle_vertices[i*8+19+2*N] = scale_factor;
        circle_vertices[i*8+20+2*N] = texture_radius*cos(rad*(i+N/2))+0.5f - texture_W;
        circle_vertices[i*8+21+2*N] = texture_radius*sin(rad*(i+N/2))+0.5f - texture_H;
        circle_vertices[i*8+22+2*N] = 0;
        circle_vertices[i*8+23+2*N] = scale_factor;
    }
    for (int i=0; i<N/4; i++) {
        circle_vertices[i*8+16+4*N] = origin_x + radius*cos(rad*(i+N*3/4)) + W;
        circle_vertices[i*8+17+4*N] = (origin_y + radius*sin(rad*(i+N*3/4)) - H)*4/3;
        circle_vertices[i*8+18+4*N] = 0;
        circle_vertices[i*8+19+4*N] = scale_factor;
        circle_vertices[i*8+20+4*N] = texture_radius*cos(rad*(i+N*3/4))+0.5f + texture_W;
        circle_vertices[i*8+21+4*N] = texture_radius*sin(rad*(i+N*3/4))+0.5f - texture_H;
        circle_vertices[i*8+22+4*N] = 0;
        circle_vertices[i*8+23+4*N] = scale_factor;
    }
    for (int i=0; i<N/4; i++) {
        circle_vertices[i*8+16+6*N] = origin_x + radius*cos(rad*i) + W;
        circle_vertices[i*8+17+6*N] = (origin_y + radius*sin(rad*i) + H)*4/3;
        circle_vertices[i*8+18+6*N] = 0;
        circle_vertices[i*8+19+6*N] = scale_factor;
        circle_vertices[i*8+20+6*N] = texture_radius*cos(rad*i)+0.5f + texture_W;
        circle_vertices[i*8+21+6*N] = texture_radius*sin(rad*i)+0.5f + texture_H;
        circle_vertices[i*8+22+6*N] = 0;
        circle_vertices[i*8+23+6*N] = scale_factor;
    }
    circle_vertices[8*N+16] = origin_x - W;
    circle_vertices[8*N+17] = (origin_y + radius + H)*4/3;
    circle_vertices[8*N+18] = 0;
    circle_vertices[8*N+19] = scale_factor;
    circle_vertices[8*N+20] = 0.5f - texture_W;
    circle_vertices[8*N+21] = 0.5f + texture_radius + texture_H;
    circle_vertices[8*N+22] = 0;
    circle_vertices[8*N+23] = scale_factor;

    for (int i=0; i<(N+3); i++) {
        circle_indices[i] = i;
    }

    m_program = new QOpenGLShaderProgram(this);
    m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
    m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
    m_program->link();
    m_program->setUniformValue("texture", 0);

    m_vao.create();
    m_vao.bind();

    arrayBuf.create();
    arrayBuf.bind();    // VBO 0
    arrayBuf.setUsagePattern(QOpenGLBuffer::StaticDraw);
    arrayBuf.allocate(circle_vertices, sizeof(circle_vertices));

    indexBuf.create();
    indexBuf.bind();    // VBO 1
    indexBuf.setUsagePattern(QOpenGLBuffer::StaticDraw);
    indexBuf.allocate(circle_indices, sizeof(circle_indices));

    quintptr offset = 0;
    int a_position = m_program->attributeLocation("a_position");
    m_program->enableAttributeArray(a_position);
    m_program->setAttributeBuffer(a_position, GL_FLOAT, offset, 4, 8*sizeof(GLfloat));
    offset += 4 * sizeof(GLfloat);
    int a_texcoord = m_program->attributeLocation("a_texcoord");
    m_program->enableAttributeArray(a_texcoord);
    m_program->setAttributeBuffer(a_texcoord, GL_FLOAT, offset, 4, 8*sizeof(GLfloat));

    m_vao.release();

    QImage mirror = QImage(":/fullscreen.png").mirrored().convertToFormat(QImage::Format_ARGB32);
    texture = new QOpenGLTexture(mirror);

    // Set nearest filtering mode for texture minification
    texture->setMinificationFilter(QOpenGLTexture::Nearest);

    // Set bilinear filtering mode for texture magnification
    texture->setMagnificationFilter(QOpenGLTexture::Linear);

    // Wrap texture coordinates by repeating
    // f.ex. texture coordinate (1.1, 1.2) is same as (0.1, 0.2)
    texture->setWrapMode(QOpenGLTexture::ClampToBorder);
}

void OpenGLWindow::render()
{
    if (!m_device)
        m_device = new QOpenGLPaintDevice;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    m_device->setSize(size());

    QPainter painter(m_device);
    render(&painter);
}
//! [2]

//! [3]
void OpenGLWindow::renderLater()
{
    if (!m_update_pending) {
        m_update_pending = true;
        QCoreApplication::postEvent(this, new QEvent(QEvent::UpdateRequest));
    }
}

bool OpenGLWindow::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::UpdateRequest:
        m_update_pending = false;
        renderNow();
        return true;
    default:
        return QWindow::event(event);
    }
}

void OpenGLWindow::exposeEvent(QExposeEvent *event)
{
    Q_UNUSED(event);

    if (isExposed())
        renderNow();
}
//! [3]

//! [4]
void OpenGLWindow::renderNow()
{
    if (!isExposed())
        return;

    bool needsInitialize = false;

    if (!m_context) {
        m_context = new QOpenGLContext(this);
        m_context->setFormat(requestedFormat());
        m_context->create();

        needsInitialize = true;
    }

    m_context->makeCurrent(this);

    if (needsInitialize) {
        initializeOpenGLFunctions();
        initialize();
    }

    render();

    m_context->swapBuffers(this);

    if (m_animating)
        renderLater();
}
//! [4]

//! [5]
void OpenGLWindow::setAnimating(bool animating)
{
    m_animating = animating;

    if (animating)
        renderLater();
}
//! [5]

