#ifndef TIMELINESETTINGSDIALOG_HPP
#define TIMELINESETTINGSDIALOG_HPP

#include <QDialog>
#include <QSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QTabWidget>
#include <QDoubleSpinBox>
#include "portaudio.h"
#include "timelinedefines.h"
#include "../TimeLineModel.h"
#include <QMessageBox>
#include <QStackedWidget>
#include "TimeCodeDefines.h"
#include "LtcSettingWidget.hpp"
#include "MtcSettingWidget.hpp"
#include "AudioSettingWidget.hpp"
#include "VideoSettingWidget.hpp"
class TimeLineSetting : public QDialog {
    Q_OBJECT
public:
    explicit TimeLineSetting(TimeLineModel* model,QWidget* parent = nullptr);
    ~TimeLineSetting() override;

    // // 通用设置
    bool getAutoSave() const { return m_autoSaveCheckBox->isChecked(); }
    int getAutoSaveInterval() const { return m_autoSaveIntervalSpinBox->value(); }
    QString getTimeFormat() const { return m_timeFormatComboBox->currentText(); }

    // 视频相关设置
    double getAspectRatio() const { return m_aspectRatioSpinBox->value(); }
    bool getHardwareAcceleration() const { return m_hardwareAccelCheckBox->isChecked(); }

    // 获取当前选择的时间码类型
    TimeCodeType getTimecodeType() const {
        return static_cast<TimeCodeType>(m_fpsCombo->currentData().toInt());
    }
    // 同步模型中设置
    void syncSettings();

    // 设置器
    void setAutoSave(bool enabled) { m_autoSaveCheckBox->setChecked(enabled); }
    void setAutoSaveInterval(int minutes) { m_autoSaveIntervalSpinBox->setValue(minutes); }
    void setAspectRatio(double ratio) { m_aspectRatioSpinBox->setValue(ratio); }
    void setHardwareAcceleration(bool enabled) { m_hardwareAccelCheckBox->setChecked(enabled); }
    void initDisplayDeviceList();
    void initAudioDeviceList();
    void saveSettings();
signals:
    void settingsChanged();
private:
    void createUI();
    void setupConnections();

    
    QWidget* createGeneralTab();
   

private:
    QTabWidget* m_tabWidget;

    // 通用设置中的控件
    QCheckBox* m_autoSaveCheckBox;
    QSpinBox* m_autoSaveIntervalSpinBox;
    QComboBox* m_timeFormatComboBox;

    // 音频设置中的控件
   

    // 视频设置中的控件
    QComboBox* m_displayDeviceCombo;
    QDoubleSpinBox* m_aspectRatioSpinBox;
    QCheckBox* m_hardwareAccelCheckBox;

    // 对话框中的按钮
    QPushButton* m_cancelButton;
    QPushButton* m_applyButton;

    QComboBox* m_fpsCombo;  // 帧率选择下拉框


    QComboBox* m_timecodeTypeCombo;

    QComboBox* m_clockSourceCombo;  // 时钟源选择

    QStackedWidget* m_clockSettingsStack;  // 不同时钟源的设置面板

    TimeLineModel* m_model;
    // 输出音频设备列表
    QStringList m_outputAudioDeviceList;
    // 输入音频设备列表
    QStringList m_inputAudioDeviceList;
    // 输出视频设备列表
    QStringList m_outputVideoDeviceList;

    LtcSettingWidget* m_ltcSettingWidget;
    MtcSettingWidget* m_mtcSettingWidget;
    AudioSettingWidget* m_audioSettingWidget;
    VideoSettingWidget* m_videoSettingWidget;
};

#endif // TIMELINESETTINGSDIALOG_HPP 