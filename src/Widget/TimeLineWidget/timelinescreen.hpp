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
#include "AbstractTimelineScreen.hpp"

class TimelineScreen : public AbstractTimelineScreen
{
    Q_OBJECT

public:
    explicit TimelineScreen(QWidget *parent = nullptr);
    ~TimelineScreen() override;

    // 实现 save 和 load 虚函数
    QJsonObject save() const override;
    void load(const QJsonObject &json) override;

    // Static method for QML registration
    static void registerType();

protected:
    // 实现抽象类的纯虚函数
    void createPropertiesWidget() override;
    void setImage(const QImage& image) override;
    void clear() override;
    void resizeEvent(QResizeEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    QRect getDisplayRect() const override;

protected slots:
    void onPropertyChanged() override;
    void updatePropertyWidgets() override;

private:
    QRect calculateDisplayRect() const;
};

#endif // TIMELINESCREEN_HPP 