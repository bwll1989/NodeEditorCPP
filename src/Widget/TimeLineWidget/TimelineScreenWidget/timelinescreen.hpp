#ifndef TIMELINESCREEN_HPP
#define TIMELINESCREEN_HPP

#include <QQuickItem>
#include <QQmlEngine>
#include <QJsonObject>
#include <QObject>
#include <QWidget>
#include <QColor>
#include <QDialog>
#include <QSpinBox>
#include <QSlider>
#include <QDoubleSpinBox>
#include "Widget/TimeLineWidget/TimelineAbstract/AbstractTimelineScreen.hpp"
class TimelineScreen : public AbstractTimelineScreen
{
    Q_OBJECT

public:
    explicit TimelineScreen(QWidget *parent = nullptr);
    ~TimelineScreen() override;

    /**
     * 保存
     * @return QJsonObject 保存的json对象
     */
    QJsonObject save() const override;
    /**
     * 加载
     * @param const QJsonObject &json 加载的json对象
     */
    void load(const QJsonObject &json) override;

    /**
     * 注册类型
     */
    static void registerType();


    

protected:
    /**
     * 创建属性窗口
     */
    void createPropertiesWidget() override;
    /**
     * 设置图像
     * @param const QImage& image 图像
     */
    void setImage(const QImage& image) override;
    /**
     * 清除
     */
    void clear() override;
    /**
     * 调整大小事件
     * @param QResizeEvent* event 调整大小事件
     */
    void resizeEvent(QResizeEvent* event) override;
    /**
     * 绘制事件
     * @param QPaintEvent* event 绘制事件
     */
    void paintEvent(QPaintEvent* event) override;
    /**
     * 获取显示矩形
     * @return QRect 显示矩形
     */
    QRect getDisplayRect() const override;

protected slots:
    /**
     * 属性变化事件
     */
    void onPropertyChanged() override;
    /**
     * 更新属性窗口
     */
    void updatePropertyWidgets() override;

private:
    /**
     * 计算显示矩形
     * @return QRect 显示矩形
     */
    QRect calculateDisplayRect() const;

};

#endif // TIMELINESCREEN_HPP 