//
// Created by WuBin on 2025/12/30.
//

#pragma once

#include <QComboBox>

#include "AbstractClipModel.hpp"
#include "Common/Devices/StatusContainer/StatusContainer.h"
class AbstractClipDelegateModel :  public AbstractClipModel {
    Q_OBJECT
public:
    explicit AbstractClipDelegateModel(int start,const QString& filePath = QString(), QObject* parent = nullptr);

    ~AbstractClipDelegateModel() override;

    void setId(ClipId id) override {
        AbstractClipModel::setId(id);
        updateOSCRegistration();
    }

    void updateOSCRegistration()
    {
        for(auto it = _OscMapping.begin(); it != _OscMapping.end(); ++it) {
            QWidget* control = it->second;
            QString oscAddress = it->first;
            // 断开之前的连接，防止重复绑定
            if(_oscConnectionProxy) control->disconnect(_oscConnectionProxy);
            const QString full = makeFullOscAddress(oscAddress);
            // qDebug() << "Registered OSC address: " << full;
            registerOSCFeedBack(full, control);
            StatusContainer::instance()->registerWidget(control, full);
        }
    }

    void registerOSCControl(const QString& oscAddress, QWidget* control) override
    {
        // 如果oscAddress不以"/"开头，则不注册
        if (!oscAddress.startsWith("/")) return;
        // 构建完整的OSC地址，自动给OSC地址添加前缀，包括节点ID
        if (!control) return;
        // 如果已存在相同地址的映射，先移除旧的
        auto it = _OscMapping.find(oscAddress);
        if (it != _OscMapping.end()) {
            _OscMapping.erase(it);
        }
        // qDebug() << "AbstractClipDelegateModel Register OSC control: " << oscAddress << " -> " << control;
        // 添加新的映射
        control->installEventFilter(this);
        control->setMouseTracking(true);
        _OscMapping[oscAddress] = control;
        StatusContainer::instance()->registerWidget(control, makeFullOscAddress(oscAddress));
        const QString full = makeFullOscAddress(oscAddress);
        registerOSCFeedBack(full, control);
        StatusContainer::instance()->registerWidget(control, full);
    }

    void registerOSCFeedBack(const QString& oscAddress, QWidget* feedback)
    {

        // 绑定值变化信号到 stateFeedBack
        if (auto* button = qobject_cast<QAbstractButton*>(feedback)) {
            if (button->isCheckable()){
                this->stateFeedBack(oscAddress, QVariant(button->isChecked()));
                QObject::connect(button, &QAbstractButton::toggled, _oscConnectionProxy, [this, oscAddress](bool checked) {
                    this->stateFeedBack(oscAddress, QVariant(checked));
                });
            }else {
                QObject::connect(button, &QAbstractButton::pressed, _oscConnectionProxy, [this, oscAddress]() {
                this->stateFeedBack(oscAddress, QVariant(1));
            });
                QObject::connect(button, &QAbstractButton::released, _oscConnectionProxy, [this, oscAddress]() {
                this->stateFeedBack(oscAddress, QVariant(0));
            });
            }

        } else if (auto* slider = qobject_cast<QAbstractSlider*>(feedback)) {
            this->stateFeedBack(oscAddress, QVariant(slider->value()));
            QObject::connect(slider, &QAbstractSlider::valueChanged, _oscConnectionProxy, [this, oscAddress](int value) {
                this->stateFeedBack(oscAddress, QVariant(value));
            });
        } else if (auto* spinBox = qobject_cast<QSpinBox*>(feedback)) {
            this->stateFeedBack(oscAddress, QVariant(spinBox->value()));
            QObject::connect(spinBox, QOverload<int>::of(&QSpinBox::valueChanged), _oscConnectionProxy, [this, oscAddress](int value) {
                this->stateFeedBack(oscAddress, QVariant(value));
            });
        } else if (auto* lineEdit = qobject_cast<QLineEdit*>(feedback)) {
            this->stateFeedBack(oscAddress, QVariant(lineEdit->text()));
            QObject::connect(lineEdit, &QLineEdit::textChanged, _oscConnectionProxy, [this, oscAddress](const QString& text) {
                this->stateFeedBack(oscAddress, QVariant(text));
            });
        } else if (auto* comboBox = qobject_cast<QComboBox*>(feedback)) {
            this->stateFeedBack(oscAddress, QVariant(comboBox->currentIndex()));
            QObject::connect(comboBox, &QComboBox::currentIndexChanged, _oscConnectionProxy, [this, oscAddress](int value) {
                this->stateFeedBack(oscAddress, QVariant(value));
            });
        } else if (auto* checkBox = qobject_cast<QCheckBox*>(feedback)) {
            this->stateFeedBack(oscAddress, QVariant(checkBox->isChecked()));
            QObject::connect(checkBox, &QCheckBox::checkStateChanged, _oscConnectionProxy, [this, oscAddress](int state) {
                this->stateFeedBack(oscAddress, QVariant(state));
            });
        }
    }

    QString makeFullOscAddress(const QString& oscAddress) const {
        // 函数级注释：根据节点ID添加前缀
        return  "/timeline/" + m_modelAlias + "/" + QString::number(m_id)  + oscAddress;
    }
    void stateFeedBack(const QString& oscAddress, QVariant value) {
        // 函数级注释：默认将状态写入到 StatusContainer，仅更新值
        StatusContainer::instance()->updateState(oscAddress, value);
    }
private:
    QObject* _oscConnectionProxy = nullptr;
};
