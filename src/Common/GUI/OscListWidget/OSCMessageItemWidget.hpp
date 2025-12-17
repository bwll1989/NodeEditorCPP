#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QJSEngine>

#include "PushButton.h"
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
    /**
     * 函数：requestDelete
     * 作用：用户点击卡片上的“删除”按钮时发出该信号，
     *       由外部容器（如列表或布局管理器）执行实际删除。
     */
    void requestDelete();

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

private:
    // 卡片强调条
    QFrame* m_accentBar;
    /**
     * 新增：删除按钮（位于卡片右上角工具面板）
     */
    // QPushButton* m_deleteBtn = nullptr;
};
