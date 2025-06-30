#include "QtNodes/ConnectionStyle"
#include "QtNodes/NodeEditorStyle"
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
 * @brief 从DefaultNodeEditorStyle.json文件中读取并设置样式
 * 
 * 该函数从资源文件中读取DDefaultNodeEditorStyle.json，并将其中定义的样式应用到
 * ConnectionStyle、NodeStyle、GraphicsViewStyle和GroupStyle中
 */
static void setNodeEditorStyle()
{
    // 构建样式文件路径
    // QString stylePath = QApplication::applicationDirPath() + "/res/styles/DefaultStyle.json";
    
    // 尝试从资源文件中读取样式
    QFile styleFile(":/styles/styles/DefaultNodeEditorStyle.json");

    if (!styleFile.open(QIODevice::ReadOnly)) {

        qWarning("无法打开样式文件:/styles/styles/DefaultNodeEditorStyle.json，将使用默认样式");
        return;
    }
    
    // 读取JSON内容
    QByteArray styleData = styleFile.readAll();
    styleFile.close();
    
    QJsonDocument styleDoc = QJsonDocument::fromJson(styleData);
    if (styleDoc.isNull() || !styleDoc.isObject()) {

        qWarning("样式文件格式错误，将使用默认样式");
        return;
    }
    
    QJsonObject styleObj = styleDoc.object();
    
    // 设置ConnectionStyle
    if (styleObj.contains("ConnectionStyle")) {
        QJsonObject connectionStyleObj;
        connectionStyleObj["ConnectionStyle"] = styleObj["ConnectionStyle"];
        QJsonDocument connectionDoc(connectionStyleObj);
        ConnectionStyle::setConnectionStyle(connectionDoc.toJson());
    }
    
    // 设置NodeStyle
    if (styleObj.contains("NodeStyle")) {
        QJsonObject nodeStyleObj;
        nodeStyleObj["NodeStyle"] = styleObj["NodeStyle"];

        QJsonDocument nodeDoc(nodeStyleObj);
        NodeStyle::setNodeStyle(nodeDoc.toJson());
    }
    
    // 设置GraphicsViewStyle
    if (styleObj.contains("GraphicsViewStyle")) {
        QJsonObject viewStyleObj;
        viewStyleObj["GraphicsViewStyle"] = styleObj["GraphicsViewStyle"];
        QJsonDocument viewDoc(viewStyleObj);
        GraphicsViewStyle::setStyle(viewDoc.toJson());
    }
    
    // 设置GroupStyle
    if (styleObj.contains("GroupStyle")) {
        QJsonObject groupStyleObj;
        groupStyleObj["GroupStyle"] = styleObj["GroupStyle"];
        QJsonDocument groupDoc(groupStyleObj);
        GroupStyle::setGroupStyle(groupDoc.toJson());
    }

}
