#ifndef AUDIOSETTINGWIDGET_HPP
#define AUDIOSETTINGWIDGET_HPP
#include <QWidget>
#include <QFormLayout>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include "portaudio.h"
#include <QSet>
#include <QJsonObject>
class AudioSettingWidget : public QWidget {
    Q_OBJECT
public:
    explicit AudioSettingWidget(QWidget* parent = nullptr,QStringList m_audioDeviceList=QStringList()):QWidget(parent),
    m_audioDeviceList(m_audioDeviceList)
    {
        createUI();
    }
    virtual ~AudioSettingWidget() override=default;
    QComboBox* m_audioOutputDeviceCombo;
    QComboBox* m_audioSampleRateCombo;
public:
    QJsonObject getAudioSetting(){
        QJsonObject audioSetting;
        audioSetting["device"] = m_audioOutputDeviceCombo->currentText();
        audioSetting["deviceIndex"] = m_audioOutputDeviceCombo->currentIndex();
        audioSetting["sampleRate"] = m_audioSampleRateCombo->currentIndex();
        return audioSetting;
    }
    void setAudioSetting(const QJsonObject& setting){
        m_audioSetting = setting;
        m_audioOutputDeviceCombo->setCurrentText(setting["device"].toString());
        m_audioOutputDeviceCombo->setCurrentIndex(setting["deviceIndex"].toInt());
        m_audioSampleRateCombo->setCurrentIndex(setting["sampleRate"].toInt());
    }
    

private:
    void createUI(){
        auto* audioDeviceLayout = new QFormLayout(this);
        
        m_audioOutputDeviceCombo = new QComboBox(this);
        m_audioOutputDeviceCombo->addItems(m_audioDeviceList);
        audioDeviceLayout->addRow(tr("音频输出设备:"),m_audioOutputDeviceCombo);
        m_audioSampleRateCombo = new QComboBox(this);
        m_audioSampleRateCombo->addItems({"44100","48000"});
        audioDeviceLayout->addRow(tr("音频采样率:"),m_audioSampleRateCombo);
    }
    QJsonObject m_audioSetting;
    QStringList m_audioDeviceList;
};
#endif // AUDIOSETTINGWIDGET_HPP



