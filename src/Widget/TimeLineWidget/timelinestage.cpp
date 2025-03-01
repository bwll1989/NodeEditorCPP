#include "timelinestage.hpp"
#include <QJsonArray>
#include <QJsonObject>

TimelineStage::TimelineStage(QQuickItem *parent)
    : QQuickItem(parent)
{
    // 允许接收更新
    setFlag(QQuickItem::ItemHasContents, true);
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
    for (const auto* screen : m_screens) {
        if (auto* timelineScreen = qobject_cast<const TimelineScreen*>(screen)) {
            screensArray.append(timelineScreen->save());
        }
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
    // 注册为 QML 类型
    qmlRegisterType<TimelineStage>("TimelineWidget", 1, 0, "TimelineStage");
    
    // 注册为元对象类型，允许在 QML 中使用 TimelineStage* 类型
    qRegisterMetaType<TimelineStage*>("TimelineStage*");
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

