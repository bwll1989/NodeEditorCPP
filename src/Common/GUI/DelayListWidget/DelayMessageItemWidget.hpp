#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QHBoxLayout>

#include <QSpinBox>

#include "../../Common/Devices/OSCSender/OSCSender.h"
#if defined(DELAYLISTWIDGET_LIBRARY)
#define DELAYLISTWIDGET_EXPORT Q_DECL_EXPORT
#else
#define DELAYLISTWIDGET_EXPORT Q_DECL_IMPORT
#endif
struct DELAYLISTWIDGET_EXPORT delay_item {
    int port = 0;
    int time = 0;
};
class DELAYLISTWIDGET_EXPORT DelayMessageItemWidget : public QWidget {
    Q_OBJECT
public:
    explicit DelayMessageItemWidget(QWidget* parent = nullptr);
    
    // 获取当前OSC消息
    delay_item getMessage() const;

    // 设置OSC消息
    void setMessage(const delay_item& message);

signals:
    void messageChanged();

private:
    QSpinBox* portIndex;
    QLineEdit* timeEdit;
    void setupUI();

};
