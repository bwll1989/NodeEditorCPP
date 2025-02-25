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

    /**
     * 添加屏幕
     * @param TimelineScreen* screen 屏幕
     */
    Q_INVOKABLE void addScreen(TimelineScreen* screen);
    /**
     * 删除屏幕
     * @param TimelineScreen* screen 屏幕
     */
    Q_INVOKABLE void removeScreen(TimelineScreen* screen);
    /**
     * 清除屏幕
     */
    Q_INVOKABLE void clearScreens();
    /**
     * 获取屏幕
     * @return QVector<TimelineScreen*> 屏幕列表
     */
    Q_INVOKABLE QVector<TimelineScreen*> getScreens() const { return m_screens; }

    /**
     * 保存
     * @return QJsonObject 保存的json对象
     */
    QJsonObject save() const;
    /**
     * 加载
     * @param const QJsonObject &json 加载的json对象
     */
    void load(const QJsonObject &json);
    /**
     * 注册类型
     */
    static void registerType();

    // 添加样式访问器
    /**
     * 获取轴x颜色
     * @return QColor 轴x颜色
     */
    QColor stageAxisXColor() const { return stageAxisXColour; }
    /**
     * 获取轴y颜色
     * @return QColor 轴y颜色
     */
    QColor stageAxisYColor() const { return stageAxisYColour; }
    /**
     * 获取屏幕正常颜色
     * @return QColor 屏幕正常颜色
     */
    QColor screenNormalColor() const { return screenNormalColour; }
    /**
     * 获取屏幕悬停颜色
     * @return QColor 屏幕悬停颜色
     */
    QColor screenHoverColor() const { return screenHoverColour; }
    /**
     * 获取屏幕选中颜色
     * @return QColor 屏幕选中颜色
     */
    QColor screenSelectedColor() const { return screenSelectedColour; }
    /**
     * 获取屏幕边框正常宽度
     * @return int 屏幕边框正常宽度
     */
    int screenBorderNormalWidth() const { return ::screenBorderNormalWidth; }
    /**
     * 获取屏幕边框悬停宽度
     * @return int 屏幕边框悬停宽度
     */
    int screenBorderHoverWidth() const { return ::screenBorderHoverWidth; }
    /**
     * 获取屏幕边框选中宽度
     * @return int 屏幕边框选中宽度
     */
    int screenBorderSelectedWidth() const { return ::screenBorderSelectedWidth; }
    /**
     * 获取轴宽度
     * @return int 轴宽度
     */
    int stageAxisWidth() const { return ::stageAxisWidth; }
    /**
     * 获取背景色
     * @return QColor 背景色
     */
    QColor stageBgColor() const { return stageBgColour; }

    /**
     * 获取缩放因子
     * @return qreal 缩放因子
     */
    qreal zoomFactor() const { return m_zoomFactor; }
    /**
     * 设置缩放因子
     * @param qreal factor 缩放因子
     */
    void setZoomFactor(qreal factor);
    /**
     * 获取视图位置
     * @return QPointF 视图位置
     */
    QPointF viewPosition() const { return m_viewPosition; }
    /**
     * 设置视图位置
     * @param const QPointF &pos 视图位置
     */
    void setViewPosition(const QPointF &pos);

signals:
    /**
     * 屏幕改变信号
     */
    void screensChanged();
    /**
     * 缩放因子改变信号
     */
    void zoomFactorChanged();
    /**
     * 视图位置改变信号
     */
    void viewPositionChanged();

private:
    //屏幕对象列表
    QVector<TimelineScreen*> m_screens;
    //缩放因子
    qreal m_zoomFactor = 1.0;
    //视图位置
    QPointF m_viewPosition;
};

#endif // TIMELINESTAGE_HPP 