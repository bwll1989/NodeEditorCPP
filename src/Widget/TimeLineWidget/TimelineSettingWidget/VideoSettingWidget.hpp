#ifndef VIDEOSSETTINGWIDGET_HPP
#define VIDEOSSETTINGWIDGET_HPP
#include <QWidget>
#include <QFormLayout>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include "portaudio.h"
#include <QSet>

class VideoSettingWidget : public QWidget {
    Q_OBJECT
public:
    explicit VideoSettingWidget(QWidget* parent = nullptr,QStringList m_videoDeviceList=QStringList()):QWidget(parent),
    m_videoDeviceList(m_videoDeviceList)
    {
        createUI();
    }
    virtual ~VideoSettingWidget() override=default;
    QComboBox* m_videoOutputDeviceCombo;

public:
    QJsonObject getVideoSetting(){
        QJsonObject videoSetting;
        videoSetting["device"] = m_videoOutputDeviceCombo->currentText();
        return videoSetting;
    }
    void setVideoSetting(const QJsonObject& setting){
        m_videoSetting = setting;
        m_videoOutputDeviceCombo->setCurrentText(setting["device"].toString());
    }
    

private:
    void createUI(){
        auto* audioDeviceLayout = new QFormLayout(this);
        m_videoOutputDeviceCombo = new QComboBox(this);
        m_videoOutputDeviceCombo->addItems(m_videoDeviceList);
        audioDeviceLayout->addRow(tr("视频输出设备:"),m_videoOutputDeviceCombo);
    }
    QJsonObject m_videoSetting;
    QStringList m_videoDeviceList;
};
#endif // VIDEOSSETTINGWIDGET_HPP



