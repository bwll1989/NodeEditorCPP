//
// Created by WuBin on 2025/12/29.
//

#pragma once

#include <memory>
#include <QVariant>
#include <QString>
#include <QtWidgets/QWidget>
#include <QtNodes/internal/NodeDelegateModel.hpp>
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using QtNodes::ConnectionPolicy;


/**
 * 函数级注释：基于 QtNodes::NodeDelegateModel 的抽象基类
 * - 提供统一的控件注册与状态反馈到 StatusContainer 的能力
 * - 你的具体节点应继承该类并实现数据流相关的纯虚函数
 */
class AbstractDelegateModel : public QtNodes::NodeDelegateModel {
    Q_OBJECT
public:
    /**
     * 函数级注释：构造函数，保持与 NodeDelegateModel 一致的初始化行为
     */
    explicit AbstractDelegateModel();

    /**
     * 函数级注释：析构函数
     */
    ~AbstractDelegateModel() override;
    /**
     * 函数级注释：注册控件到 OSC 地址映射，并同步注册到 StatusContainer
     */
    void registerOSCControl(const QString& oscAddress, QWidget* control) override;
    /**
     * 函数级注释：模型初始化完成回调（GraphModel 设置了 NodeID 与 ParentAlias 之后调用）
     * - 将已有的控件映射统一注册到 StatusContainer
     */
    void onModelReady();
    /**
     * 函数级注释：状态反馈，默认写入到 StatusContainer（地址为完整 /dataflow/...）
     */
    virtual void stateFeedBack(const QString& oscAddress, QVariant value);

protected:
    /**
     * 函数级注释：构造完整的 OSC 地址（/dataflow/<alias>/<nodeId><relative>）
     */
    QString makeFullOscAddress(const QString& relative) const;

public:
    /**
     * 函数级注释：返回输出端口数据（需由派生类实现）
     */
    virtual std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex const port) = 0;

    /**
     * 函数级注释：设置输入端口数据（需由派生类实现）
     */
    virtual void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex const portIndex) = 0;

    /**
     * 函数级注释：返回端口数据类型（需由派生类实现）
     */
    virtual QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const = 0;

    /**
     * 函数级注释：返回嵌入式控件（需由派生类实现）
     */
    virtual QWidget* embeddedWidget() = 0;

    ConnectionPolicy portConnectionPolicy(PortType portType, PortIndex index) const override;

private:
    bool _ready = false;
};

