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
#include "Widget/TimeLineWidget/TimelineAbstract/timelinetypes.h"
#include "Widget/TimeLineWidget/TimelineMVC/timelinemodel.hpp"
#include <QMessageBox>

class TimelineSettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit TimelineSettingsDialog(TimelineModel* model,QWidget* parent = nullptr);
    ~TimelineSettingsDialog() override = default;

    // // 通用设置
    bool getAutoSave() const { return m_autoSaveCheckBox->isChecked(); }
    int getAutoSaveInterval() const { return m_autoSaveIntervalSpinBox->value(); }
    QString getTimeFormat() const { return m_timeFormatComboBox->currentText(); }

    // 音频相关设置
    int getSampleRate() const { return m_sampleRateCombo->currentText().toInt(); }

    // 视频相关设置
   
    double getAspectRatio() const { return m_aspectRatioSpinBox->value(); }
    bool getHardwareAcceleration() const { return m_hardwareAccelCheckBox->isChecked(); }

    // 获取当前选择的时间码类型
    TimecodeType getTimecodeType() const {
        return static_cast<TimecodeType>(m_fpsCombo->currentData().toInt());
    }
    // 同步模型中设置
    void syncSettings();

    // 设置器
    void setAutoSave(bool enabled) { m_autoSaveCheckBox->setChecked(enabled); }
    void setAutoSaveInterval(int minutes) { m_autoSaveIntervalSpinBox->setValue(minutes); }
    void setSampleRate(int rate);
    void setAspectRatio(double ratio) { m_aspectRatioSpinBox->setValue(ratio); }
    void setHardwareAcceleration(bool enabled) { m_hardwareAccelCheckBox->setChecked(enabled); }

    QStringList getAudioDeviceList();
    QStringList getDisplayDeviceList();
signals:
    void settingsChanged();

private:
    void createUI();
    void setupConnections();
    void saveSettings();
    
    QWidget* createGeneralTab();
    QWidget* createAudioTab();
    QWidget* createVideoTab();

private:
    QTabWidget* m_tabWidget;

    // 通用设置中的控件
    QCheckBox* m_autoSaveCheckBox;
    QSpinBox* m_autoSaveIntervalSpinBox;
    QComboBox* m_timeFormatComboBox;

    // 音频设置中的控件
    QComboBox* m_sampleRateCombo;
    QComboBox* m_audioDeviceCombo;

    // 视频设置中的控件
    QComboBox* m_displayDeviceCombo;
    QDoubleSpinBox* m_aspectRatioSpinBox;
    QCheckBox* m_hardwareAccelCheckBox;

    // 对话框中的按钮
    QPushButton* m_cancelButton;
    QPushButton* m_applyButton;

    QComboBox* m_fpsCombo;  // 帧率选择下拉框

    TimelineModel* m_model;
    
};

#endif // TIMELINESETTINGSDIALOG_HPP 