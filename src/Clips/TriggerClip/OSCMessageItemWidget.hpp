#ifndef OSCMESSAGEITEMWIDGET_HPP
#define OSCMESSAGEITEMWIDGET_HPP

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QHBoxLayout>
#include "../../Common/Devices/OSCSender/OSCSender.h"

class OSCMessageItemWidget : public QWidget {
    Q_OBJECT
public:
    explicit OSCMessageItemWidget(QWidget* parent = nullptr);
    
    // 获取当前OSC消息
    OSCMessage getMessage() const;
    
    // 设置OSC消息
    void setMessage(const OSCMessage& message);

signals:
    void messageChanged();

private:
    QLineEdit* hostEdit;
    QLineEdit* addressEdit;
    QComboBox* typeCombo;
    QLineEdit* valueEdit;
    
    void setupUI();
    void connectSignals();
    void updateValueWidget(const QString& type);
};

#endif // OSCMESSAGEITEMWIDGET_HPP 