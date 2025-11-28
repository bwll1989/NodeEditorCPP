#ifndef OSCMESSAGEITEMWIDGET_HPP
#define OSCMESSAGEITEMWIDGET_HPP

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QHBoxLayout>
#include <QJSEngine>
#include "../../Common/Devices/OSCSender/OSCSender.h"
#if defined(OSCLISTWIDGET_LIBRARY)
#define OSCLISTWIDGET_EXPORT Q_DECL_EXPORT
#else
#define OSCLISTWIDGET_EXPORT Q_DECL_IMPORT
#endif
class OSCLISTWIDGET_EXPORT OSCMessageItemWidget : public QWidget {
    Q_OBJECT
public:
    explicit OSCMessageItemWidget(bool onlyInternal,QWidget* parent = nullptr);
    
    // 获取当前OSC消息
    OSCMessage getMessage() const;
    
    // 设置OSC消息
    void setMessage(const OSCMessage& message);

    QString getExpression() const;

    QString getAddress() const;

    QJSEngine* getJSEngine();
public slots:
    // 设置表达式
    void setExpression(QString val);
    // 设置地址
    void setAddress(QString addr);
signals:
    void messageChanged();

private:
    QLineEdit* hostEdit;
    QLineEdit* addressEdit;
    QComboBox* typeCombo;
    QLineEdit* valueEdit;
    mutable QJSEngine engine;
    mutable OSCMessage m_currentMessage;
    void setupUI();
    void connectSignals();
    void updateValueWidget(const QString& type);
    bool OnlyInternal;
};

#endif // OSCMESSAGEITEMWIDGET_HPP 