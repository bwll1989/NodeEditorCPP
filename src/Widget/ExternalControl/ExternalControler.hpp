//
// Created by bwll1 on 2024/9/1.
//

#ifndef NODEEDITORCPP_EXTERNALCONTROLER_HPP
#define NODEEDITORCPP_EXTERNALCONTROLER_HPP
#include <QObject>
#include <QColor>
#include <QJsonObject>
#include <unordered_map>
#include <QString>
#include <memory>
#include "Common/Devices/OSCReceiver/OSCReceiver.h"
#include "OSCMessage.h"
#include <QWidget>
class ExternalControler:public QObject {
        Q_OBJECT
public:
    explicit ExternalControler();
    ~ExternalControler();
    static ExternalControler* instance() {
        static ExternalControler* instance = nullptr;
        if (!instance) {
            instance = new ExternalControler(); // 需要传入父窗口参数
        }
        return instance;
    }
    // 注册控件到OSC地址映射
    void registerControl(const QString& oscAddress, QWidget* control);

    // 取消注册控件
    void unregisterControl(const QString& oscAddress);

        // 获取指定OSC地址对应的控件
    QWidget* getControl(const QString& oscAddress) const;

    // 获取所有注册的OSC地址
    QStringList getRegisteredAddresses() const;
private slots:
    /**
     * 处理接收到的UDP数据
     * @param const QVariantMap &data 接收到的UDP数据
     */
    void hasOSC(const OSCMessage &message);  // 处理接收到的UDP数据
    // void handleJsonMessage(const QJsonObject &json);  // 处理解析后的JSON消息
 
private:
    // 禁用拷贝和赋值
    ExternalControler(const ExternalControler&) = delete;
    ExternalControler& operator=(const ExternalControler&) = delete;
private:
    std::unordered_map<QString, QWidget*> controlMap;  // OSC地址到控件的映射
    //OSC接收器
    OSCReceiver *OSC_Receiver;
    //布局

    // QtVariantPropertyManager *variantManager;

};


#endif //NODEEDITORCPP_EXTERNALCONTROLER_HPP
