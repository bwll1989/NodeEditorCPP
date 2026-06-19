#pragma once

#include <memory>
#include <QColor>
#include <QFrame>
#include <QLabel>
#include <QMouseEvent>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLWidget>
#include <QPainter>
#include <QRect>
#include <QVBoxLayout>
#include <QtGlobal>
#include <opencv2/core/mat.hpp>
#include <opencv2/imgproc.hpp>

namespace Nodes
{
/**
 * @brief 基于 OpenGL 的 ROI 预览控件
 *
 * 该控件直接接收 `cv::Mat`，上传为 OpenGL 纹理进行显示，
 * 避免在预览路径中反复转换成 `QImage`。
 * 鼠标拖拽交互仍以原图像像素坐标维护 ROI。
 */
class ROIImageView final : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父控件
     */
    explicit ROIImageView(QWidget* parent = nullptr)
        : QOpenGLWidget(parent)
    {
        setMinimumSize(320, 200);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        setMouseTracking(true);
    }

    /**
     * @brief 析构函数
     *
     * 销毁 OpenGL 资源前先确保上下文已激活。
     */
    ~ROIImageView() override
    {
        if (context()) {
            makeCurrent();
            destroyGlResources();
            doneCurrent();
        }
    }

    /**
     * @brief 设置当前预览图像
     * @param image OpenCV 图像
     */
    void setImage(const cv::Mat& image)
    {
        if (image.empty()) {
            m_frame = cv::Mat();
            m_frameSize = QSize();
            m_roiRect = QRect();
            m_dragging = false;
            m_textureDirty = true;
            update();
            return;
        }

        if (image.type() == CV_8UC1 || image.type() == CV_8UC3 || image.type() == CV_8UC4) {
            m_frame = image.clone();
        } else {
            cv::Mat converted8U;
            image.convertTo(converted8U, CV_8U);
            if (converted8U.channels() == 1 || converted8U.channels() == 3 || converted8U.channels() == 4) {
                m_frame = converted8U;
            } else {
                cv::Mat bgr;
                cv::cvtColor(converted8U, bgr, cv::COLOR_BGR2RGB);
                m_frame = bgr;
            }
        }

        m_frameSize = QSize(m_frame.cols, m_frame.rows);
        m_roiRect = clampRectToImage(m_roiRect);
        m_textureDirty = true;
        update();
    }

    /**
     * @brief 程序化设置 ROI 选区
     * @param rect 以原图像像素坐标表示的矩形
     */
    void setRoiRect(const QRect& rect)
    {
        const QRect clamped = clampRectToImage(rect);
        if (m_roiRect == clamped) {
            return;
        }
        m_roiRect = clamped;
        update();
    }

    /**
     * @brief 获取当前 ROI 选区
     * @return 原图像像素坐标矩形
     */
    QRect roiRect() const
    {
        return m_roiRect;
    }

    /**
     * @brief 判断当前是否存在可显示图像
     * @return 存在图像返回 true
     */
    bool hasImage() const
    {
        return !m_frame.empty();
    }

signals:
    /**
     * @brief 当用户用鼠标修改 ROI 时发出
     * @param rect 原图像像素坐标矩形
     */
    void roiRectChanged(const QRect& rect);

protected:
    /**
     * @brief 初始化 OpenGL 资源
     */
    void initializeGL() override
    {
        initializeOpenGLFunctions();
        glDisable(GL_DEPTH_TEST);
        glClearColor(0.10f, 0.10f, 0.10f, 1.0f);

        m_program = std::make_unique<QOpenGLShaderProgram>();
        m_program->addShaderFromSourceCode(
            QOpenGLShader::Vertex,
            "attribute vec2 aPos;\n"
            "attribute vec2 aTexCoord;\n"
            "varying vec2 vTexCoord;\n"
            "void main() {\n"
            "    vTexCoord = aTexCoord;\n"
            "    gl_Position = vec4(aPos, 0.0, 1.0);\n"
            "}\n");
        m_program->addShaderFromSourceCode(
            QOpenGLShader::Fragment,
            "uniform sampler2D uTexture;\n"
            "uniform int uChannels;\n"
            "uniform int uUseTexture;\n"
            "uniform vec4 uSolidColor;\n"
            "varying vec2 vTexCoord;\n"
            "void main() {\n"
            "    if (uUseTexture == 0) {\n"
            "        gl_FragColor = uSolidColor;\n"
            "        return;\n"
            "    }\n"
            "    vec4 color = texture2D(uTexture, vTexCoord);\n"
            "    if (uChannels == 1) {\n"
            "        gl_FragColor = vec4(color.r, color.r, color.r, 1.0);\n"
            "    } else {\n"
            "        gl_FragColor = color;\n"
            "    }\n"
            "}\n");
        m_program->link();

        m_vertexBuffer.create();
        m_vertexBuffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    }

    /**
     * @brief 调整视口尺寸
     * @param w 新宽度
     * @param h 新高度
     */
    void resizeGL(int w, int h) override
    {
        glViewport(0, 0, w, h);
    }

    /**
     * @brief 绘制图像与 ROI 选区
     */
    void paintGL() override
    {
        glClear(GL_COLOR_BUFFER_BIT);

        if (!m_frame.empty()) {
            uploadTextureIfNeeded();
            drawTextureQuad();
            drawRoiOverlay();
            return;
        }

        QPainter painter(this);
        painter.fillRect(rect(), QColor(26, 26, 26));
        painter.setPen(QColor(180, 180, 180));
        painter.drawText(rect(), Qt::AlignCenter, tr("请先连接图像输入"));
        painter.end();
    }

    /**
     * @brief 处理鼠标按下，开始绘制 ROI
     * @param event 鼠标事件
     */
    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() != Qt::LeftButton || m_frame.empty()) {
            return;
        }
        const QRect target = imageTargetRect();
        if (!target.contains(event->pos())) {
            return;
        }
        m_dragging = true;
        m_dragStartImagePos = widgetPointToImagePoint(event->pos());
        updateRoiFromUserRect(QRect(m_dragStartImagePos, QSize(1, 1)));
    }

    /**
     * @brief 处理鼠标拖动，实时更新 ROI
     * @param event 鼠标事件
     */
    void mouseMoveEvent(QMouseEvent* event) override
    {
        if (!m_dragging || m_frame.empty()) {
            return;
        }
        const QPoint currentImagePos = widgetPointToImagePoint(event->pos());
        const int left = qMin(m_dragStartImagePos.x(), currentImagePos.x());
        const int top = qMin(m_dragStartImagePos.y(), currentImagePos.y());
        const int right = qMax(m_dragStartImagePos.x(), currentImagePos.x());
        const int bottom = qMax(m_dragStartImagePos.y(), currentImagePos.y());
        updateRoiFromUserRect(QRect(left, top, right - left + 1, bottom - top + 1));
    }

    /**
     * @brief 处理鼠标释放，结束 ROI 绘制
     * @param event 鼠标事件
     */
    void mouseReleaseEvent(QMouseEvent* event) override
    {
        if (event->button() != Qt::LeftButton) {
            return;
        }
        if (!m_dragging) {
            return;
        }
        m_dragging = false;
        const QPoint currentImagePos = widgetPointToImagePoint(event->pos());
        const int left = qMin(m_dragStartImagePos.x(), currentImagePos.x());
        const int top = qMin(m_dragStartImagePos.y(), currentImagePos.y());
        const int right = qMax(m_dragStartImagePos.x(), currentImagePos.x());
        const int bottom = qMax(m_dragStartImagePos.y(), currentImagePos.y());
        updateRoiFromUserRect(QRect(left, top, right - left + 1, bottom - top + 1));
    }

private:
    /**
     * @brief 销毁 OpenGL 资源
     */
    void destroyGlResources()
    {
        if (m_textureId != 0) {
            glDeleteTextures(1, &m_textureId);
            m_textureId = 0;
        }
        if (m_vertexBuffer.isCreated()) {
            m_vertexBuffer.destroy();
        }
        m_program.reset();
    }

    /**
     * @brief 按需上传纹理数据
     */
    void uploadTextureIfNeeded()
    {
        if (!m_textureDirty || m_frame.empty()) {
            return;
        }

        if (m_textureId == 0) {
            glGenTextures(1, &m_textureId);
        }
        glBindTexture(GL_TEXTURE_2D, m_textureId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        GLenum format = GL_RGB;
        GLint internalFormat = GL_RGB8;
        m_textureChannels = m_frame.channels();
        if (m_textureChannels == 1) {
            format = GL_RED;
            internalFormat = GL_R8;
        } else if (m_textureChannels == 3) {
            format = GL_BGR;
            internalFormat = GL_RGB8;
        } else if (m_textureChannels == 4) {
            format = GL_BGRA;
            internalFormat = GL_RGBA8;
        }

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     internalFormat,
                     m_frame.cols,
                     m_frame.rows,
                     0,
                     format,
                     GL_UNSIGNED_BYTE,
                     m_frame.data);
        glBindTexture(GL_TEXTURE_2D, 0);
        m_textureDirty = false;
    }

    /**
     * @brief 使用着色器绘制纹理图像
     */
    void drawTextureQuad()
    {
        if (m_textureId == 0 || !m_program) {
            return;
        }

        const QRect target = imageTargetRect();
        if (!target.isValid() || width() <= 0 || height() <= 0) {
            return;
        }

        const float left = static_cast<float>(2.0 * target.left() / width() - 1.0);
        const float right = static_cast<float>(2.0 * (target.right() + 1) / width() - 1.0);
        const float top = static_cast<float>(1.0 - 2.0 * target.top() / height());
        const float bottom = static_cast<float>(1.0 - 2.0 * (target.bottom() + 1) / height());

        const GLfloat vertices[] = {
            left,  top,    0.0f, 0.0f,
            left,  bottom, 0.0f, 1.0f,
            right, top,    1.0f, 0.0f,
            right, bottom, 1.0f, 1.0f
        };

        if (!m_vertexBuffer.isCreated()) {
            m_vertexBuffer.create();
            m_vertexBuffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);
        }

        m_vertexBuffer.bind();
        m_vertexBuffer.allocate(vertices, static_cast<int>(sizeof(vertices)));

        m_program->bind();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_textureId);
        m_program->setUniformValue("uTexture", 0);
        m_program->setUniformValue("uChannels", m_textureChannels);
        m_program->setUniformValue("uUseTexture", 1);
        m_program->setUniformValue("uSolidColor", QColor(0, 0, 0, 0));

        const int posLoc = m_program->attributeLocation("aPos");
        const int texLoc = m_program->attributeLocation("aTexCoord");
        m_program->enableAttributeArray(posLoc);
        m_program->enableAttributeArray(texLoc);
        m_program->setAttributeBuffer(posLoc, GL_FLOAT, 0, 2, 4 * static_cast<int>(sizeof(GLfloat)));
        m_program->setAttributeBuffer(texLoc, GL_FLOAT, 2 * static_cast<int>(sizeof(GLfloat)), 2, 4 * static_cast<int>(sizeof(GLfloat)));

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        m_program->disableAttributeArray(posLoc);
        m_program->disableAttributeArray(texLoc);
        glBindTexture(GL_TEXTURE_2D, 0);
        m_program->release();
        m_vertexBuffer.release();
    }

    /**
     * @brief 绘制 ROI 叠加层（暗化 ROI 外区域 + ROI 边框）
     */
    void drawRoiOverlay()
    {
        if (!m_program || m_frameSize.isEmpty() || !m_roiRect.isValid() || m_roiRect.width() <= 0 || m_roiRect.height() <= 0) {
            return;
        }

        const QRect target = imageTargetRect();
        if (!target.isValid()) {
            return;
        }

        const QRect widgetRect = imageRectToWidgetRect(m_roiRect);
        const QRect roiInTarget = widgetRect.intersected(target);
        if (!roiInTarget.isValid() || roiInTarget.width() <= 0 || roiInTarget.height() <= 0) {
            return;
        }

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        const QColor dimColor(0, 0, 0, 120);
        const int topH = roiInTarget.top() - target.top();
        if (topH > 0) {
            drawSolidQuadPx(QRect(target.left(), target.top(), target.width(), topH), dimColor);
        }
        const int bottomY = roiInTarget.bottom() + 1;
        const int bottomH = target.bottom() - bottomY + 1;
        if (bottomH > 0) {
            drawSolidQuadPx(QRect(target.left(), bottomY, target.width(), bottomH), dimColor);
        }
        const int leftW = roiInTarget.left() - target.left();
        if (leftW > 0) {
            drawSolidQuadPx(QRect(target.left(), roiInTarget.top(), leftW, roiInTarget.height()), dimColor);
        }
        const int rightX = roiInTarget.right() + 1;
        const int rightW = target.right() - rightX + 1;
        if (rightW > 0) {
            drawSolidQuadPx(QRect(rightX, roiInTarget.top(), rightW, roiInTarget.height()), dimColor);
        }

        drawSolidQuadPx(roiInTarget, QColor(255, 215, 0, 40));
        drawSolidRectOutlinePx(roiInTarget, 2, QColor(255, 215, 0, 255));
    }

    /**
     * @brief 绘制纯色矩形（像素坐标）
     * @param rectPx 像素坐标矩形
     * @param color 颜色（含 alpha）
     */
    void drawSolidQuadPx(const QRect& rectPx, const QColor& color)
    {
        if (!rectPx.isValid() || rectPx.width() <= 0 || rectPx.height() <= 0 || !m_program) {
            return;
        }
        if (width() <= 0 || height() <= 0) {
            return;
        }

        const float left = static_cast<float>(2.0 * rectPx.left() / width() - 1.0);
        const float right = static_cast<float>(2.0 * (rectPx.right() + 1) / width() - 1.0);
        const float top = static_cast<float>(1.0 - 2.0 * rectPx.top() / height());
        const float bottom = static_cast<float>(1.0 - 2.0 * (rectPx.bottom() + 1) / height());

        const GLfloat vertices[] = {
            left,  top,    0.0f, 0.0f,
            left,  bottom, 0.0f, 1.0f,
            right, top,    1.0f, 0.0f,
            right, bottom, 1.0f, 1.0f
        };

        m_vertexBuffer.bind();
        m_vertexBuffer.allocate(vertices, static_cast<int>(sizeof(vertices)));

        m_program->bind();
        m_program->setUniformValue("uUseTexture", 0);
        m_program->setUniformValue("uChannels", 3);
        m_program->setUniformValue("uSolidColor", color);

        const int posLoc = m_program->attributeLocation("aPos");
        const int texLoc = m_program->attributeLocation("aTexCoord");
        m_program->enableAttributeArray(posLoc);
        m_program->enableAttributeArray(texLoc);
        m_program->setAttributeBuffer(posLoc, GL_FLOAT, 0, 2, 4 * static_cast<int>(sizeof(GLfloat)));
        m_program->setAttributeBuffer(texLoc, GL_FLOAT, 2 * static_cast<int>(sizeof(GLfloat)), 2, 4 * static_cast<int>(sizeof(GLfloat)));

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        m_program->disableAttributeArray(posLoc);
        m_program->disableAttributeArray(texLoc);
        m_program->release();
        m_vertexBuffer.release();
    }

    /**
     * @brief 绘制纯色矩形边框（像素坐标）
     * @param rectPx 像素坐标矩形
     * @param thicknessPx 边框厚度（像素）
     * @param color 边框颜色（含 alpha）
     */
    void drawSolidRectOutlinePx(const QRect& rectPx, int thicknessPx, const QColor& color)
    {
        if (!rectPx.isValid() || rectPx.width() <= 0 || rectPx.height() <= 0) {
            return;
        }
        const int t = qMax(1, thicknessPx);
        drawSolidQuadPx(QRect(rectPx.left(), rectPx.top(), rectPx.width(), t), color);
        drawSolidQuadPx(QRect(rectPx.left(), rectPx.bottom() - t + 1, rectPx.width(), t), color);
        drawSolidQuadPx(QRect(rectPx.left(), rectPx.top(), t, rectPx.height()), color);
        drawSolidQuadPx(QRect(rectPx.right() - t + 1, rectPx.top(), t, rectPx.height()), color);
    }

    /**
     * @brief 计算图像在控件中的实际显示区域
     * @return 保持宽高比缩放后的目标矩形
     */
    QRect imageTargetRect() const
    {
        if (m_frameSize.isEmpty()) {
            return {};
        }
        const QRect area = rect().adjusted(8, 8, -8, -8);
        const QSize scaled = m_frameSize.scaled(area.size(), Qt::KeepAspectRatio);
        const int x = area.x() + (area.width() - scaled.width()) / 2;
        const int y = area.y() + (area.height() - scaled.height()) / 2;
        return QRect(x, y, scaled.width(), scaled.height());
    }

    /**
     * @brief 将控件坐标映射为原图像像素坐标
     * @param widgetPoint 控件坐标
     * @return 原图像像素坐标
     */
    QPoint widgetPointToImagePoint(const QPoint& widgetPoint) const
    {
        if (m_frameSize.isEmpty()) {
            return {};
        }
        const QRect target = imageTargetRect();
        const int clampedX = qBound(target.left(), widgetPoint.x(), target.right());
        const int clampedY = qBound(target.top(), widgetPoint.y(), target.bottom());
        const double xRatio = static_cast<double>(m_frameSize.width()) / static_cast<double>(target.width());
        const double yRatio = static_cast<double>(m_frameSize.height()) / static_cast<double>(target.height());
        const int imageX = qBound(0, static_cast<int>((clampedX - target.left()) * xRatio), m_frameSize.width() - 1);
        const int imageY = qBound(0, static_cast<int>((clampedY - target.top()) * yRatio), m_frameSize.height() - 1);
        return QPoint(imageX, imageY);
    }

    /**
     * @brief 将原图像矩形映射为控件显示矩形
     * @param imageRect 原图像像素坐标矩形
     * @return 控件坐标矩形
     */
    QRect imageRectToWidgetRect(const QRect& imageRect) const
    {
        if (m_frameSize.isEmpty() || !imageRect.isValid()) {
            return {};
        }
        const QRect target = imageTargetRect();
        const double xScale = static_cast<double>(target.width()) / static_cast<double>(m_frameSize.width());
        const double yScale = static_cast<double>(target.height()) / static_cast<double>(m_frameSize.height());
        const int x = target.x() + static_cast<int>(imageRect.x() * xScale);
        const int y = target.y() + static_cast<int>(imageRect.y() * yScale);
        const int width = qMax(1, static_cast<int>(imageRect.width() * xScale));
        const int height = qMax(1, static_cast<int>(imageRect.height() * yScale));
        return QRect(x, y, width, height);
    }

    /**
     * @brief 将 ROI 限制在当前图像范围内
     * @param rect 原图像像素坐标矩形
     * @return 裁剪后的合法矩形
     */
    QRect clampRectToImage(const QRect& rect) const
    {
        if (m_frameSize.isEmpty() || !rect.isValid()) {
            return {};
        }
        const QRect imageBounds(0, 0, m_frameSize.width(), m_frameSize.height());
        return rect.intersected(imageBounds);
    }

    /**
     * @brief 处理用户拖拽产生的新 ROI，并发出信号
     * @param rect 原图像像素坐标矩形
     */
    void updateRoiFromUserRect(const QRect& rect)
    {
        const QRect clamped = clampRectToImage(rect);
        if (m_roiRect == clamped) {
            update();
            return;
        }
        m_roiRect = clamped;
        update();
        emit roiRectChanged(m_roiRect);
    }

private:
    cv::Mat m_frame;
    QSize m_frameSize;
    QRect m_roiRect;
    bool m_dragging = false;
    QPoint m_dragStartImagePos;

    bool m_textureDirty = true;
    GLuint m_textureId = 0;
    int m_textureChannels = 3;
    std::unique_ptr<QOpenGLShaderProgram> m_program;
    QOpenGLBuffer m_vertexBuffer{QOpenGLBuffer::VertexBuffer};
};

/**
 * @brief ROI 节点内嵌界面
 *
 * 提供一块图像预览区和一行状态文本，用于提示当前 ROI 参数。
 */
class ROIInterface final : public QFrame
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父控件
     */
    explicit ROIInterface(QWidget* parent = nullptr)
        : QFrame(parent)
    {
        setFrameShape(QFrame::NoFrame);
        setMinimumSize(340, 260);

        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(6);

        hintLabel = new QLabel(tr("左键拖动绘制 ROI，输出框选区域图像"), this);
        hintLabel->setWordWrap(true);
        layout->addWidget(hintLabel);

        imageView = new ROIImageView(this);
        layout->addWidget(imageView, 1);

        infoLabel = new QLabel(tr("当前 ROI：未选择"), this);
        infoLabel->setWordWrap(true);
        layout->addWidget(infoLabel);
    }

    /**
     * @brief 更新界面中的 ROI 状态文本
     * @param roiRect 选区矩形
     * @param imageSize 输入图像尺寸
     */
    void setRoiInfo(const QRect& roiRect, const QSize& imageSize)
    {
        if (!roiRect.isValid() || roiRect.width() <= 0 || roiRect.height() <= 0) {
            infoLabel->setText(tr("当前 ROI：未选择"));
            return;
        }
        infoLabel->setText(
            tr("当前 ROI：X=%1, Y=%2, W=%3, H=%4，图像尺寸：%5x%6")
                .arg(roiRect.x())
                .arg(roiRect.y())
                .arg(roiRect.width())
                .arg(roiRect.height())
                .arg(imageSize.width())
                .arg(imageSize.height()));
    }

public:
    QLabel* hintLabel = nullptr;
    ROIImageView* imageView = nullptr;
    QLabel* infoLabel = nullptr;
};
} // namespace Nodes
