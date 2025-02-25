#include "timelinesettingsdialog.hpp"
#include <QSettings>
#include <QGuiApplication>
#include <QScreen>
TimelineSettingsDialog::TimelineSettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Timeline设置"));
    setModal(true);
    createUI();
    setupConnections();
    loadSettings();
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
    m_timeFormatComboBox->addItems({
        tr("Frames"),
        tr("Seconds"),
        tr("Timecode (HH:MM:SS:FF)")
    });
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
// FPS settings
    m_fpsSpinBox = new QSpinBox(widget);
    m_fpsSpinBox->setRange(1, 120);
    m_fpsSpinBox->setValue(30);
    m_fpsSpinBox->setSuffix(tr(" fps"));
    layout->addRow(tr("帧率:"), m_fpsSpinBox);
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

void TimelineSettingsDialog::loadSettings()
{
   
}

void TimelineSettingsDialog::saveSettings()
{
    
    
    emit settingsChanged();
}

void TimelineSettingsDialog::setTimeFormat(const QString& format)
{
    int index = m_timeFormatComboBox->findText(format);
    if (index >= 0) {
        m_timeFormatComboBox->setCurrentIndex(index);
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