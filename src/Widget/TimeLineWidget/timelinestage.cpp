#include "timelinestage.hpp"
#include <QJsonArray>
#include <QJsonObject>

TimelineStage::TimelineStage(QQuickItem *parent)
    : QQuickItem(parent)
{
}

TimelineStage::~TimelineStage()
{
    clearScreens();
}

void TimelineStage::addScreen(TimelineScreen* screen)
{
    if (!m_screens.contains(screen)) {
        m_screens.append(screen);
        emit screensChanged();
    }
}

void TimelineStage::removeScreen(TimelineScreen* screen)
{
    if (m_screens.contains(screen)) {
        m_screens.removeOne(screen);
        
        screen->deleteLater();  // 使用 deleteLater 而不是直接删除
        emit screensChanged();
    }
}

void TimelineStage::clearScreens()
{
    // 使用 deleteLater 而不是直接删除
    for (auto* screen : m_screens) {
        screen->deleteLater();
    }
    m_screens.clear();
    emit screensChanged();
}

QJsonObject TimelineStage::save() const
{
    QJsonObject stageJson;
    
    // 保存屏幕信息
    QJsonArray screensArray;
    for (const TimelineScreen* screen : m_screens) {
        screensArray.append(screen->save());
    }
    stageJson["screens"] = screensArray;

    return stageJson;
}

void TimelineStage::load(const QJsonObject &json)
{
    clearScreens();

    // 加载屏幕信息
    QJsonArray screensArray = json["screens"].toArray();
    for (const QJsonValue &screenValue : screensArray) {
        QJsonObject screenJson = screenValue.toObject();
        TimelineScreen* screen = new TimelineScreen();
        screen->load(screenJson);
        m_screens.append(screen);
    }

    emit screensChanged();
}

void TimelineStage::registerType()
{
    qmlRegisterType<TimelineStage>("TimelineWidget", 1, 0, "TimelineStage");
}

void TimelineStage::setZoomFactor(qreal factor)
{
    if (qFuzzyCompare(m_zoomFactor, factor))
        return;
    m_zoomFactor = factor;
    emit zoomFactorChanged();
}

void TimelineStage::setViewPosition(const QPointF &pos)
{
    if (m_viewPosition == pos)
        return;
    m_viewPosition = pos;
    emit viewPositionChanged();
} 