#pragma once

#include <atomic>
#include <cmath>
#include <memory>

#include <QJsonObject>
#include <QPointer>
#include <QRectF>
#include <QtNodes/NodeDelegateModel>

#include "Common/BaseClass/AbstractDelegateModel.h"
#include "Common/DataTypes/ImageGpuUpload.h"
#include "Common/DataTypes/ImageReadback.h"
#include "NodeDataList.hpp"
#include "ROICommon.hpp"
#include "ROIGpu.hpp"
#include "ROIInterface.hpp"
#include "TimestampGenerator/TimestampGenerator.hpp"

using namespace NodeDataTypes;

namespace Nodes
{
/**
 * @brief ROI 节点数据模型
 *
 * 输入一张图像，在内嵌界面中通过鼠标拖拽选择感兴趣区域，
 * 输出对应的裁剪图像。
 */
class ROIDataModel final : public AbstractDelegateModel
{
    Q_OBJECT
    Q_PROPERTY(double roiXNorm READ roiXNorm WRITE setRoiXNorm NOTIFY roiXNormChanged)
    Q_PROPERTY(double roiYNorm READ roiYNorm WRITE setRoiYNorm NOTIFY roiYNormChanged)
    Q_PROPERTY(double roiWidthNorm READ roiWidthNorm WRITE setRoiWidthNorm NOTIFY roiWidthNormChanged)
    Q_PROPERTY(double roiHeightNorm READ roiHeightNorm WRITE setRoiHeightNorm NOTIFY roiHeightNormChanged)

public:
    /**
     * @brief 构造函数
     *
     * 初始化端口数量、标题和内嵌界面，并建立鼠标框选到属性更新的连接。
     */
    ROIDataModel()
    {
        InPortCount = 2;
        OutPortCount = 2;
        CaptionVisible = true;
        Caption = "ROI";
        WidgetEmbeddable = true;
        Resizable = true;
        PortEditable = false;

        m_widget = new ROIInterface();
        m_outputRectData = std::make_shared<VariableData>(QRect());
        m_emptyImage = std::make_shared<ImageData>();
        m_emptyVariable = std::make_shared<VariableData>();
        ensureImageDataBuffer(m_outputImage, m_outputBuffer);

        connect(m_widget->imageView,
                &ROIImageView::roiRectChanged,
                this,
                &ROIDataModel::onWidgetRoiRectChanged);
    }

    ~ROIDataModel() override
    {
        m_shuttingDown.store(true);
        disconnect(TimestampGenerator::getInstance(), nullptr, this, nullptr);

        if (m_widget) {
            m_widget->disconnect(this);
            if (m_widget->imageView) {
                m_widget->imageView->disconnect(this);
            }
            if (m_widget->parentWidget() == nullptr) {
                delete m_widget.data();
            }
            m_widget = nullptr;
        }

        if (m_outputBuffer) {
            m_outputBuffer->setActive(false);
            m_outputBuffer->clear();
        }
        m_inputImage.reset();
    }

    void afterModelReady() override
    {
        AbstractDelegateModel::afterModelReady();
        ImageGpuUpload::instance().warmup();

        connect(TimestampGenerator::getInstance(),
                &TimestampGenerator::frameCountUpdated,
                this,
                [self = QPointer<ROIDataModel>(this)](qint64 frameCount) {
                    if (!self || self->m_shuttingDown.load()) {
                        return;
                    }
                    self->onSystemFrameTick(frameCount);
                },
                Qt::QueuedConnection);

        if (m_inputImage) {
            updateFromSharedBuffer(TimestampGenerator::getInstance()->getCurrentFrameCount());
        }
    }

    /**
     * @brief 返回端口类型
     * @param portType 端口方向
     * @param portIndex 端口索引
     * @return 节点端口的数据类型
     */
    QtNodes::NodeDataType dataType(QtNodes::PortType portType,
                                   QtNodes::PortIndex portIndex) const override
    {
        if (portType == QtNodes::PortType::In) {
            if (portIndex == 0) {
                return ImageData().type();
            }
            return VariableData().type();
        }
        if (portType == QtNodes::PortType::Out) {
            if (portIndex == 0) {
                return ImageData().type();
            }
            return VariableData().type();
        }
        return VariableData().type();
    }

    /**
     * @brief 返回端口标题
     * @param portType 端口方向
     * @param portIndex 端口索引
     * @return 端口标题文本
     */
    QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
    {
        switch (portType) {
        case QtNodes::PortType::In:
            switch (portIndex) {
            case 0: return "IMAGE";
            case 1: return "RECT";
            default: return QString();
            }
        case QtNodes::PortType::Out:
            switch (portIndex) {
            case 0: return "IMAGE";
            case 1: return "RECT";
            default: return QString();
            }
        default:
            return QString();
        }
    }

    /**
     * @brief 设置输入数据
     * @param nodeData 输入节点数据
     * @param portIndex 端口索引
     *
     * 接收新的图像后会刷新预览界面，并根据当前 ROI 重新生成输出图像。
     */
    void setInData(std::shared_ptr<QtNodes::NodeData> nodeData,
                   QtNodes::PortIndex portIndex) override
    {
        switch (portIndex) {
        case 0: {
            m_inputImage = std::dynamic_pointer_cast<ImageData>(nodeData);
            if (!m_inputImage) {
                m_inputImageSize = QSize();
                if (m_outputBuffer) {
                    m_outputBuffer->clear();
                }
                m_lastPushedTimestamp = -1;
                m_roiRectPx = QRect();
                if (m_outputRectData) {
                    m_outputRectData->insert("default", m_roiRectPx);
                }
                if (m_widget && m_widget->imageView) {
                    m_widget->imageView->setImage(cv::Mat());
                    m_widget->setRoiInfo(QRect(), QSize());
                }
                emit dataUpdated(0);
                emit dataUpdated(1);
                return;
            }

            ensureImageDataBuffer(m_outputImage, m_outputBuffer);
            m_lastProcessedInputTimestamp = -1;
            emit dataUpdated(0);

            updateFromSharedBuffer(TimestampGenerator::getInstance()->getCurrentFrameCount());

            if (m_hasPendingRectInput) {
                applyRectInput(m_pendingRectInput, m_pendingRectInputIsNorm);
                m_hasPendingRectInput = false;
            }
            return;
        }
        case 1: {
            if (!nodeData) {
                return;
            }
            const auto varData = std::dynamic_pointer_cast<VariableData>(nodeData);
            if (!varData) {
                return;
            }

            const QRectF rectF = varData->value().toRectF();
            if (!rectF.isValid() || rectF.width() <= 0.0 || rectF.height() <= 0.0) {
                return;
            }

            const bool isNorm = isNormalizedRect(rectF);
            if (m_inputImageSize.isEmpty()) {
                m_pendingRectInput = rectF;
                m_pendingRectInputIsNorm = isNorm;
                m_hasPendingRectInput = true;
                return;
            }

            applyRectInput(rectF, isNorm);
            return;
        }
        default:
            return;
        }
    }

    /**
     * @brief 返回输出数据
     * @param portIndex 输出端口索引
     * @return 当前裁剪后的 ROI 图像
     */
    std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex portIndex) override
    {
        switch (portIndex) {
        case 0:
            return m_outputImage ? m_outputImage : m_emptyImage;
        case 1:
            return m_outputRectData ? m_outputRectData : m_emptyVariable;
        default:
            return m_emptyVariable;
        }
    }

    /**
     * @brief 返回内嵌控件
     * @return ROI 交互界面
     */
    QWidget* embeddedWidget() override
    {
        return m_widget;
    }

    /**
     * @brief 保存节点配置
     * @return 包含 ROI 参数的 JSON 对象
     */
    QJsonObject save() const override
    {
        QJsonObject values;
        values["roiXNorm"] = m_roiNormRect.x();
        values["roiYNorm"] = m_roiNormRect.y();
        values["roiWidthNorm"] = m_roiNormRect.width();
        values["roiHeightNorm"] = m_roiNormRect.height();

        QJsonObject modelJson = NodeDelegateModel::save();
        modelJson["values"] = values;
        return modelJson;
    }

    /**
     * @brief 恢复节点配置
     * @param data 序列化数据
     */
    void load(const QJsonObject& data) override
    {
        const QJsonObject values = data["values"].toObject();
        setRoiNormRect(QRectF(values.value("roiXNorm").toDouble(),
                              values.value("roiYNorm").toDouble(),
                              values.value("roiWidthNorm").toDouble(),
                              values.value("roiHeightNorm").toDouble()));
    }

    /**
     * @brief 获取 ROI 左上角 X 的归一化坐标
     * @return 归一化 X，范围 [0,1]
     */
    double roiXNorm() const
    {
        return m_roiNormRect.x();
    }

    /**
     * @brief 获取 ROI 左上角 Y 的归一化坐标
     * @return 归一化 Y，范围 [0,1]
     */
    double roiYNorm() const
    {
        return m_roiNormRect.y();
    }

    /**
     * @brief 获取 ROI 宽度的归一化值
     * @return 归一化宽度，范围 [0,1]
     */
    double roiWidthNorm() const
    {
        return m_roiNormRect.width();
    }

    /**
     * @brief 获取 ROI 高度的归一化值
     * @return 归一化高度，范围 [0,1]
     */
    double roiHeightNorm() const
    {
        return m_roiNormRect.height();
    }

signals:
    /**
     * @brief ROI X 归一化坐标变化信号
     * @param value 新的归一化 X
     */
    void roiXNormChanged(double value);

    /**
     * @brief ROI Y 归一化坐标变化信号
     * @param value 新的归一化 Y
     */
    void roiYNormChanged(double value);

    /**
     * @brief ROI 宽度归一化变化信号
     * @param value 新的归一化宽度
     */
    void roiWidthNormChanged(double value);

    /**
     * @brief ROI 高度归一化变化信号
     * @param value 新的归一化高度
     */
    void roiHeightNormChanged(double value);

private slots:
    /**
     * @brief 处理界面鼠标框选产生的新 ROI
     * @param rect 新的 ROI 矩形
     */
    void onWidgetRoiRectChanged(const QRect& rect)
    {
        setRoiRectPx(rect);
    }

    /**
     * @brief 跟随全局时钟从共享输入缓冲拉取预览帧
     */
    void onSystemFrameTick(qint64 frameCount)
    {
        if (m_shuttingDown.load() || !m_inputImage) {
            return;
        }
        updateFromSharedBuffer(frameCount);
    }

private:
    bool isNormalizedRect(const QRectF& rectF) const
    {
        const double eps = 1e-6;
        if (rectF.x() < -eps || rectF.y() < -eps) return false;
        if (rectF.width() <= eps || rectF.height() <= eps) return false;
        if (rectF.x() > 1.0 + eps || rectF.y() > 1.0 + eps) return false;
        if (rectF.width() > 1.0 + eps || rectF.height() > 1.0 + eps) return false;
        if (rectF.x() + rectF.width() > 1.0 + eps) return false;
        if (rectF.y() + rectF.height() > 1.0 + eps) return false;
        return true;
    }

    void applyRectInput(const QRectF& rectF, bool isNorm)
    {
        if (isNorm) {
            setRoiNormRect(rectF);
            return;
        }

        const int x = qMax(0, static_cast<int>(std::floor(rectF.x())));
        const int y = qMax(0, static_cast<int>(std::floor(rectF.y())));
        const int w = qMax(1, static_cast<int>(std::ceil(rectF.width())));
        const int h = qMax(1, static_cast<int>(std::ceil(rectF.height())));
        setRoiRectPx(QRect(x, y, w, h));
    }

    /**
     * @brief 设置完整 ROI 矩形（像素坐标）
     * @param rectPx 新的 ROI 像素矩形
     *
     * 将像素矩形转换为归一化矩形作为单一真相保存，确保分辨率变化时 ROI 仍然保持相同区域比例。
     */
    void setRoiRectPx(const QRect& rectPx)
    {
        const QRect clampedPx = clampRectToImagePx(normalizeRectPx(rectPx));
        const QRectF norm = normRectFromPx(clampedPx, m_inputImageSize);
        setRoiNormRect(norm);
    }

    /**
     * @brief 设置完整 ROI 矩形（归一化坐标）
     * @param rectNorm 归一化 ROI 矩形
     *
     * 归一化坐标是本节点的“单一真相”。当输入图像分辨率变化时，会重新派生像素 ROI。
     */
    void setRoiNormRect(const QRectF& rectNorm)
    {
        const QRectF normalizedNorm = normalizeRectNorm(rectNorm);
        if (normalizedNorm == m_roiNormRect) {
            const QRect oldPx = m_roiRectPx;
            updateDerivedRoiPxFromNorm();
            if (oldPx != m_roiRectPx && m_outputRectData) {
                m_outputRectData->insert("default", m_roiRectPx);
                emit dataUpdated(1);
            }
            syncWidgetState();
            m_roiParamsDirty = true;
            processImage();
            return;
        }

        const QRectF oldNorm = m_roiNormRect;
        const QRect oldPx = m_roiRectPx;
        m_roiNormRect = normalizedNorm;
        updateDerivedRoiPxFromNorm();
        if (oldPx != m_roiRectPx && m_outputRectData) {
            m_outputRectData->insert("default", m_roiRectPx);
            emit dataUpdated(1);
        }

        if (!qFuzzyCompare(oldNorm.x(), m_roiNormRect.x())) emit roiXNormChanged(m_roiNormRect.x());
        if (!qFuzzyCompare(oldNorm.y(), m_roiNormRect.y())) emit roiYNormChanged(m_roiNormRect.y());
        if (!qFuzzyCompare(oldNorm.width(), m_roiNormRect.width())) emit roiWidthNormChanged(m_roiNormRect.width());
        if (!qFuzzyCompare(oldNorm.height(), m_roiNormRect.height())) emit roiHeightNormChanged(m_roiNormRect.height());

        syncWidgetState();
        m_roiParamsDirty = true;
        processImage();
    }

    /**
     * @brief 设置 ROI 左上角 X 的归一化坐标
     * @param value 归一化 X
     */
    void setRoiXNorm(double value)
    {
        setRoiNormRect(QRectF(value, m_roiNormRect.y(), m_roiNormRect.width(), m_roiNormRect.height()));
    }

    /**
     * @brief 设置 ROI 左上角 Y 的归一化坐标
     * @param value 归一化 Y
     */
    void setRoiYNorm(double value)
    {
        setRoiNormRect(QRectF(m_roiNormRect.x(), value, m_roiNormRect.width(), m_roiNormRect.height()));
    }

    /**
     * @brief 设置 ROI 宽度的归一化值
     * @param value 归一化宽度
     */
    void setRoiWidthNorm(double value)
    {
        setRoiNormRect(QRectF(m_roiNormRect.x(), m_roiNormRect.y(), value, m_roiNormRect.height()));
    }

    /**
     * @brief 设置 ROI 高度的归一化值
     * @param value 归一化高度
     */
    void setRoiHeightNorm(double value)
    {
        setRoiNormRect(QRectF(m_roiNormRect.x(), m_roiNormRect.y(), m_roiNormRect.width(), value));
    }

    /**
     * @brief 将像素 ROI 规范化为正向矩形
     * @param rectPx 输入像素矩形
     * @return 规范化后的像素矩形
     */
    QRect normalizeRectPx(const QRect& rectPx) const
    {
        QRect normalized = rectPx.normalized();
        if (normalized.width() <= 0 || normalized.height() <= 0) {
            return {};
        }
        return normalized;
    }

    /**
     * @brief 将归一化 ROI 规范化并限制在 [0,1] 范围内
     * @param rectNorm 输入归一化矩形
     * @return 合法的归一化矩形
     */
    QRectF normalizeRectNorm(const QRectF& rectNorm) const
    {
        if (rectNorm.width() <= 0.0 || rectNorm.height() <= 0.0) {
            return {};
        }
        const double x = qBound(0.0, rectNorm.x(), 1.0);
        const double y = qBound(0.0, rectNorm.y(), 1.0);
        const double w = qBound(0.0, rectNorm.width(), 1.0);
        const double h = qBound(0.0, rectNorm.height(), 1.0);

        if (w <= 0.0 || h <= 0.0) {
            return {};
        }

        const double maxW = qMax(0.0, 1.0 - x);
        const double maxH = qMax(0.0, 1.0 - y);
        return QRectF(x, y, qMin(w, maxW), qMin(h, maxH));
    }

    /**
     * @brief 将 ROI 限制到当前输入图像范围（像素坐标）
     * @param rectPx 输入像素矩形
     * @return 合法范围内的像素矩形
     */
    QRect clampRectToImagePx(const QRect& rectPx) const
    {
        if (!rectPx.isValid() || m_inputImageSize.isEmpty()) {
            return rectPx.isValid() ? rectPx : QRect();
        }
        const QRect imageBounds(0, 0, m_inputImageSize.width(), m_inputImageSize.height());
        return rectPx.intersected(imageBounds);
    }

    /**
     * @brief 将像素矩形转换为归一化矩形
     * @param rectPx 像素矩形
     * @param imageSize 图像尺寸
     * @return 归一化矩形
     */
    QRectF normRectFromPx(const QRect& rectPx, const QSize& imageSize) const
    {
        if (!rectPx.isValid() || imageSize.isEmpty()) {
            return {};
        }
        const double w = static_cast<double>(imageSize.width());
        const double h = static_cast<double>(imageSize.height());
        return normalizeRectNorm(QRectF(rectPx.x() / w,
                                        rectPx.y() / h,
                                        rectPx.width() / w,
                                        rectPx.height() / h));
    }

    /**
     * @brief 根据归一化 ROI 派生当前像素 ROI
     */
    void updateDerivedRoiPxFromNorm()
    {
        if (m_inputImageSize.isEmpty() || !m_roiNormRect.isValid()) {
            m_roiRectPx = QRect();
            return;
        }

        const int imgW = m_inputImageSize.width();
        const int imgH = m_inputImageSize.height();

        const int x = qBound(0, static_cast<int>(std::floor(m_roiNormRect.x() * imgW)), imgW - 1);
        const int y = qBound(0, static_cast<int>(std::floor(m_roiNormRect.y() * imgH)), imgH - 1);
        const int w = qBound(1, static_cast<int>(std::ceil(m_roiNormRect.width() * imgW)), imgW - x);
        const int h = qBound(1, static_cast<int>(std::ceil(m_roiNormRect.height() * imgH)), imgH - y);
        m_roiRectPx = QRect(x, y, w, h);
    }

    /**
     * @brief 同步内嵌界面的图像与 ROI 信息
     */
    void syncWidgetState()
    {
        if (!m_widget || !m_widget->imageView) {
            return;
        }
        m_widget->imageView->setRoiRect(m_roiRectPx);
        m_widget->setRoiInfo(m_roiRectPx, m_inputImageSize);
    }

    void clearOutputBuffer()
    {
        if (m_outputBuffer) {
            m_outputBuffer->clear();
        }
        m_lastPushedTimestamp = -1;
    }

    /**
     * @brief 根据当前 ROI 重新裁剪输出图像
     */
    void processImage()
    {
        ensureImageDataBuffer(m_outputImage, m_outputBuffer);
        const qint64 outputTimestamp = TimestampGenerator::getInstance()->getCurrentFrameCount();

        if (!m_inputImage) {
            clearOutputBuffer();
            return;
        }

        ImageFrame frame;
        if (!ROINode::resolveInputFrame(m_inputImage, outputTimestamp, frame)) {
            clearOutputBuffer();
            return;
        }

        const bool inputUpdated = frame.timestamp != m_lastProcessedInputTimestamp;
        if (!inputUpdated && !m_roiParamsDirty) {
            return;
        }

        processImageFrame(frame, outputTimestamp);
        m_roiParamsDirty = false;
        if (frame.timestamp >= 0) {
            m_lastProcessedInputTimestamp = frame.timestamp;
        }
    }

    void processImageFrame(const ImageFrame& inputFrame, qint64 outputTimestamp)
    {
        if (!m_roiRectPx.isValid() || m_roiRectPx.width() <= 0 || m_roiRectPx.height() <= 0) {
            clearOutputBuffer();
            return;
        }

        const QRect validRoi = clampRectToImagePx(m_roiRectPx);
        if (!validRoi.isValid() || validRoi.width() <= 0 || validRoi.height() <= 0) {
            clearOutputBuffer();
            return;
        }

        ImageFrame frame = inputFrame;
        if (!frame.texture.valid() && !frame.ensureGpuTexture()) {
            const cv::Mat inputMat = ImageReadback::matFromFrame(frame);
            if (inputMat.empty()) {
                clearOutputBuffer();
                return;
            }

            const cv::Rect cvRoi(validRoi.x(), validRoi.y(), validRoi.width(), validRoi.height());
            cv::Mat cropped = inputMat(cvRoi).clone();
            pushFrameToImageBufferDedup(m_outputBuffer, std::move(cropped), outputTimestamp, m_lastPushedTimestamp);
            return;
        }

        auto outTex = ROINode::cropTexture(frame.texture, validRoi);
        if (!outTex.valid()) {
            clearOutputBuffer();
            return;
        }

        ROINode::pushTextureToOutput(m_outputBuffer, m_lastPushedTimestamp, std::move(outTex), outputTimestamp);
    }

    void updateFromSharedBuffer(qint64 frameCount)
    {
        if (m_shuttingDown.load() || !m_inputImage) {
            return;
        }

        ImageFrame frame;
        if (!ROINode::resolveInputFrame(m_inputImage, frameCount, frame)) {
            return;
        }

        const int fw = ROINode::frameWidth(frame);
        const int fh = ROINode::frameHeight(frame);
        if (fw <= 0 || fh <= 0) {
            return;
        }

        const QSize newSize(fw, fh);
        const bool sizeChanged = newSize != m_inputImageSize;
        const bool inputUpdated = frame.timestamp != m_lastProcessedInputTimestamp;

        if (sizeChanged) {
            m_inputImageSize = newSize;
            updateDerivedRoiPxFromNorm();
        }

        if (inputUpdated && m_widget && m_widget->imageView) {
            m_widget->imageView->setImage(ImageReadback::matFromFrame(frame));
        }

        if (inputUpdated || sizeChanged) {
            syncWidgetState();
        }

        if (inputUpdated || m_roiParamsDirty) {
            processImageFrame(frame, frameCount);
            m_roiParamsDirty = false;
            if (frame.timestamp >= 0) {
                m_lastProcessedInputTimestamp = frame.timestamp;
            }
        }
    }

private:
    QPointer<ROIInterface> m_widget;
    std::shared_ptr<ImageData> m_inputImage;
    std::shared_ptr<ImageData> m_outputImage;
    std::shared_ptr<ImageTimestampRingQueue> m_outputBuffer;
    qint64 m_lastPushedTimestamp = -1;
    std::shared_ptr<VariableData> m_outputRectData;
    std::shared_ptr<ImageData> m_emptyImage;
    std::shared_ptr<VariableData> m_emptyVariable;
    QRectF m_roiNormRect;
    QRect m_roiRectPx;
    QSize m_inputImageSize;
    qint64 m_lastProcessedInputTimestamp = -1;
    std::atomic<bool> m_shuttingDown{false};
    bool m_roiParamsDirty = false;

    bool m_hasPendingRectInput = false;
    bool m_pendingRectInputIsNorm = false;
    QRectF m_pendingRectInput;
};
} // namespace Nodes
