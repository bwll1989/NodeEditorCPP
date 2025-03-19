#ifndef OSC_MESSAGE_H
#define OSC_MESSAGE_H
#include <QString>
#include <QVariant>

struct OSCMessage {
    QString host;
    int port;
    QString address;
    QVariant value;
};

#endif // OSC_MESSAGE_H
