/**
 * @file FmodDecoderDataModel.hpp
 * @brief Fmod 节点模型：Bank 加载、动态输入口（触发 + 参数）、面板与 Worker 协作
 *
 * 不注册 Q_PROPERTY / ExternalBinding，不做 OSC 状态反馈与外部命令控制；
 * 仅通过面板与节点输入口交互。
 */

#pragma once

#include "Common/BaseClass/AbstractDelegateModel.h"
#include "FmodDecoderInterface.hpp"
#include "FmodDecoderWorker.h"
#include "NodeDataList.hpp"

#include "QtNodes/Definitions"
#include "QtNodes/NodeDelegateModel"

#include <QThread>
#include <QVariantMap>
#include <QVector>
#include <memory>

using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::PortIndex;
using QtNodes::PortType;

using namespace NodeDataTypes;

namespace Nodes
{

/**
 * @brief FMOD Bank 播放节点的 DataModel
 *
 * 职责概览：
 * - 管理内嵌 UI 与后台 FmodDecoderWorker（独立线程）
 * - Bank 加载后按事件重建输入口与面板控件（一一对应）
 * - 触发口 / Play → 播放；参数口 / 面板 → setParameter
 */
class FmodDecoderDataModel : public AbstractDelegateModel
{
    Q_OBJECT

public:
    /** 输入口语义：事件触发 或 参数数值 */
    enum class InPortKind { Trigger, Parameter };

    /** 与 InPortCount 下标对齐的端口描述 */
    struct InPortDesc {
        InPortKind kind = InPortKind::Trigger;
        QString eventPath;   ///< FMOD 事件路径（通常含 event:/）
        QString paramName;   ///< 仅 Parameter 口有效
        QString caption;     ///< 端口显示名
    };

    FmodDecoderDataModel();
    ~FmodDecoderDataModel() override;

    NodeDataType dataType(PortType portType, PortIndex portIndex) const override;

    /**
     * @brief 处理上游数据
     * - Trigger：仅 asBool()==true 时播放（避免连线瞬间 false/当前值误触发）
     * - Parameter：asNumber() 写入参数并同步面板
     */
    void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override;

    /** @brief 输出共享环形缓冲包装为 AudioData */
    std::shared_ptr<NodeData> outData(PortIndex port) override;

    /** @brief 存盘：Bank 路径 + 参数当前值 */
    QJsonObject save() const override;

    /** @brief 读盘：先恢复参数表，再设 Bank 路径触发异步加载 */
    void load(QJsonObject const& p) override;

    QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

    QWidget* embeddedWidget() override { return widget_; }

public slots:
    /** @brief Select 按钮：选 Bank 文件夹 */
    void selectBankFolder();

    /**
     * @brief Worker 枚举完成：刷新事件/参数缓存、输入口、面板，并把参数推到 Worker 缓存
     */
    void onEventCatalogUpdated(const QStringList& events, const QVector<Nodes::FmodParamDesc>& params);

    /**
     * @brief 面板或参数口改参：记入 paramValues_，并转发 Worker（缓存 + 存活实例）
     */
    void onParameterChanged(const QString& eventPath, const QString& paramName, float value);

private slots:
    void playEventPath(const QString& eventPath);

private:
    /** @brief 按 availableEvents_/availableParams_ 重建 inPorts_（触发口后紧跟参数口） */
    void rebuildInPorts();

    /** @brief 将 InPortCount 与 inPorts_.size() 对齐，并通知图模型增删口 */
    void syncInputPortCount();

    /** @brief 设置 Bank 目录：同步 UI 文本并异步 loadBanks */
    void setBankPath(const QString& path);

    FmodDecoderInterface* widget_ = nullptr;

    std::vector<std::shared_ptr<AudioTimestampRingQueue>> outputBuffers_;

    QStringList availableEvents_;
    QVector<FmodParamDesc> availableParams_;
    QVector<InPortDesc> inPorts_;

    /** 参数存盘/面板值；key = eventPath + U+001F + paramName */
    QVariantMap paramValues_;

    QThread* workerThread_ = nullptr;
    FmodDecoderWorker* worker_ = nullptr;

    QString bankPath_;
};

} // namespace Nodes
