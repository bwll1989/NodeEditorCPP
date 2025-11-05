//
// Created by WuBin on 2024/12/6.
//

#ifndef NODEEDITORCPP_HTTPCLIENT_H
#define NODEEDITORCPP_HTTPCLIENT_H

#include <QJsonDocument>
#include "QObject"
#include "QtNetwork/QNetworkAccessManager"
#include "QtNetwork//QNetworkReply"
#include <QJsonObject>
namespace Nodes
{
    class HttpClient : public QObject {
        Q_OBJECT

        public:
        HttpClient(QObject *parent = nullptr) ;

        // 发送 POST 请求
        void sendPostRequest(const QUrl &url, const QJsonObject &data = QJsonObject()) ;
        void sendGetRequest(const QUrl &url) ;
    signals:
        void getSatus(QJsonObject &status);

    private slots:
        // 响应完成的处理槽
        void onReplyFinished(QNetworkReply *reply) ;

    private:
        QNetworkAccessManager manager;
    };
}
#endif //NODEEDITORCPP_HTTPCLIENT_H
