#include "StatusItem.h"
#include <QJsonValue>

#include "ConstantDefines.h"

StatusItem::StatusItem() : ptr(nullptr) {}

StatusItem::StatusItem(QWidget* p, const QString& a, const QVariant& v)
    : ptr(p), address(a), value(v) {}

OSCMessage StatusItem::toOSCMessage() const {
    OSCMessage msg;
    msg.host="127.0.0.1";
    msg.port=AppConstants::EXTRA_CONTROL_PORT;
    msg.address = address;
    msg.value = value;
    return msg;
}

QJsonObject StatusItem::toJsonObject() const {
    QJsonObject obj;
    obj["address"] = address;
    obj["value"] = QJsonValue::fromVariant(value);
    return obj;
}
