#ifndef MTCSETTINGWIDGET_HPP
#define MTCSETTINGWIDGET_HPP
#include <QWidget>
#include <QFormLayout>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include "portaudio.h"
#include <QSet>
#include <QJsonObject>
class MtcSettingWidget : public QWidget {
    Q_OBJECT
public:
    explicit MtcSettingWidget(QWidget* parent = nullptr,QStringList m_mtcDeviceList=QStringList()):QWidget(parent),
    m_MtcDeviceList(m_mtcDeviceList)
    {
        createUI();
    }
    virtual ~MtcSettingWidget() override=default;
    QComboBox* m_MtcInputDeviceCombo;

public:
    QJsonObject getMtcSetting(){
        QJsonObject mtcSetting;
        mtcSetting["device"] = m_MtcInputDeviceCombo->currentText();
        return mtcSetting;
    }
    void setMtcSetting(const QJsonObject& setting){
        m_MtcSetting = setting;
        m_MtcInputDeviceCombo->setCurrentText(setting["device"].toString());
    }
    

private:
    void createUI(){
        auto* audioDeviceLayout = new QFormLayout(this);
        m_MtcInputDeviceCombo = new QComboBox(this);
        audioDeviceLayout->addRow(tr("MIDI设备:"),m_MtcInputDeviceCombo);
        m_MtcInputDeviceCombo->addItems(m_MtcDeviceList);
    }
    QJsonObject m_MtcSetting;
    QStringList m_MtcDeviceList;
};
#endif // LTCSETTINGWIDGET_HPP



