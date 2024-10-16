//
// Created by bwll1 on 2024/9/1.
//

#ifndef NODEEDITORCPP_EXTERNALCONTROLER_H
#define NODEEDITORCPP_EXTERNALCONTROLER_H
#include <QObject>
#include <QColor>
#include <QUdpSocket>
class MainWindow;
class ExternalControler:public QObject {
        Q_OBJECT

        public:
        explicit ExternalControler(MainWindow *widget, quint16 port = 12345, QObject *parent = nullptr);

        void changeBackgroundColor(const QColor &color);  // 改变背景颜色
        void showWidget();  // 显示窗口
        void hideWidget();  // 隐藏窗口

        private slots:
        void processPendingDatagrams();  // 处理接收到的UDP数据
        void handleJsonMessage(const QJsonObject &json);  // 处理解析后的JSON消息

        private:
        MainWindow *m_widget;  // 控制的 QWidget 对象
        QUdpSocket *m_udpSocket;  // 用于接收 UDP 数据的套接字
    };


#endif //NODEEDITORCPP_EXTERNALCONTROLER_H
