//
// Created by bwll1 on 2024/9/1.
//

#ifndef NODEEDITORCPP_EXTERNALCONTROLER_HPP
#define NODEEDITORCPP_EXTERNALCONTROLER_HPP
#include <QObject>
#include <QColor>
#include <QJsonObject>

#include "Common/Devices/OSCReceiver/OSCReceiver.h"
#include "qtvariantproperty.h"
#include "Common/GUI/QPropertyBrowser/QPropertyBrowser.h"
#include <QWidget>
class MainWindow;
class ExternalControler:public QWidget {
        Q_OBJECT
    public:
        explicit ExternalControler(MainWindow *widget, quint16 port = 12345, QWidget *parent = nullptr);
    private slots:
        /**
         * 处理接收到的UDP数据
         * @param const QVariantMap &data 接收到的UDP数据
         */
        void hasOSC(const QVariantMap &data);  // 处理接收到的UDP数据
        // void handleJsonMessage(const QJsonObject &json);  // 处理解析后的JSON消息

    private:
        //要控制的窗口
        MainWindow *m_widget;  // 控制的 QWidget 对象
        //OSC接收器
        OSCReceiver *OSC_Receiver;
        //布局
        QVBoxLayout *layout;
        // QtVariantPropertyManager *variantManager;

    };


#endif //NODEEDITORCPP_EXTERNALCONTROLER_HPP
