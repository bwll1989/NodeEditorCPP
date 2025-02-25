#ifndef TRIGGERCLIPMODEL_H
#define TRIGGERCLIPMODEL_H

#include "TimeLineWidget/AbstractClipModel.hpp"
#include <QJsonArray>

class TriggerClipModel : public AbstractClipModel {
    Q_OBJECT
public:


    TriggerClipModel(int start, int end): AbstractClipModel(start, end, "Trigger")
    {
            // 触发器不可调整大小，不可显示小部件
       RESIZEABLE = false;
       EMBEDWIDGET = true;
       SHOWBORDER = false;
    }

    ~TriggerClipModel() override = default;
    // 重写保存和加载函数
    QJsonObject save() const override {
        QJsonObject json = AbstractClipModel::save();
        json["actionName"] = m_actionName;
        json["parameters"] = m_parameters;
        return json;
    }

    void load(const QJsonObject& json) override {
        AbstractClipModel::load(json);
        m_actionName = json["actionName"].toString();
        m_parameters = json["parameters"].toObject();
    }

    QVariant data(int role) const override {
        if (role == Qt::DisplayRole) {
            return m_actionName;
        }
        return QVariant();
    }
    
    QVariant currentData(int currentFrame) const override {
        return QVariant();
    }

Q_SIGNALS:
    void triggered(const QString& actionName, const QJsonObject& parameters);

private:
    QString m_actionName;                  // 动作名称
    QJsonObject m_parameters;              // 动作参数
};

#endif // TRIGGERCLIPMODEL_H 