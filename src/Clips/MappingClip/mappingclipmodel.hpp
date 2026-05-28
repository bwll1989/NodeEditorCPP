#ifndef MAPPINGCLIPMODEL_H
#define MAPPINGCLIPMODEL_H
#include <QPushButton>
#include "AbstractClipModel.hpp"
#include <QJsonArray>
#include "../../Common/Devices/OSCSender/OSCSender.h"
#include "OscListWidget/OSCMessageListWidget.hpp"
#include "OscListWidget/OSCMessageItemWidget.hpp"
#include "../../Common/BaseClass/AbstractClipDelegateModel.h"
namespace Clips
{
    class MappingClipModel : public AbstractClipDelegateModel {
        Q_OBJECT
    public:


        MappingClipModel(int start): AbstractClipDelegateModel(start,"Mapping"),
                                    m_editor(nullptr),
                                    m_listWidget(new OSCMessageListWidget(true,m_editor))
        {
            m_end=start+100;
            // 片段正常颜色
            ClipColor=QColor("#b83b5e");
            initPropertyWidget();
        }
        ~MappingClipModel() override =default;
        // 重写保存和加载函数
        QJsonObject save() const override {
            QJsonObject json = AbstractClipModel::save();
            auto arr = m_listWidget->save()["messages"].toArray();
            json["messages"] = arr;
            return json;
        }

        void load(const QJsonObject& json) override {
            AbstractClipModel::load(json);
             m_listWidget->load(json);
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
                    OSCMessageItemWidget* widget = m_listWidget->item(i);

                    if (!widget) {
                        continue;
                    }
                    // auto* widget = static_cast<OSCMessageItemWidget*>(m_listWidget->itemWidget(currentItem));
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
            playGroup->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            return m_editor;
        }

        void trigger(){
            auto messages = m_listWidget->getOSCMessages();
            for(auto message : messages){

                StatusContainer::instance()->parseOSC(message);
                // OSCSender::instance()->sendOSCMessageWithQueue(message);
            }
        }


public slots:
        /**
         * 函数级注释：处理来自全局事件总线的外部命令，调用业务槽并阻断控件信号
         */
        void onGlobalEvent(const GlobalEvent& ev){
            if (ev.kind != GlobalEventKind::Command) {
                return;
            }

            const QString addrStartFrame = makeFullOscAddress("/start_frame");
            const QString addrEndFrame = makeFullOscAddress("/end_frame");
            const QString addrPositionFrame = makeFullOscAddress("/position_frame");
            const QString addrSend = makeFullOscAddress("/send");
            if (ev.address == addrStartFrame && timeGroupBox->m_startFrameSpinBox) {
                bool ok = false;
                int v = ev.payload.toInt(&ok);
                if (!ok) return;
                QSignalBlocker blocker(timeGroupBox->m_startFrameSpinBox);
                setStart(v);
                timeGroupBox->m_startFrameSpinBox->setValue(v);
            } else if (ev.address == addrEndFrame && timeGroupBox->m_endFrameSpinBox) {
                bool ok = false;
                int v = ev.payload.toInt(&ok);
                if (!ok) return;
                QSignalBlocker blocker(timeGroupBox->m_endFrameSpinBox);
                setEnd(v);
                timeGroupBox->m_endFrameSpinBox->setValue(v);
            } else if (ev.address == addrPositionFrame && timeGroupBox->m_endFrameSpinBox&&timeGroupBox->m_startFrameSpinBox) {
                bool ok = false;
                int v = ev.payload.toInt(&ok);
                if (!ok) return;
                int len = length();
                QSignalBlocker blocker(timeGroupBox->m_startFrameSpinBox);
                QSignalBlocker blocker2(timeGroupBox->m_endFrameSpinBox);
                setStart(v);
                setEnd(v+len);
                timeGroupBox->m_startFrameSpinBox->setValue(v);
                timeGroupBox->m_endFrameSpinBox->setValue(v+len);
            }
        }

    protected:
        /**
         * 函数级注释：剪辑模型初始化完成后订阅位置与尺寸相关的外部命令
         */
        void afterModelReady() override{

             GlobalEventBus::instance()->subscribe(
                makeFullOscAddress("/start_frame"),
                this,
                SLOT(onGlobalEvent(GlobalEvent))
            );
            GlobalEventBus::instance()->subscribe(
               makeFullOscAddress("/end_frame"),
               this,
               SLOT(onGlobalEvent(GlobalEvent))
           );
            GlobalEventBus::instance()->subscribe(
               makeFullOscAddress("/position_frame"),
               this,
               SLOT(onGlobalEvent(GlobalEvent))
           );

        }
    private:
        QWidget* m_editor;
        OSCMessageListWidget* m_listWidget;
        mutable int m_currentFrame;
    };
}
#endif // MAPPINGCLIPMODEL_H