#pragma once

#include <QComboBox>

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeDelegateModelRegistry>

#include "Common/AppConfig/ConfigManager.h"
#include "Common/BaseClass/AbstractDelegateModel.h"
#include "NodeDataList.hpp"
#include "Common/DataTypes/ImageReadback.h"
#include "StatusContainer/GlobalEventBus.hpp"
#include "TimestampGenerator/TimestampGenerator.hpp"

#include "Elements/SelectorComboBox/SelectorComboBox.hpp"
#include "MediaLibrary/MediaLibrary.h"

#include <QtCore/QDir>
#include <QtCore/QEvent>
#include <QtWidgets/QFileDialog>

using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using namespace NodeDataTypes;

struct GlobalEvent;
namespace Nodes
{
    // -----------------------------------------------------------------------
    // ImageLoaderModel — 静态图像文件源节点
    //
    // 数据发布策略：
    //   1. 解码 + GPU 上传完成后，向 ImageTimestampRingQueue 写入一帧（publishFrameOnce）。
    //   2. 静态源不随 tick 重复 push，也不 emit dataUpdated 通知下游。
    //   3. 下游显示/处理节点订阅 TimestampGenerator，按系统时钟自行 getLatestFrame。
    // -----------------------------------------------------------------------
    class ImageLoaderModel : public AbstractDelegateModel
    {
        Q_OBJECT
        Q_PROPERTY(QString file READ file WRITE setFile NOTIFY fileChanged)

    public:
        ImageLoaderModel()
        {
            InPortCount = 0;
            OutPortCount = 1;

            CaptionVisible = true;
            Caption = "Image File";
            WidgetEmbeddable = true;
            Resizable = false;
            PortEditable = false;

            // 初始化输出 ImageData 及其共享 ring buffer（默认容量见 ImageData 配置）。
            ensureImageDataBuffer(m_outImageData, m_outputBuffer);

            NodeDelegateModel::ExternalBinding binding;
            binding.member = "file";
            binding.control = _fileSelectComboBox;
            AbstractDelegateModel::registerExternalBinding("/file", this, binding);

            connect(_fileSelectComboBox, &SelectorComboBox::textChanged, this, &ImageLoaderModel::setFile);
            connect(this, &ImageLoaderModel::fileChanged, this, [this](const QString&) {
                {
                    QSignalBlocker blocker(_fileSelectComboBox);
                    _fileSelectComboBox->setText(m_file);
                }
                loadImage(m_file);
            });
        }

        ~ImageLoaderModel() override = default;

        NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            Q_UNUSED(portType);
            Q_UNUSED(portIndex);
            return ImageData().type();
        }

        std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex port) override
        {
            Q_UNUSED(port);
            return m_outImageData;
        }

        void setInData(std::shared_ptr<QtNodes::NodeData>, QtNodes::PortIndex const portIndex) override
        {
            Q_UNUSED(portIndex);
        }

        QWidget* embeddedWidget() override
        {
            return _fileSelectComboBox;
        }

        QJsonObject save() const override
        {
            QJsonObject modelJson = NodeDelegateModel::save();
            if (m_outImageData && !file().isEmpty()) {
                modelJson["path"] = file();
            }
            return modelJson;
        }

        void load(QJsonObject const& jsonObj) override
        {
            const QJsonValue path = jsonObj["path"];
            if (!path.isUndefined()) {
                setFile(path.toString());
            }
        }

        QString file() const
        {
            return m_file;
        }

        void setFile(const QString& fileName)
        {
            const QString trimmed = fileName.trimmed();
            if (trimmed == m_file) {
                return;
            }
            m_file = trimmed;
            Q_EMIT fileChanged(trimmed);
        }

    Q_SIGNALS:
        void fileChanged(const QString& file);

    protected:
        void afterModelReady() override
        {
            GlobalEventBus::instance()->subscribe(
                makeFullOscAddress("/file"),
                this,
                SLOT(onGlobalEvent(GlobalEvent)));
        }

    private Q_SLOTS:
        void onGlobalEvent(const GlobalEvent& ev)
        {
            if (ev.kind != GlobalEventKind::Command) {
                return;
            }
            if (ev.address != makeFullOscAddress("/file")) {
                return;
            }
            setFile(ev.payload.toString());
        }

    private:
        // 将缓存帧写入 ring buffer 一次；timestamp 取当前系统帧号以便下游对齐时钟。
        // pushFrameToImageBufferDedup 会跳过与 m_lastPushedTimestamp 相同的重复写入。
        void publishFrameOnce(qint64 timestamp)
        {
            if (m_cachedFrame.empty() || !m_cachedFrame.texture.valid() || timestamp < 0) {
                return;
            }

            ensureImageDataBuffer(m_outImageData, m_outputBuffer);

            ImageFrame frame;
            frame.texture = m_cachedFrame.texture;
            frame.timestamp = timestamp;
            qDebug() << "publishFrameOnce: " << timestamp;
            pushFrameToImageBufferDedup(m_outputBuffer, std::move(frame), m_lastPushedTimestamp);
        }

        void loadImage(QString fileName)
        {
            ensureImageDataBuffer(m_outImageData, m_outputBuffer);

            // ---- 错误路径：清空缓冲，不 emit dataUpdated ----
            // 下游会在下一 tick 发现 buffer 无有效帧并自行清屏/报错。
            if (fileName.isEmpty()) {
                updateNodeState(QtNodes::NodeValidationState::State::Error, "cannot open file");
                m_cachedFrame = ImageFrame();
                m_lastPushedTimestamp = -1;
                if (m_outputBuffer) {
                    m_outputBuffer->clear();
                }
                return;
            }

            m_path = AppConstants::MEDIA_LIBRARY_STORAGE_DIR + "/" + fileName;
            const cv::Mat decoded = ImageReadback::decodeImageFromFile(m_path);
            if (decoded.empty()) {
                updateNodeState(QtNodes::NodeValidationState::State::Error, "cannot open file");
                m_cachedFrame = ImageFrame();
                m_lastPushedTimestamp = -1;
                if (m_outputBuffer) {
                    m_outputBuffer->clear();
                }
                return;
            }

            // 解码成功：上传 GPU 纹理，并保留 CPU 副本供下游 OpenCV 节点读 frame.image / matFromFrame
            m_cachedFrame = ImageFrame::fromMat(decoded);
            m_lastPushedTimestamp = -1;

            if (!m_cachedFrame.ensureGpuTexture()) {
                updateNodeState(QtNodes::NodeValidationState::State::Error, "cannot upload texture");
                if (m_outputBuffer) {
                    m_outputBuffer->clear();
                }
                return;
            }

            // 成功：写入 ring buffer 一次即可；不 emit dataUpdated，避免事件链式刷新显示。
            publishFrameOnce(TimestampGenerator::getInstance()->getCurrentFrameCount());
            updateNodeState(QtNodes::NodeValidationState::State::Valid);
        }

        SelectorComboBox* _fileSelectComboBox = new SelectorComboBox(MediaLibrary::Category::Image);
        QString m_path;
        QString m_file;

        std::shared_ptr<ImageData> m_outImageData;
        std::shared_ptr<ImageTimestampRingQueue> m_outputBuffer;
        qint64 m_lastPushedTimestamp = -1;  // 去重：避免同一 timestamp 重复 push
        ImageFrame m_cachedFrame;             // 本地缓存，供 publishFrameOnce 引用纹理句柄
    };
}
