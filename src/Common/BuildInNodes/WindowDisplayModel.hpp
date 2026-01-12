#pragma once
#include <QtNodes/NodeDelegateModelRegistry>
#include "DataTypes/NodeDataList.hpp"
#include "AbstractDelegateModel.h"
#include <QOpenGLWindow>
#include <QOpenGLFunctions>
#include <QComboBox>
#include <QPushButton>
#include <QGridLayout>
#include <QScreen>
#include <QGuiApplication>
#include <QPainter>
#include <QKeyEvent>
#include <functional>
using namespace NodeDataTypes;
namespace Nodes
{
    class WindowDisplayModel final : public AbstractDelegateModel
    {
        Q_OBJECT

    public:
        /**
         * @brief 构造函数
         */
        WindowDisplayModel();

        ~WindowDisplayModel() override;

        /**
         * @brief 获取节点端口数据类型
         */
        QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

        /**
         * @brief 输出当前图像数据
         */
        std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex port) override;

        /**
         * @brief 设置输入图像并更新界面
         */
        void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex port) override;

        QWidget *embeddedWidget() override { return _panel; }

        bool resizable() const override { return true; }

        QJsonObject save() const override;

        void load(const QJsonObject &p) override;

    private:
        /**
         * @brief 简单OpenGL窗口用于显示图像
         */
        class ImageOpenGLWindow : public QOpenGLWindow, protected QOpenGLFunctions {
        public:
            /**
             * @brief 构造OpenGL窗口，设置无边框与置顶
             */
            ImageOpenGLWindow() : QOpenGLWindow(NoPartialUpdate) {
                setFlags(flags() | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
            }
            /**
             * @brief 设置当前帧图像并请求重绘
             */
            void setFrame(const QImage& img) {
                m_frame = img.convertToFormat(QImage::Format_RGBA8888);
                update();
            }
            /**
             * @brief ESC 键关闭窗口
             */
            void keyPressEvent(QKeyEvent *e) override {
                if (e && e->key() == Qt::Key_Escape) {
                    close();
                    e->accept();
                    return;
                }
                QOpenGLWindow::keyPressEvent(e);
            }
        protected:
            /**
             * @brief OpenGL初始化
             */
            void initializeGL() override {
                initializeOpenGLFunctions();
                glDisable(GL_DEPTH_TEST);
                glDisable(GL_BLEND);
            }
            /**
             * @brief 调整视口
             */
            void resizeGL(int w, int h) override {
                glViewport(0, 0, w, h);
            }
            /**
             * @brief 使用QPainter在OpenGL后端绘制图像
             */
            void paintGL() override {
                glClearColor(0.f, 0.f, 0.f, 1.f);
                glClear(GL_COLOR_BUFFER_BIT);
                if (m_frame.isNull()) return;
                QPainter p(this);
                p.setRenderHint(QPainter::SmoothPixmapTransform, true);
                // 保持纵横比，居中显示
                const QSizeF wndSize(width(), height());
                const QSizeF imgSize(m_frame.width(), m_frame.height());
                const qreal wndRatio = wndSize.width() / wndSize.height();
                const qreal imgRatio = imgSize.width() / imgSize.height();
                QRectF target;
                if (imgRatio > wndRatio) {
                    qreal tw = wndSize.width();
                    qreal th = tw / imgRatio;
                    target = QRectF(0, (wndSize.height() - th) / 2.0, tw, th);
                } else {
                    qreal th = wndSize.height();
                    qreal tw = th * imgRatio;
                    target = QRectF((wndSize.width() - tw) / 2.0, 0, tw, th);
                }
                p.drawImage(target, m_frame);
            }
            /**
             * @brief 捕获窗口隐藏/关闭事件以同步外部按钮状态
             */
            bool event(QEvent* e) override {
                if (e) {
                    if (e->type() == QEvent::Hide || e->type() == QEvent::Close) {
                        if (onClosed) onClosed();
                    }
                }
                return QOpenGLWindow::event(e);
            }
        private:
            QImage m_frame;
        public:
            std::function<void()> onClosed;
        };

        /**
         * @brief 刷新显示器列表
         */
        void refreshScreens();
        /**
         * @brief 切换显示器
         */
        void onScreenChanged(int index);
        /**
         * @brief 打开或关闭显示窗口
         */
        void toggleWindow();

        /**
         * @brief 设置窗口可见性
         */
        void setWindowVisible(bool visible);

        QWidget *_panel = nullptr;
        QGridLayout *_layout = nullptr;
        QComboBox * _screenCombo = nullptr;
        QPushButton * _openBtn = nullptr;
        ImageOpenGLWindow * _glWindow = nullptr;
        int _currentScreenIndex = 0;

        std::shared_ptr<ImageData> m_outData;
    };
}
