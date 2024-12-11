//
// Created by WuBin on 2024/12/6.
//

#include <QJsonArray>
#include "HttpClient.h"
#include "QtNetwork/QNetworkAccessManager"
#include "QtNetwork//QNetworkReply"

HttpClient::HttpClient(QObject *parent ) : QObject(parent) {
        connect(&manager, &QNetworkAccessManager::finished, this, &HttpClient::onReplyFinished);
    }

// 发送 POST 请求
void HttpClient::sendPostRequest(const QUrl &url, const QJsonObject &data) {
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // 将 QJsonObject 转换为 QByteArray
    QJsonDocument doc(data);
    QByteArray byteArray = doc.toJson();

    // 发送 POST 请求
    manager.post(request, byteArray);
}

void HttpClient::sendGetRequest(const QUrl &url) {
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // 发送 GET 请求
    manager.get(request);
}

// 响应完成的处理槽
void HttpClient::onReplyFinished(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        // 获取回复内容
        QByteArray responseData = reply->readAll();

        // 将 QByteArray 转换为 QJsonDocument
        QJsonDocument doc = QJsonDocument::fromJson(responseData);

        // 检查解析是否成功
        if (doc.isNull()) {
            qDebug() << "Failed to parse JSON";
        } else {
            // 解析成功，可以根据实际情况处理 JSON
            if (doc.isObject()) {
                QJsonObject jsonObj = doc.object();
//                qDebug() << "Parsed JSON Object:" << jsonObj;
                emit getSatus(jsonObj);
                // 可以访问 JSON 对象的各个字段
                // 示例：jsonObj["key"]
            } else if (doc.isArray()) {
                QJsonArray jsonArray = doc.array();
                qDebug() << "Parsed JSON Array:" << jsonArray;
                // 可以遍历 JSON 数组
            }
        }
    } else {
        qDebug() << "Error:" << reply->errorString();
    }

    reply->deleteLater();  // 记得删除回复对象
}


