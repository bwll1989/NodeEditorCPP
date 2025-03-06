#ifndef IMAGECLIPMODEL_H
#define IMAGECLIPMODEL_H

#include "Widget/TimeLineWidget/TimelineAbstract/AbstractClipModel.hpp"
#include <QImage>
#include <QPainter>
#include <QFont>
#include <QDebug>
#include <QOpenGLTexture>
#include <QOpenGLContext>
#include <QOffscreenSurface>
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

class ImageClipModel : public AbstractClipModel {
    Q_OBJECT
public:
    explicit ImageClipModel(int start, int end, const QString& filePath = QString(), QObject* parent = nullptr)
        : AbstractClipModel(start, end, "Image", TimecodeType::PAL, parent), 
          m_filePath(filePath), 
          m_frameRate(25.0),
          m_width(0), 
          m_height(0),
          m_image(nullptr){
             // 初始化OpenGL上下文
        m_glContext = new QOpenGLContext();
        m_glContext->create();

        // 创建离屏surface
        m_surface.create();
        
        // 确保OpenGL上下文是当前的
        m_glContext->makeCurrent(&m_surface);

          }

    ~ImageClipModel() override {
        if (m_texture) {
            m_texture->destroy();
            delete m_texture;
        }
    };

    // 设置文件路径并加载视频信息
    void setFilePath(const QString& path) { 
        if (m_filePath != path) {
            m_filePath = path;
            loadVideoInfo(path);
            emit filePathChanged(path);
        }
    }

    // 其他 getter/setter 保持不变
    QString filePath() const { return m_filePath; }
   

    // 重写保存和加载函数
    QJsonObject save() const override {
        QJsonObject json = AbstractClipModel::save();
        json["filePath"] = m_filePath;
        json["width"] = m_width;
        json["height"] = m_height;
        json["posX"] = m_PosX;
        json["posY"] = m_PosY;
        return json;
    }

    void load(const QJsonObject& json) override {
        AbstractClipModel::load(json);
        m_filePath = json["filePath"].toString();
        m_width = json["width"].toInt();
        m_height = json["height"].toInt();
        m_PosX = json["posX"].toInt();
        m_PosY = json["posY"].toInt();
        m_image = new QImage(m_filePath);
    }

    QVariant data(int role) const override {
        switch (role) {
            case Qt::DisplayRole:
                return m_filePath;
            case TimelineRoles::ClipModelRole:
                return QVariant::fromValue(static_cast<AbstractClipModel*>(const_cast<ImageClipModel*>(this)));
            default:
                return AbstractClipModel::data(role);
        }
    }

    QVariantMap currentVideoData(int currentFrame) const override {
        QVariantMap data;
        data["image"] = QVariant::fromValue(*m_image);
        data["texture"] = QVariant::fromValue(m_texture);
        data["posX"] = m_PosX;
        data["posY"] = m_PosY;
        data["width"] = m_width;
        data["height"] = m_height;
        return data;
    }

    // 获取帧率
    double getFrameRate() const { return m_frameRate; }
    /**
     * 获取视频宽度
     */
    int getWidth() const { return m_width; }
    /**
     * 获取视频高度
     */
    int getHeight() const { return m_height; }
    /**
     * 获取视频水平位置
     */
    int getPosX() const { return m_PosX; }
    /**
     * 获取视频垂直位置
     */
    int getPosY() const { return m_PosY; }
public Q_SLOTS:
    /**
     * 设置视频位置
     */

    /**
     * 设置视频显示位置
     */
    void setPos(int x,int y) {
        m_PosX=x;
        m_PosY = y;
        emit posChanged(QPoint(m_PosX,m_PosY));
    }
    /**
     * 设置视频显示尺寸
     */
    void setSize(int width,int height){
        m_width = width;
        m_height=height;
        emit sizeChanged(QSize(m_width,m_height));
    }
private:
    void loadVideoInfo(const QString& path) {
        m_image=new QImage(path);
        if (!m_image->isNull()) {
            if (m_texture) {
                m_texture->destroy();
                delete m_texture;
            }
            // 创建纹理前检查图像是否有效
            if (m_image && !m_image->isNull()) {
              
                if (!m_glContext) {
                    qDebug() << "No valid OpenGL context found";
                    return;
                }
              
                m_texture = new QOpenGLTexture(*m_image);
                if (m_texture && m_texture->isCreated()) {
                    // 设置纹理过滤参数
                    m_texture->setMinificationFilter(QOpenGLTexture::Linear);
                    m_texture->setMagnificationFilter(QOpenGLTexture::Linear);
                    m_texture->setWrapMode(QOpenGLTexture::ClampToEdge);
                } else {
                    qDebug() << "Failed to create texture";
                    delete m_texture;
                    m_texture = nullptr;
                }
            } else {
                qDebug() << "Invalid image for texture creation";
            }
            qDebug()<<m_texture->isCreated();
            setSize(m_image->size().width(),m_image->size().height());
          
        }
    }

    QString m_filePath;
    double m_frameRate;
    int m_width;
    int m_height;
    int m_PosX;
    int m_PosY;
    QImage* m_image;
    QOpenGLTexture* m_texture;
    QOpenGLContext* m_glContext;
    QOffscreenSurface m_surface;
};

#endif // IMAGECLIPMODEL_H 