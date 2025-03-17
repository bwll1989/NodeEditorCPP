#ifndef TRIGGERCLIPMODEL_H
#define TRIGGERCLIPMODEL_H

#include "TimeLineWidget/TimelineAbstract/AbstractClipModel.hpp"
#include <QJsonArray>
#include "../../Common/Devices/OSCSender/OSCSender.h"
class TriggerClipModel : public AbstractClipModel {
    Q_OBJECT
public:


    TriggerClipModel(int start, int end): AbstractClipModel(start, end, "Trigger")
    {
            // 触发器不可调整大小，不可显示小部件
       RESIZEABLE = false;
       EMBEDWIDGET = false;
       SHOWBORDER = false;
    }

    ~TriggerClipModel() override = default;
    // 重写保存和加载函数
    QJsonObject save() const override {
        QJsonObject json = AbstractClipModel::save();

        return json;
    }

    void load(const QJsonObject& json) override {
        AbstractClipModel::load(json);
       
    }

    QVariant data(int role) const override {
        switch (role) {
            case TimelineRoles::ClipModelRole:
                return QVariant::fromValue(static_cast<AbstractClipModel*>(const_cast<TriggerClipModel*>(this)));
            default:
                return AbstractClipModel::data(role);
        }
    }

    
    QVariantMap currentControlData(int currentFrame) const override {
        return QVariantMap();
    }

Q_SIGNALS:
    void triggered(const QString& actionName, const QJsonObject& parameters);

private:
    OSCSender* m_oscSender;
    QString m_oscHost;
    QString m_oscPort;
};

#endif // TRIGGERCLIPMODEL_H 