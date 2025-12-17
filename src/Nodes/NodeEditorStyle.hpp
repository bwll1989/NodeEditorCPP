#include "QtNodes/ConnectionStyle"
#include "QtNodes/NodeStyle"
#include <QtNodes/GraphicsViewStyle>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>

using QtNodes::GraphicsViewStyle;
using QtNodes::ConnectionStyle;
using QtNodes::NodeStyle;
using QtNodes::GroupStyle;

/**
 * 函数：setNodeEditorDarkStyle
 * 作用：从 JSON 样式文件加载并应用 NodeEditor 的样式（支持资源与磁盘回退）。
 * 参数：isDarkTheme 为 true 使用暗色样式，false 使用亮色样式。
 */
static void setNodeEditorDarkStyle(bool isDarkTheme=true)
{
    QString rsrcPath = isDarkTheme
        ? ":/styles/styles/NodeEditorDarkStyle.json"
        : ":/styles/styles/NodeEditorLightStyle.json";
    QString diskPath = isDarkTheme
        ? "res/styles/NodeEditorDarkStyle.json"
        : "res/styles/NodeEditorLightStyle.json";

    QByteArray styleData;

    // 优先从磁盘加载（便于热更新），失败则回退到资源路径
    {
        QFile diskFile(diskPath);
        if (diskFile.open(QIODevice::ReadOnly)) {
            styleData = diskFile.readAll();
            diskFile.close();
        } else {
            QFile rsrcFile(rsrcPath);
            if (rsrcFile.open(QIODevice::ReadOnly)) {
                styleData = rsrcFile.readAll();
                rsrcFile.close();
            } else {
                qWarning("无法打开样式文件（磁盘与资源均失败）：%s / %s", qPrintable(diskPath), qPrintable(rsrcPath));
                return;
            }
        }
    }

    QJsonDocument styleDoc = QJsonDocument::fromJson(styleData);
    if (styleDoc.isNull() || !styleDoc.isObject()) {
        qWarning("样式文件格式错误，将使用默认样式");
        return;
    }
    QJsonObject styleObj = styleDoc.object();

    if (styleObj.contains("ConnectionStyle")) {
        QJsonObject obj; obj["ConnectionStyle"] = styleObj["ConnectionStyle"];
        ConnectionStyle::setConnectionStyle(QJsonDocument(obj).toJson());
    }
    if (styleObj.contains("NodeStyle")) {
        QJsonObject obj; obj["NodeStyle"] = styleObj["NodeStyle"];
        NodeStyle::setNodeStyle(QJsonDocument(obj).toJson());
    }
    if (styleObj.contains("GraphicsViewStyle")) {
        QJsonObject obj; obj["GraphicsViewStyle"] = styleObj["GraphicsViewStyle"];
        GraphicsViewStyle::setStyle(QJsonDocument(obj).toJson());
    }
    if (styleObj.contains("GroupStyle")) {
        QJsonObject obj; obj["GroupStyle"] = styleObj["GroupStyle"];
        GroupStyle::setGroupStyle(QJsonDocument(obj).toJson());
    }
}
