//
// Created by WuBin on 2025/12/30.
//

#pragma once

#include <QComboBox>
#include <QAbstractButton>
#include <QAbstractSlider>
#include <QSpinBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QVariant>
#include <unordered_map>

#include "AbstractClipModel.hpp"
#include "Common/Devices/StatusContainer/StatusContainer.h"
#include "Common/Devices/StatusContainer/GlobalEventBus.hpp"
class AbstractClipDelegateModel :  public AbstractClipModel {
    Q_OBJECT
public:
    explicit AbstractClipDelegateModel(int start,const QString& filePath = QString(), QObject* parent = nullptr);

    ~AbstractClipDelegateModel() override;

    void setId(ClipId id) override {
        // 函数级注释：设置剪辑 ID 后触发一次统一的控件注册与状态推送
        AbstractClipModel::setId(id);
        onModelReady();
    }

    /**
     * 函数级注释：注册剪辑属性控件到本地映射（相对 OSC 地址）
     * - 不直接与 StatusContainer/事件总线交互，等待 ID 就绪后由 onModelReady 统一处理
     */
    void registerExternalControl(const QString& oscAddress, QWidget* control) override
    {
        if (!oscAddress.startsWith("/")) return;
        if (!control) return;
        auto it = _OscMapping.find(oscAddress);
        if (it != _OscMapping.end()) {
            _OscMapping.erase(it);
        }
        control->installEventFilter(this);
        control->setMouseTracking(true);
        _OscMapping[oscAddress] = control;
    }

    /**
     * 函数级注释：根据剪辑 ID 构造完整的 OSC 地址（/timeline/<alias>/<id><relative>）
     */
    QString makeFullOscAddress(const QString& oscAddress) const {
        const QString norm = oscAddress.startsWith("/") ? oscAddress : ("/" + oscAddress);
        return  "/timeline/" + m_modelAlias + "/" + QString::number(m_id)  + norm;
    }

    /**
     * 函数级注释：通过事件总线发送状态反馈事件，避免直接操作 StatusContainer
     * @param oscAddress 相对地址（形如 "/gain"），内部自动补全为完整地址
     */
    void stateFeedBack(const QString& oscAddress, QVariant value) {
        GlobalEventBus::instance()->publishState(makeFullOscAddress(oscAddress), value);
    }

    /**
     * 函数级注释：剪辑 ID 与别名就绪后统一注册控件并推送初始状态
     * - 为每个控件在 StatusContainer 中注册完整地址
     * - 根据控件类型将当前值以 StateFeedback 事件形式推送到事件总线
     * - 调用 afterModelReady 供派生类执行额外初始化（例如事件总线订阅命令）
     */
    void onModelReady()
    {
        for (auto it = _OscMapping.begin(); it != _OscMapping.end(); ++it) {
            const QString& rel = it->first;
            QWidget* control = it->second;
            if (!control) continue;
            const QString full = makeFullOscAddress(rel);
            StatusContainer::instance()->registerWidget(control, full);

            if (auto* button = qobject_cast<QAbstractButton*>(control)) {
                if (button->isCheckable()){
                    this->stateFeedBack(rel, QVariant(button->isChecked()));
                }
            } else if (auto* slider = qobject_cast<QAbstractSlider*>(control)) {
                this->stateFeedBack(rel, QVariant(slider->value()));
            } else if (auto* spinBox = qobject_cast<QSpinBox*>(control)) {
                this->stateFeedBack(rel, QVariant(spinBox->value()));
            } else if (auto* lineEdit = qobject_cast<QLineEdit*>(control)) {
                this->stateFeedBack(rel, QVariant(lineEdit->text()));
            } else if (auto* comboBox = qobject_cast<QComboBox*>(control)) {
                this->stateFeedBack(rel, QVariant(comboBox->currentIndex()));
            } else if (auto* checkBox = qobject_cast<QCheckBox*>(control)) {
                this->stateFeedBack(rel, QVariant(checkBox->isChecked()));
            }
        }
        afterModelReady();
    }

protected:
    /**
     * 函数级注释：剪辑模型初始化完成后的扩展点（ClipId 与别名已就绪）
     * - 派生类可重写，在此阶段基于完整地址执行事件总线订阅等操作
     */
    virtual void afterModelReady() {}

private:
    QObject* _oscConnectionProxy = nullptr;
};
