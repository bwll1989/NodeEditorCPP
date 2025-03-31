#ifndef LTCSETTINGWIDGET_HPP
#define LTCSETTINGWIDGET_HPP
#include <QWidget>
#include <QFormLayout>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include "portaudio.h"
#include <QSet>
#include <QJsonObject>

class LtcSettingWidget : public QWidget {
    Q_OBJECT
public:
    explicit LtcSettingWidget(QStringList m_inputAudioDeviceList, QWidget* parent = nullptr)
        : QWidget(parent),
          m_inputAudioDeviceList(m_inputAudioDeviceList),
          m_LtcInputDeviceCombo(new QComboBox(this)),
          m_LtcInputChannelCombo(new QComboBox(this)),
          m_LtcInputSampleRateCombo(new QComboBox(this))
    {
        // 如果没有输入设备，添加一个默认选项
        if (m_inputAudioDeviceList.isEmpty()) {
            m_inputAudioDeviceList << tr("无可用设备");
        }
        
        createUI();
        
        // 确保通道下拉框至少有一个选项
        if (m_LtcInputChannelCombo->count() == 0) {
            m_LtcInputChannelCombo->addItem("0");
        }
    }
    virtual ~LtcSettingWidget() override=default;
   
public:
    QJsonObject getLtcSetting() {
        QJsonObject ltcSetting;
        
        // 检查设备下拉框是否有选中项
        if (m_LtcInputDeviceCombo && m_LtcInputDeviceCombo->currentIndex() != -1) {
            ltcSetting["device"] = m_LtcInputDeviceCombo->currentText();
        } else {
            ltcSetting["device"] = QString();
        }
        
        // 检查通道下拉框是否有选中项
        if (m_LtcInputChannelCombo && m_LtcInputChannelCombo->currentIndex() != -1) {
            ltcSetting["channel"] = m_LtcInputChannelCombo->currentIndex();
        } else {
            ltcSetting["channel"] = 0;
        }

        // qDebug() << "ltcSetting" <<m_LtcInputDeviceCombo->currentIndex();
        return ltcSetting;
    }

    void setLtcSetting(const QJsonObject& setting){
        m_LtcSetting = setting;
        m_LtcInputDeviceCombo->setCurrentText(setting["device"].toString());
        m_LtcInputChannelCombo->setCurrentIndex(setting["channel"].toInt());
    }
    
   

    int getInputAudioDeviceChannel(const int *deviceIndex){
        if(deviceIndex == nullptr)
            return 0;
        const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(*deviceIndex);
        if(deviceInfo == nullptr)
            return 0;
        return deviceInfo->maxInputChannels;
    }

private:
    void createUI(){
        auto* audioDeviceLayout = new QFormLayout(this);
        m_LtcInputDeviceCombo->addItems(m_inputAudioDeviceList);
        audioDeviceLayout->addRow(tr("音频设备:"),m_LtcInputDeviceCombo);
        audioDeviceLayout->addRow(tr("音频通道:"),m_LtcInputChannelCombo);
        connect(m_LtcInputDeviceCombo,&QComboBox::currentTextChanged,this,[this](const QString &text){
            int deviceIndex = text.split(":")[0].toInt();
            int channelCount = getInputAudioDeviceChannel(&deviceIndex);
            m_LtcInputChannelCombo->clear();
            for(int i = 0; i < channelCount; i++){
                m_LtcInputChannelCombo->addItem(QString::number(i));
            }
        });
        m_LtcInputSampleRateCombo->addItems({"44100","48000"});
        audioDeviceLayout->addRow(tr("音频采样率:"),m_LtcInputSampleRateCombo);
    }
    
    QComboBox* m_LtcInputDeviceCombo;
    QComboBox* m_LtcInputChannelCombo;
    QComboBox* m_LtcInputSampleRateCombo;
    QJsonObject m_LtcSetting;
    QStringList m_inputAudioDeviceList;
};
#endif // LTCSETTINGWIDGET_HPP



