#pragma once

#include <iostream>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeDelegateModelRegistry>
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>

#include <QImage>
#include <QMatrix4x4>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions>
#include <QOpenGLShader>
#include <QOpenGLShaderProgram>
#include <QtOpenGL/QOpenGLFramebufferObject>
#include <QtOpenGL/QOpenGLShaderProgram>

#include "Elements/ColorEditorWidget/ColorEditorWidget.hpp"
#include "ImageLayoutInterface.hpp"
#include "NodeDataList.hpp"
#include "opencv2/imgcodecs/imgcodecs.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "Common/BaseClass/AbstractDelegateModel.h"
using namespace NodeDataTypes;
using namespace Nodes;

namespace Nodes
{
    class ImageLayoutModel final : public AbstractDelegateModel
    {
        Q_OBJECT

    public:
        /**
         * @brief 构造函数：初始化端口、UI信号与默认输出图像
         */
        ImageLayoutModel(){
            InPortCount = 4;
            OutPortCount=1;
            CaptionVisible=true;
            Caption="Image Layout";
            WidgetEmbeddable=false;
            Resizable=false;
            PortEditable=true;
            m_watcher = new QFutureWatcher<void>(this);
            // 连接输入框变化信号到槽
            connect(widget->widthEdit, &IntDragValueWidget::valueChanged, this, & ImageLayoutModel::onInputChanged);
            connect(widget->heightEdit, &IntDragValueWidget::valueChanged, this, & ImageLayoutModel::onInputChanged);
            connect(widget->layoutCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &ImageLayoutModel::onInputChanged);
            connect(widget->rowsSpin, &IntDragValueWidget::valueChanged, this, &ImageLayoutModel::onInputChanged);
            connect(widget->colsSpin, &IntDragValueWidget::valueChanged, this, &ImageLayoutModel::onInputChanged);
            connect(widget->spacingSpin, &IntDragValueWidget::valueChanged, this, &ImageLayoutModel::onInputChanged);
            connect(colorEditorWidget, &ColorEditorWidget::colorChanged, this, & ImageLayoutModel::onInputChanged);
            connect(widget->colorEditButton, &QPushButton::clicked, this, & ImageLayoutModel::toggleEditorMode);
            updateImage();
        };

        ~ ImageLayoutModel() override{
            if (colorEditorWidget) {
                colorEditorWidget->setParent(nullptr);
                colorEditorWidget->deleteLater();
            }
            releaseGL();
        }

        QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override{
            return ImageData().type();
        };

        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override{
            if (portType == QtNodes::PortType::In)
            {
                return "Image"+QString::number(portIndex + 1);
            }else
            {
                return "Image";
            }
        }

        std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex port) override{
            // 输出当前生成的图像
            if (m_image.empty()) return nullptr;
            return std::make_shared<ImageData>(m_image);
        }

        void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex port) override{
            auto image = std::dynamic_pointer_cast<ImageData>(nodeData);
            if (image && !image->mat().empty()) {
                m_inImages[port] = image;
            } else {
                m_inImages.erase(port);
            }
            updateImage();
            Q_EMIT dataUpdated(0);
        }

        QWidget *embeddedWidget() override { return widget; }

        QJsonObject save() const override{
            QJsonObject modelJson1;
            modelJson1["width"] = widget->widthEdit->value();
            modelJson1["height"] = widget->heightEdit->value();
            modelJson1["layout"] = widget->layoutCombo->currentIndex();
            modelJson1["rows"] = widget->rowsSpin->value();
            modelJson1["cols"] = widget->colsSpin->value();
            modelJson1["spacing"] = widget->spacingSpin->value();
            modelJson1["color"] = colorEditorWidget->getColor().name(QColor::HexArgb);
            QJsonObject modelJson  = NodeDelegateModel::save();
            modelJson["values"]=modelJson1;
            return modelJson;
        }

        void load(const QJsonObject &p) override{
            QJsonValue v = p["values"];
            if (!v.isUndefined()&&v.isObject()) {
                widget->widthEdit->setValue(v["width"].toInt(0));
                widget->heightEdit->setValue(v["height"].toInt(0));
                widget->layoutCombo->setCurrentIndex(v["layout"].toInt(0));
                widget->rowsSpin->setValue(v["rows"].toInt(2));
                widget->colsSpin->setValue(v["cols"].toInt(2));
                widget->spacingSpin->setValue(v["spacing"].toInt(0));
                colorEditorWidget->setColor(QColor(v["color"].toString()));
                updateImage();
            }
        }

    public Q_SLOTS:
        /**
         * @brief 切换颜色编辑器为独立窗口显示
         */
        void toggleEditorMode(){
            // 移除父子关系，使其成为独立窗口
            colorEditorWidget->setParent(nullptr);

            // 设置为独立窗口
            colorEditorWidget->setWindowTitle("颜色编辑器");

            // 设置窗口图标
            colorEditorWidget->setWindowIcon(QIcon(":/icons/icons/curve.png"));

            // 设置窗口标志：独立窗口 + 置顶显示 + 关闭按钮
            colorEditorWidget->setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint | Qt::WindowCloseButtonHint);

            // 设置窗口属性：当关闭时自动删除
            colorEditorWidget->setAttribute(Qt::WA_DeleteOnClose, false); // 不自动删除，我们手动管理
            colorEditorWidget->setAttribute(Qt::WA_QuitOnClose, false);   // 关闭窗口时不退出应用程序

            // 设置窗口大小和显示
            colorEditorWidget->resize(800, 400);
            colorEditorWidget->show();
            // 激活窗口并置于前台
            colorEditorWidget->activateWindow();
            colorEditorWidget->raise();
        }

    private Q_SLOTS:
        /**
         * @brief 生成布局后的合成图像，并更新预览与输出
         */
        void updateImage(){
        if (m_width <= 0) m_width = 1;
        if (m_height <= 0) m_height = 1;

        auto width = m_width;
        auto height = m_height;
        auto color = m_color;
        auto mode = widget->layoutCombo->currentIndex();
        auto rows = widget->rowsSpin->value();
        auto cols = widget->colsSpin->value();
        auto spacing = widget->spacingSpin->value();
        auto inputs = m_inImages;

        if (composeWithOpenGL(width, height, color, mode, rows, cols, spacing, inputs)) {
            QImage preview = ImageData(m_image).image();
            // const QSize previewSize = widget->display->size().isEmpty() ? QSize(160, 90) : widget->display->size();
            // widget->display->setPixmap(QPixmap::fromImage(preview.scaled(previewSize, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
            Q_EMIT dataUpdated(0);
        } else {
            QFuture<void> future = QtConcurrent::run([this, width, height, color, mode, rows, cols, spacing, inputs]() {
                cv::Mat canvas(height, width, CV_8UC4, cv::Scalar(color.blue(), color.green(), color.red(), color.alpha()));

                std::vector<cv::Mat> mats;
                mats.reserve(inputs.size());
                for (int i = 0; i < InPortCount; ++i) {
                    auto it = inputs.find(i);
                    if (it == inputs.end() || !it->second) continue;
                    const cv::Mat src = it->second->mat();
                    if (src.empty()) continue;
                    cv::Mat bgra;
                    if (src.channels() == 4) {
                        if (src.type() == CV_8UC4) bgra = src;
                        else src.convertTo(bgra, CV_8UC4);
                    } else if (src.channels() == 3) {
                        cv::cvtColor(src, bgra, cv::COLOR_BGR2BGRA);
                    } else if (src.channels() == 1) {
                        cv::cvtColor(src, bgra, cv::COLOR_GRAY2BGRA);
                    } else {
                        continue;
                    }
                    mats.push_back(bgra);
                }

                const int n = static_cast<int>(mats.size());
                if (n <= 0) {
                    m_image = canvas.clone();
                    return;
                }

                int gridRows = 1;
                int gridCols = 1;
                if (mode == 0) { // Horizontal
                    gridRows = 1;
                    gridCols = n;
                } else if (mode == 1) { // Vertical
                    gridRows = n;
                    gridCols = 1;
                } else { // Grid
                    gridRows = std::max(1, rows);
                    gridCols = std::max(1, cols);
                    const int needRows = (n + gridCols - 1) / gridCols;
                    if (needRows > gridRows) gridRows = needRows;
                }

                const int safeSpacing = std::max(0, spacing);
                const int cellW = std::max(1, (width - safeSpacing * (gridCols - 1)) / gridCols);
                const int cellH = std::max(1, (height - safeSpacing * (gridRows - 1)) / gridRows);

                for (int i = 0; i < n && i < gridRows * gridCols; ++i) {
                    const int r = i / gridCols;
                    const int c = i % gridCols;
                    const int x = c * (cellW + safeSpacing);
                    const int y = r * (cellH + safeSpacing);
                    if (x < 0 || y < 0 || x + cellW > width || y + cellH > height) continue;

                    cv::Mat resized;
                    cv::resize(mats[i], resized, cv::Size(cellW, cellH), 0, 0, cv::INTER_LINEAR);
                    resized.copyTo(canvas(cv::Rect(x, y, cellW, cellH)));
                }

                m_image = canvas.clone();
            });

            QObject::disconnect(m_watcher, nullptr, nullptr, nullptr);
            QObject::connect(m_watcher, &QFutureWatcher<void>::finished, this, [this]() {
                QImage preview = ImageData(m_image).image();
                // const QSize previewSize = widget->display->size().isEmpty() ? QSize(160, 90) : widget->display->size();
                // widget->display->setPixmap(QPixmap::fromImage(preview.scaled(previewSize, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
                Q_EMIT dataUpdated(0);
            });
            m_watcher->setFuture(future);
        }
    }

        /**
         * @brief 当界面参数变化时同步更新内部参数与输出图像
         */
        void onInputChanged(){
            // 解析输入框内容

            const int w = widget->widthEdit->value();
            const int h = widget->heightEdit->value();
            if ( w > 0) m_width = w;
            if ( h > 0) m_height = h;
            m_color = QColor(colorEditorWidget->getColor());
            updateImage();
            Q_EMIT dataUpdated(0);
        }
    private:
        ImageLayoutInterface *widget = new ImageLayoutInterface();
        std::unordered_map<int, std::shared_ptr<ImageData>> m_inImages;
        int m_width = 100;
        int m_height = 100;
        ColorEditorWidget *colorEditorWidget=new ColorEditorWidget();
        QColor m_color = QColor(0,0,0,255);
        cv::Mat m_image=cv::Mat(100, 100, CV_8UC4, cv::Scalar(0, 0, 0, 255));;
        QFutureWatcher<void> *m_watcher = nullptr;
        // OpenGL 资源
        struct TexInfo {
            GLuint id = 0;
            int w = 0;
            int h = 0;
        };
        std::unordered_map<int, TexInfo> m_texInfos;
        QOpenGLContext* m_glContext = nullptr;
        QOffscreenSurface m_surface;
        QOpenGLFramebufferObject* m_fbo = nullptr;
        QOpenGLShaderProgram m_shader;
        bool m_glReady = false;

        /**
         * @brief 初始化或校验 OpenGL 上下文与着色器
         */
        bool ensureGL(){
            if (m_glReady) return true;
            releaseGL();
            // 指定更明确的Surface与上下文格式以提升兼容性与性能
            QSurfaceFormat fmt;
            fmt.setRenderableType(QSurfaceFormat::OpenGL);
            fmt.setVersion(3, 3);
            fmt.setProfile(QSurfaceFormat::CompatibilityProfile);
            fmt.setRedBufferSize(8);
            fmt.setGreenBufferSize(8);
            fmt.setBlueBufferSize(8);
            fmt.setAlphaBufferSize(8);
            fmt.setSwapBehavior(QSurfaceFormat::SingleBuffer);
            m_glContext = new QOpenGLContext();
            m_glContext->setFormat(fmt);
            if (!m_glContext->create()) {
                releaseGL();
                return false;
            }
            m_surface.setFormat(fmt);
            m_surface.create();
            if (!m_glContext->makeCurrent(&m_surface)) {
                releaseGL();
                return false;
            }
            // 顶点/片元着色器
            const char *vs = R"(
                attribute vec2 vertex;
                attribute vec2 texCoord;
                uniform mat4 projection;
                uniform mat4 modelView;
                varying vec2 texCoordVarying;
                void main() {
                    gl_Position = projection * modelView * vec4(vertex, 0.0, 1.0);
                    texCoordVarying = texCoord;
                }
            )";
            const char *fs = R"(
                uniform sampler2D texture;
                varying vec2 texCoordVarying;
                void main() {
                    gl_FragColor = texture2D(texture, texCoordVarying);
                }
            )";
            if (!m_shader.addShaderFromSourceCode(QOpenGLShader::Vertex, vs)) { m_glContext->doneCurrent(); releaseGL(); return false; }
            if (!m_shader.addShaderFromSourceCode(QOpenGLShader::Fragment, fs)) { m_glContext->doneCurrent(); releaseGL(); return false; }
            if (!m_shader.link()) { m_glContext->doneCurrent(); releaseGL(); return false; }
            m_glContext->doneCurrent();
            m_glReady = true;
            return true;
        }

        /**
         * @brief 使用 OpenGL 加速合成图像并读回到 cv::Mat
         */
        bool composeWithOpenGL(int width, int height, const QColor& color, int mode, int rows, int cols, int spacing,
                               const std::unordered_map<int, std::shared_ptr<ImageData>>& inputs){
            if (!ensureGL()) return false;
            if (!m_glContext->makeCurrent(&m_surface)) return false;
            // FBO
            if (!m_fbo || m_fbo->size() != QSize(width, height)) {
                delete m_fbo;
                m_fbo = new QOpenGLFramebufferObject(QSize(width, height));
            }
            m_fbo->bind();
            QOpenGLFunctions *f = m_glContext->functions();
            f->glViewport(0, 0, width, height);
            f->glClearColor(color.redF(), color.greenF(), color.blueF(), color.alphaF());
            f->glClear(GL_COLOR_BUFFER_BIT);
            f->glEnable(GL_BLEND);
            f->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            // 收集输入并转换到 BGRA（尽量避免深拷贝）
            std::vector<cv::Mat> mats;
            mats.reserve(inputs.size());
            for (int i = 0; i < InPortCount; ++i) {
                auto it = inputs.find(i);
                if (it == inputs.end() || !it->second) continue;
                const cv::Mat& src = it->second->mat();
                if (src.empty()) continue;
                cv::Mat bgra;
                if (src.channels() == 4) {
                    if (src.type() == CV_8UC4) bgra = src;
                    else src.convertTo(bgra, CV_8UC4);
                } else if (src.channels() == 3) {
                    cv::cvtColor(src, bgra, cv::COLOR_BGR2BGRA);
                } else if (src.channels() == 1) {
                    cv::cvtColor(src, bgra, cv::COLOR_GRAY2BGRA);
                } else {
                    continue;
                }
                if (!bgra.isContinuous() || bgra.step != static_cast<size_t>(bgra.cols * 4)) {
                    bgra = bgra.clone();
                }
                mats.push_back(bgra);
            }

            const int n = static_cast<int>(mats.size());
            if (n == 0) {
                // 直接输出纯背景
                QImage glImage = m_fbo->toImage().convertToFormat(QImage::Format_ARGB32);
                m_image = cv::Mat(glImage.height(), glImage.width(), CV_8UC4, const_cast<uchar*>(glImage.bits()), glImage.bytesPerLine()).clone();
                m_fbo->release();
                m_glContext->doneCurrent();
                return true;
            }

            int gridRows = 1, gridCols = 1;
            if (mode == 0) { gridRows = 1; gridCols = n; }
            else if (mode == 1) { gridRows = n; gridCols = 1; }
            else {
                gridRows = std::max(1, rows);
                gridCols = std::max(1, cols);
                const int needRows = (n + gridCols - 1) / gridCols;
                if (needRows > gridRows) gridRows = needRows;
            }
            const int safeSpacing = std::max(0, spacing);
            const int cellW = std::max(1, (width - safeSpacing * (gridCols - 1)) / gridCols);
            const int cellH = std::max(1, (height - safeSpacing * (gridRows - 1)) / gridRows);

            // 统一投影
            QMatrix4x4 projection;
            projection.ortho(0, width, height, 0, -1, 1);

            static const GLfloat vertices[] = {
                0.0f, 0.0f,
                1.0f, 0.0f,
                0.0f, 1.0f,
                1.0f, 1.0f
            };
            static const GLfloat texCoords[] = {
                0.0f, 0.0f,
                1.0f, 0.0f,
                0.0f, 1.0f,
                1.0f, 1.0f
            };

            // 跟踪本帧使用到的端口索引，以便回收未使用纹理
            std::unordered_set<int> usedIndices;
            usedIndices.reserve(n);
            for (int i = 0; i < n && i < gridRows * gridCols; ++i) {
                const int r = i / gridCols;
                const int c = i % gridCols;
                const int x = c * (cellW + safeSpacing);
                const int y = r * (cellH + safeSpacing);
                if (x < 0 || y < 0 || x + cellW > width || y + cellH > height) continue;

                const cv::Mat& bgra = mats[i];
                auto& tex = m_texInfos[i];
                if (tex.id == 0) {
                    f->glGenTextures(1, &tex.id);
                    tex.w = 0;
                    tex.h = 0;
                }
                f->glActiveTexture(GL_TEXTURE0);
                f->glBindTexture(GL_TEXTURE_2D, tex.id);
                f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                f->glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                if (tex.w != bgra.cols || tex.h != bgra.rows) {
                    tex.w = bgra.cols;
                    tex.h = bgra.rows;
                    f->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.w, tex.h, 0, GL_BGRA, GL_UNSIGNED_BYTE, bgra.data);
                } else {
                    f->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tex.w, tex.h, GL_BGRA, GL_UNSIGNED_BYTE, bgra.data);
                }

                if (!m_shader.bind()) continue;
                m_shader.setUniformValue("projection", projection);
                QMatrix4x4 modelView;
                modelView.translate(x, y);
                modelView.scale(cellW, cellH);
                m_shader.setUniformValue("modelView", modelView);
                m_shader.setUniformValue("texture", 0);
                m_shader.enableAttributeArray("vertex");
                m_shader.enableAttributeArray("texCoord");
                m_shader.setAttributeArray("vertex", vertices, 2);
                m_shader.setAttributeArray("texCoord", texCoords, 2);
                f->glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                m_shader.disableAttributeArray("vertex");
                m_shader.disableAttributeArray("texCoord");
                f->glBindTexture(GL_TEXTURE_2D, 0);
                m_shader.release();
                usedIndices.insert(i);
            }

            // 回收未使用的纹理资源，避免内存泄漏与显存占用增长
            if (!m_texInfos.empty()) {
                std::vector<int> toErase;
                toErase.reserve(m_texInfos.size());
                for (const auto& kv : m_texInfos) {
                    if (usedIndices.find(kv.first) == usedIndices.end() && kv.second.id != 0) {
                        GLuint id = kv.second.id;
                        f->glDeleteTextures(1, &id);
                        toErase.push_back(kv.first);
                    }
                }
                for (int idx : toErase) {
                    m_texInfos.erase(idx);
                }
            }

            // 读回
            QImage glImage = m_fbo->toImage().convertToFormat(QImage::Format_ARGB32);
            m_fbo->release();
            m_glContext->doneCurrent();

            m_image = cv::Mat(glImage.height(), glImage.width(), CV_8UC4, const_cast<uchar*>(glImage.bits()), glImage.bytesPerLine()).clone();
            return true;
        }

        /**
         * @brief 释放 OpenGL 相关资源
         */
        void releaseGL(){
            if (m_glContext && m_glContext->makeCurrent(&m_surface)) {
                QOpenGLFunctions* f = m_glContext->functions();
                for (auto& kv : m_texInfos) {
                    if (kv.second.id != 0) {
                        GLuint id = kv.second.id;
                        f->glDeleteTextures(1, &id);
                        kv.second.id = 0;
                        kv.second.w = 0;
                        kv.second.h = 0;
                    }
                }
                m_shader.removeAllShaders();
                delete m_fbo;
                m_fbo = nullptr;
                m_glContext->doneCurrent();
            }
            m_surface.destroy();
            delete m_glContext;
            m_glContext = nullptr;
            m_texInfos.clear();
            m_glReady = false;
        }
    };
}
