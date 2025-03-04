#include "timelinesettingsdialog.hpp"
#include <QSettings>
#include <QGuiApplication>
#include <QScreen>
#include <limits>
#include "Widget/TimeLineWidget/timelinetypes.h"
#include <QMessageBox>

TimelineSettingsDialog::TimelineSettingsDialog(TimelineModel* model,QWidget* parent)
    : QDialog(parent),m_model(model)
{
    setWindowTitle(tr("Timeline设置"));
    setModal(true);
    createUI();
    setupConnections();
}

void TimelineSettingsDialog::createUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    
    // Create tab widget
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->addTab(createGeneralTab(), tr("通用"));
    m_tabWidget->addTab(createAudioTab(), tr("音频"));
    m_tabWidget->addTab(createVideoTab(), tr("视频"));
    m_tabWidget->setDocumentMode(true);
    mainLayout->addWidget(m_tabWidget);

    // Create buttons
    auto* buttonLayout = new QHBoxLayout;
    m_cancelButton = new QPushButton(tr("取消"), this);
    m_applyButton = new QPushButton(tr("应用"), this);
    
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addWidget(m_applyButton);
    
    mainLayout->addLayout(buttonLayout);
}

QWidget* TimelineSettingsDialog::createGeneralTab()
{
    auto* widget = new QWidget(this);
    auto* layout = new QFormLayout(widget);

    // FPS 设置 - 使用预定义的时间码格式
    m_fpsCombo = new QComboBox(widget);
    m_fpsCombo->addItem(tr("0: Film (24 fps)"), static_cast<int>(TimecodeType::Film));
    m_fpsCombo->addItem(tr("1: NTSC (30 fps)"), static_cast<int>(TimecodeType::NTSC));
    m_fpsCombo->addItem(tr("2: NTSC Drop Frame (29.97 fps)"), static_cast<int>(TimecodeType::NTSC_DF));
    m_fpsCombo->addItem(tr("3: PAL (25 fps)"), static_cast<int>(TimecodeType::PAL));
    m_fpsCombo->addItem(tr("4: HD 50 (50 fps)"), static_cast<int>(TimecodeType::HD_50));
    m_fpsCombo->addItem(tr("5: HD 60 (60 fps)"), static_cast<int>(TimecodeType::HD_60));
    m_fpsCombo->addItem(tr("6: HD 60 Drop Frame (59.94 fps)"), static_cast<int>(TimecodeType::HD_60_DF));
    m_fpsCombo->addItem(tr("7: HD 100 (100 fps)"), static_cast<int>(TimecodeType::HD_100));
    m_fpsCombo->addItem(tr("8: Game 120 (120 fps)"), static_cast<int>(TimecodeType::GAME_120));
    m_fpsCombo->addItem(tr("9: Game 144 (144 fps)"), static_cast<int>(TimecodeType::GAME_144));
    m_fpsCombo->addItem(tr("10: Game 240 (240 fps)"), static_cast<int>(TimecodeType::GAME_240));
    layout->addRow(tr("帧率:"), m_fpsCombo);
    // Auto-save settings
    m_autoSaveCheckBox = new QCheckBox(tr("启用自动保存"), widget);
    layout->addRow(m_autoSaveCheckBox);

    m_autoSaveIntervalSpinBox = new QSpinBox(widget);
    m_autoSaveIntervalSpinBox->setRange(1, 60);
    m_autoSaveIntervalSpinBox->setValue(5);
    m_autoSaveIntervalSpinBox->setSuffix(tr(" 分钟"));
    layout->addRow(tr("自动保存间隔:"), m_autoSaveIntervalSpinBox);

    // Time format settings
    m_timeFormatComboBox = new QComboBox(widget);
    m_timeFormatComboBox->addItem(tr("0: TimeCode (HH:MM:SS:FF)"),static_cast<int>(TimedisplayFormat::TimeCodeFormat));
    m_timeFormatComboBox->addItem(tr("1: TimeFormat (HH:MM:SS:MS)"),static_cast<int>(TimedisplayFormat::AbsoluteTimeFormat));
    
    layout->addRow(tr("时间显示格式:"), m_timeFormatComboBox);

    return widget;
}

QWidget* TimelineSettingsDialog::createAudioTab()
{
    auto* widget = new QWidget(this);
    auto* layout = new QFormLayout(widget);

    // Sample rate settings
    m_sampleRateCombo = new QComboBox(widget);
    m_sampleRateCombo->addItems({
        "44100",
        "48000",
        "96000",
        "192000"
    });
    layout->addRow(tr("采样率:"), m_sampleRateCombo);

    m_audioDeviceCombo = new QComboBox(widget);
    m_audioDeviceCombo->addItems(getAudioDeviceList());
    layout->addRow(tr("音频设备:"), m_audioDeviceCombo);


    return widget;
}

QWidget* TimelineSettingsDialog::createVideoTab()
{
    auto* widget = new QWidget(this);
    auto* layout = new QFormLayout(widget);
    // Resolution settings
    m_displayDeviceCombo = new QComboBox(widget);
    m_displayDeviceCombo->addItems(getDisplayDeviceList());
    layout->addRow(tr("显示器:"), m_displayDeviceCombo);

    // Aspect ratio settings
    m_aspectRatioSpinBox = new QDoubleSpinBox(widget);
    m_aspectRatioSpinBox->setRange(0.1, 10.0);
    m_aspectRatioSpinBox->setValue(1.778); // 16:9
    m_aspectRatioSpinBox->setDecimals(3);
    layout->addRow(tr("宽高比:"), m_aspectRatioSpinBox);

    // Hardware acceleration
    m_hardwareAccelCheckBox = new QCheckBox(tr("启用硬件加速"), widget);
    layout->addRow(m_hardwareAccelCheckBox);

    return widget;
}

void TimelineSettingsDialog::setupConnections()
{
   
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    
    connect(m_applyButton, &QPushButton::clicked, this, [this]() {
        saveSettings();
        accept();
        emit settingsChanged();
    });

    // Enable/disable auto-save interval based on checkbox
    connect(m_autoSaveCheckBox, &QCheckBox::toggled, 
            m_autoSaveIntervalSpinBox, &QSpinBox::setEnabled);
}

void TimelineSettingsDialog::saveSettings()
{
    // 获取新的时间码类型
    TimecodeType newTimecodeType = static_cast<TimecodeType>(m_fpsCombo->currentData().toInt());
    TimedisplayFormat newDisplayFormat = static_cast<TimedisplayFormat>(m_timeFormatComboBox->currentData().toInt());
    
    // 检查时间码类型是否改变
    if (m_model->getTimecodeType() != newTimecodeType) {
        // 显示警告对话框
        QMessageBox::StandardButton reply = QMessageBox::warning(
            this,
            tr("更改时间码格式"),
            tr("更改时间码格式将导致所有资源需要重新添加。\n是否继续？"),
            QMessageBox::Yes | QMessageBox::No
        );
        
        if (reply == QMessageBox::Yes) {
            // 用户确认更改
            m_model->setTimecodeType(newTimecodeType);
            m_model->setTimeDisplayFormat(newDisplayFormat);
            
            // 清除模型中的所有片段
            for(auto track:m_model->getTracks()){
                for(auto clip:track->getClips()){
                    track->removeClip(clip);
                }
            }
            m_model->onTimelineLengthChanged();
            emit settingsChanged();
            accept();
        }
    } else {
        // 只更改显示格式
        m_model->setTimeDisplayFormat(newDisplayFormat);
        emit settingsChanged();
        accept();
    }
}


void TimelineSettingsDialog::setSampleRate(int rate)
{
    int index = m_sampleRateCombo->findText(QString::number(rate));
    if (index >= 0) {
        m_sampleRateCombo->setCurrentIndex(index);
    }
}



QStringList TimelineSettingsDialog::getAudioDeviceList(){
        PaError err;
        int     i, numDevices;
        QStringList deviceList;
        err = Pa_Initialize();
//        widget->device_selector->clear();
        if( err != paNoError )
        {
            return deviceList;
            Pa_Terminate();
        }
        numDevices = Pa_GetDeviceCount();
        if( numDevices < 0 )
        {
            return deviceList;
            Pa_Terminate();
        }
        for( i=0; i<numDevices; i++ )
        {
            const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo( i );

            if (deviceInfo != nullptr && deviceInfo->maxOutputChannels>0)
            {
                deviceList << QString("%1: %2").arg(i).arg(deviceInfo->name);

            }
        }
        return deviceList;
    }

QStringList TimelineSettingsDialog::getDisplayDeviceList(){
    QStringList deviceList;
    //获取已连接的显示器
    QList<QScreen *> screens = QGuiApplication::screens();
    foreach (QScreen *screen, screens) {
        deviceList << QString("%1: %2 * %3").arg(screen->name()).arg(screen->geometry().size().width()).arg(screen->geometry().size().height());
    }
    return deviceList;
}

void TimelineSettingsDialog::syncSettings(){
    // 同步模型中设置
    m_fpsCombo->setCurrentIndex(static_cast<int>(m_model->getTimecodeGenerator()->getTimecodeType()));
    m_timeFormatComboBox->setCurrentIndex(static_cast<int>(m_model->getTimeDisplayFormat()));
}
