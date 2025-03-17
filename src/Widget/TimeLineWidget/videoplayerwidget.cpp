#include "videoplayerwidget.hpp"
#include <QOpenGLBuffer>
#include <QPainter>
#include <QFont>
#include <QDebug>
#include <QFileDialog>  // Include for file dialog

// 顶点着色器
static const char* vertexShaderSource =
    "attribute vec4 position;\n"
    "attribute vec2 texCoord;\n"
    "varying vec2 texCoordVarying;\n"
    "uniform mat4 matrix;\n"
    "void main() {\n"
    "    gl_Position = matrix * position;\n"
    "    texCoordVarying = texCoord;\n"
    "}\n";

// 片段着色器
static const char* fragmentShaderSource =
    "varying vec2 texCoordVarying;\n"
    "uniform sampler2D texture;\n"
    "void main() {\n"
    "    gl_FragColor = texture2D(texture, texCoordVarying);\n"
    "}\n";

VideoPlayerWidget::VideoPlayerWidget(QWidget* parent)
    : QOpenGLWidget(parent)
    , m_program(nullptr)
    , m_texture(nullptr)
    , m_formatCtx(nullptr)
    , m_codecCtx(nullptr)
    , m_frame(nullptr)
    , m_packet(nullptr)
    , m_swsCtx(nullptr)
    , m_videoStreamIndex(-1)
    , m_isPlaying(false)
    , m_keepAspectRatio(true)
{
    avformat_network_init();
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &VideoPlayerWidget::decodeVideo);

    // 设置背景色为黑色
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, Qt::black);
    setPalette(pal);

    QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    setFormat(format);
    // openFile("D:/02_Media/01_Videos/test.mp4");
    // play();
}

VideoPlayerWidget::~VideoPlayerWidget()
{
    makeCurrent();
    delete m_texture;
    delete m_program;
    doneCurrent();

    if (m_frame) av_frame_free(&m_frame);
    if (m_packet) av_packet_free(&m_packet);
    if (m_codecCtx) avcodec_free_context(&m_codecCtx);
    if (m_formatCtx) avformat_close_input(&m_formatCtx);
    if (m_swsCtx) sws_freeContext(m_swsCtx);
}

void VideoPlayerWidget::openFile(const QString& filePath)
{
    avformat_open_input(&m_formatCtx, filePath.toStdString().c_str(), nullptr, nullptr);
    avformat_find_stream_info(m_formatCtx, nullptr);

    for (unsigned int i = 0; i < m_formatCtx->nb_streams; ++i) {
        if (m_formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            m_videoStreamIndex = i;
            break;
        }
    }

    const AVCodec* codec = avcodec_find_decoder(m_formatCtx->streams[m_videoStreamIndex]->codecpar->codec_id);
    m_codecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(m_codecCtx, m_formatCtx->streams[m_videoStreamIndex]->codecpar);
    avcodec_open2(m_codecCtx, codec, nullptr);
    // Specify the hardware device type as GPU
    AVBufferRef* hw_device_ctx = nullptr;
    if (av_hwdevice_ctx_create(&hw_device_ctx, AV_HWDEVICE_TYPE_VAAPI, nullptr, nullptr, 0) < 0) {
        qDebug() << "Failed to create hardware device context";
    } else {
        m_codecCtx->hw_device_ctx = av_buffer_ref(hw_device_ctx);
    }
    qDebug() << "hw_device_ctx" << m_codecCtx->hw_device_ctx->data;
    m_frame = av_frame_alloc();
    m_packet = av_packet_alloc();
    m_swsCtx = sws_getContext(m_codecCtx->width, m_codecCtx->height, m_codecCtx->pix_fmt,
                              m_codecCtx->width, m_codecCtx->height, AV_PIX_FMT_RGBA,
                              SWS_BILINEAR, nullptr, nullptr, nullptr);
}

void VideoPlayerWidget::play()
{
    m_isPlaying = true;
    m_timer->start(1000 / 30); // Assume 30 FPS for simplicity
}

void VideoPlayerWidget::pause()
{
    m_isPlaying = false;
    m_timer->stop();
}

void VideoPlayerWidget::stop()
{
    m_isPlaying = false;
    m_timer->stop();
    av_seek_frame(m_formatCtx, m_videoStreamIndex, 0, AVSEEK_FLAG_BACKWARD);
}

void VideoPlayerWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);  // 设置清除色为黑色

    setupShaders();
    setupVertexBuffers();

    m_texture = new QOpenGLTexture(QOpenGLTexture::Target2D);
    m_texture->setMinificationFilter(QOpenGLTexture::Linear);
    m_texture->setMagnificationFilter(QOpenGLTexture::Linear);
    m_texture->setWrapMode(QOpenGLTexture::ClampToEdge);
}

void VideoPlayerWidget::setupShaders()
{
    m_program = new QOpenGLShaderProgram;
    m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
    m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
    m_program->link();

    m_posAttr = m_program->attributeLocation("position");
    m_texAttr = m_program->attributeLocation("texCoord");
    m_matrixUniform = m_program->uniformLocation("matrix");
}

void VideoPlayerWidget::setupVertexBuffers()
{
    // 设置顶点坐标
    m_vertices = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f,
        -1.0f,  1.0f
    };

    // 设置纹理坐标
    m_texCoords = {
        0.0f, 1.0f,
        1.0f, 1.0f,
        1.0f, 0.0f,
        0.0f, 0.0f
    };
}

void VideoPlayerWidget::decodeVideo()
{
    if (!m_isPlaying) return;

    if (av_read_frame(m_formatCtx, m_packet) >= 0) {
        if (m_packet->stream_index == m_videoStreamIndex) {
            avcodec_send_packet(m_codecCtx, m_packet);
            if (avcodec_receive_frame(m_codecCtx, m_frame) == 0) {
                QImage image(m_codecCtx->width, m_codecCtx->height, QImage::Format_RGBA8888);
                uint8_t* dest[4] = { image.bits(), nullptr, nullptr, nullptr };
                int destLinesize[4] = { static_cast<int>(image.bytesPerLine()), 0, 0, 0 };
                sws_scale(m_swsCtx, m_frame->data, m_frame->linesize, 0, m_codecCtx->height, dest, destLinesize);
                updateTexture(image);
            }
        }
        av_packet_unref(m_packet);
    }
}

void VideoPlayerWidget::updateTexture(const QImage& frame)
{
    makeCurrent();
    m_texture->destroy();
    m_texture->create();
    m_texture->setData(frame);
    update();
}

void VideoPlayerWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);

    if (!m_texture || !m_texture->isCreated()) return;

    m_program->bind();
    m_texture->bind();

    m_program->setUniformValue(m_matrixUniform, m_projection);

    m_program->enableAttributeArray(m_posAttr);
    m_program->enableAttributeArray(m_texAttr);
    m_program->setAttributeArray(m_posAttr, m_vertices.constData(), 2);
    m_program->setAttributeArray(m_texAttr, m_texCoords.constData(), 2);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    m_program->disableAttributeArray(m_posAttr);
    m_program->disableAttributeArray(m_texAttr);
    m_program->release();
}

void VideoPlayerWidget::resizeGL(int w, int h)
{
    m_projection.setToIdentity();
    m_projection.ortho(-1, 1, -1, 1, -1, 1);
}

void VideoPlayerWidget::setKeepAspectRatio(bool keep)
{
    if (m_keepAspectRatio != keep) {
        m_keepAspectRatio = keep;
        update();
    }
}

void VideoPlayerWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        QString filePath = QFileDialog::getOpenFileName(this, tr("Open Video File"), "", tr("Video Files (*.mp4 *.avi *.mkv *.mov);;All Files (*)"));
        if (!filePath.isEmpty()) {
            openFile(filePath);
            play();
        }
    }
}

