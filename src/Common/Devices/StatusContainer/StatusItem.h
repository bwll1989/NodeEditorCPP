//
// Created by WuBin on 2025/12/29.
//
#pragma once

#include <QString>
#include <QVariant>
#include <QJsonObject>
#include <QMetaType>
#include "OSCMessage.h"
class QWidget;

#ifdef STATUSCONTAINER_LIBRARY
#define STATUSCONTAINER_EXPORT Q_DECL_EXPORT
#else
#define STATUSCONTAINER_EXPORT Q_DECL_IMPORT
#endif

struct STATUSCONTAINER_EXPORT StatusItem {
    QWidget* ptr;     // 注册控件的指针
    QString address;  // 控件地址
    QVariant value;   // 控件当前值

    StatusItem();
    /**
     * 函数级注释：带参构造，存储注册控件指针、地址与当前值
     */
    StatusItem(QWidget* p, const QString& a, const QVariant& v);

    /**
     * 函数级注释：转换为 OSCMessage 对象（仅填充 address 与 value）
     */
    OSCMessage toOSCMessage() const;

    /**
     * 函数级注释：转换为 QJsonObject 对象（不导出控件指针）
     */
    QJsonObject toJsonObject() const;
};

Q_DECLARE_METATYPE(StatusItem)
