#ifndef MAPPINGCLIPMODEL_H
#define MAPPINGCLIPMODEL_H
#include <QPushButton>
#include "AbstractClipModel.hpp"
#include <QJsonArray>
#include "../../Common/Devices/OSCSender/OSCSender.h"
#include "OscListWidget/OSCMessageListWidget.hpp"
#include "OscListWidget/OSCMessageItemWidget.hpp"
namespace Clips
{
    class MappingClipModel : public AbstractClipModel {
        Q_OBJECT
    public:


        MappingClipModel(int start): AbstractClipModel(start,"Mapping"),
        m_editor(nullptr),
        m_listWidget(new OSCMessageListWidget(m_editor))
        {
            setEnd(start+100);
            // 片段正常颜色
            ClipColor=QColor("#b83b5e");
            initPropertyWidget();
        }
        ~MappingClipModel() override =default;
        // 重写保存和加载函数
        QJsonObject save() const override {
            QJsonObject json = AbstractClipModel::save();
            json["messages"] = m_listWidget->save();
            return json;
        }

        void load(const QJsonObject& json) override {
            AbstractClipModel::load(json);
            m_listWidget->load(json["messages"].toObject());

        }

        QVariant data(int role) const override {
            switch (role) {
            case TimelineRoles::ClipModelRole:
                return QVariant::fromValue(static_cast<AbstractClipModel*>(const_cast<MappingClipModel*>(this)));
            default:
                return AbstractClipModel::data(role);
            }
        }


        QVariantMap currentData(int currentFrame) const override {
            if(currentFrame >= m_start && currentFrame <= m_end && m_currentFrame!=currentFrame){
                float value = static_cast<float>(currentFrame - m_start)/(m_end-m_start); // 修正分母为时间区间长度
                m_currentFrame = currentFrame;
                for (int i = 0; i < m_listWidget->count(); ++i)
                {
                    QListWidgetItem* currentItem = m_listWidget->item(i);

                    if (!currentItem) {
                        continue;
                    }
                    auto* widget = static_cast<OSCMessageItemWidget*>(m_listWidget->itemWidget(currentItem));
                    // widget->getValue()
                    widget->getJSEngine()->globalObject().setProperty("$percent", value);
                    widget->getJSEngine()->globalObject().setProperty("$frame", m_currentFrame-m_start);

                }

                const_cast<MappingClipModel*>(this)->trigger();
            }
            return QVariantMap();
        }


        QWidget* clipPropertyWidget() override{
            m_editor = new QWidget();
            QVBoxLayout* mainLayout = new QVBoxLayout(m_editor);
            mainLayout->setContentsMargins(5, 5, 5, 5);
            mainLayout->setSpacing(4);
            // OSC消息设置
            auto* playGroup = new QGroupBox("Mapping设置", m_editor);
            auto* playLayout = new QVBoxLayout(playGroup);
            playLayout->setContentsMargins(0, 0, 0, 0);
            playLayout->addWidget(m_listWidget);
            mainLayout->addWidget(playGroup);
            return m_editor;
        }

        void trigger(){
            auto messages = m_listWidget->getOSCMessages();
            for(auto message : messages){
                OSCSender::instance()->sendOSCMessageWithQueue(message);
            }
        }



    private:
        QWidget* m_editor;
        OSCMessageListWidget* m_listWidget;
        mutable int m_currentFrame;
    };
}
#endif // MAPPINGCLIPMODEL_H