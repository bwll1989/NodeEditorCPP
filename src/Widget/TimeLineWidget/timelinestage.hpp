#ifndef TIMELINESTAGE_HPP
#define TIMELINESTAGE_HPP

#include <QQuickItem>
#include <QQmlEngine>
#include <QVector>
#include "timelinescreen.hpp"
#include "timelinestyle.hpp"

class TimelineStage : public QQuickItem
{
    Q_OBJECT

    // 添加样式属性
    Q_PROPERTY(QColor stageBgColor READ stageBgColor CONSTANT)
    Q_PROPERTY(QColor stageAxisXColor READ stageAxisXColor CONSTANT)
    Q_PROPERTY(QColor stageAxisYColor READ stageAxisYColor CONSTANT)
    Q_PROPERTY(QColor screenNormalColor READ screenNormalColor CONSTANT)
    Q_PROPERTY(QColor screenHoverColor READ screenHoverColor CONSTANT)
    Q_PROPERTY(QColor screenSelectedColor READ screenSelectedColor CONSTANT)
    Q_PROPERTY(int screenBorderNormalWidth READ screenBorderNormalWidth CONSTANT)
    Q_PROPERTY(int screenBorderHoverWidth READ screenBorderHoverWidth CONSTANT)
    Q_PROPERTY(int screenBorderSelectedWidth READ screenBorderSelectedWidth CONSTANT)
    Q_PROPERTY(int stageAxisWidth READ stageAxisWidth CONSTANT)

    // 添加视图状态属性
    Q_PROPERTY(qreal zoomFactor READ zoomFactor WRITE setZoomFactor NOTIFY zoomFactorChanged)
    Q_PROPERTY(QPointF viewPosition READ viewPosition WRITE setViewPosition NOTIFY viewPositionChanged)

public:
    explicit TimelineStage(QQuickItem *parent = nullptr);
    ~TimelineStage();

    // Screen management
    Q_INVOKABLE void addScreen(TimelineScreen* screen);
    Q_INVOKABLE void removeScreen(TimelineScreen* screen);
    Q_INVOKABLE void clearScreens();
    Q_INVOKABLE QVector<TimelineScreen*> getScreens() const { return m_screens; }

    // Serialization
    QJsonObject save() const;
    void load(const QJsonObject &json);

    // Static method for QML registration
    static void registerType();

    // 添加样式访问器
    QColor stageAxisXColor() const { return stageAxisXColour; }
    QColor stageAxisYColor() const { return stageAxisYColour; }
    QColor screenNormalColor() const { return screenNormalColour; }
    QColor screenHoverColor() const { return screenHoverColour; }
    QColor screenSelectedColor() const { return screenSelectedColour; }
    int screenBorderNormalWidth() const { return ::screenBorderNormalWidth; }
    int screenBorderHoverWidth() const { return ::screenBorderHoverWidth; }
    int screenBorderSelectedWidth() const { return ::screenBorderSelectedWidth; }
    int stageAxisWidth() const { return ::stageAxisWidth; }

    // 添加背景色访问器
    QColor stageBgColor() const { return stageBgColour; }

    // 添加视图状态访问器
    qreal zoomFactor() const { return m_zoomFactor; }
    void setZoomFactor(qreal factor);
    
    QPointF viewPosition() const { return m_viewPosition; }
    void setViewPosition(const QPointF &pos);

signals:
    void screensChanged();
    void zoomFactorChanged();
    void viewPositionChanged();

private:
    QVector<TimelineScreen*> m_screens;      // 屏幕对象列表
    qreal m_zoomFactor = 1.0;
    QPointF m_viewPosition;
};

#endif // TIMELINESTAGE_HPP 