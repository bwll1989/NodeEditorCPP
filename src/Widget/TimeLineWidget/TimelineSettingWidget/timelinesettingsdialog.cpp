#include "timelinesettingsdialog.hpp"
#include <QSettings>
#include <QGuiApplication>
#include <QScreen>
#include <limits>
#include "Widget/TimeLineWidget/TimelineAbstract/timelinetypes.h"
#include <QMessageBox>

TimelineSettingsDialog::TimelineSettingsDialog(TimelineModel* model,QWidget* parent)
    : QDialog(parent),m_model(model)
{
    setWindowTitle(tr("Timeline设置"));
    setModal(true);
    initAudioDeviceList();
    createUI();
    setupConnections();
    
}

TimelineSettingsDialog::~TimelineSettingsDialog()
{
    delete m_cancelButton;
    delete m_applyButton;
    delete m_tabWidget;
    
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
    m_timeFormatComboBox->addItem(tr("0: TimeCode (HH:MM:SS:FF)"),static_cast<int>(TimedisplayFormat::TimeCodeFormat));
    m_timeFormatComboBox->addItem(tr("1: TimeFormat (HH:MM:SS:MS)"),static_cast<int>(TimedisplayFormat::AbsoluteTimeFormat));
    
    layout->addRow(tr("时间显示格式:"), m_timeFormatComboBox);

    m_fpsCombo = new QComboBox(widget);
    m_fpsCombo->addItem(tr("0: Film (24 fps)"), static_cast<int>(TimeCodeType::Film));
    m_fpsCombo->addItem(tr("1: Film Drop Frame (23.976 fps)"), static_cast<int>(TimeCodeType::Film_DF));
    m_fpsCombo->addItem(tr("2: NTSC (30 fps)"), static_cast<int>(TimeCodeType::NTSC));
    m_fpsCombo->addItem(tr("3: NTSC Drop Frame (29.97 fps)"), static_cast<int>(TimeCodeType::NTSC_DF));
    m_fpsCombo->addItem(tr("4: PAL (25 fps)"), static_cast<int>(TimeCodeType::PAL));
    layout->addRow(tr("设置帧率:"), m_fpsCombo);
    m_fpsCombo->setCurrentIndex(static_cast<int>(m_model->getTimecodeGenerator()->getTimecodeType()));
    // 添加时钟源设置组
    auto* clockGroup = new QGroupBox(tr("时钟源设置"), widget);
    auto* clockLayout = new QVBoxLayout(clockGroup);

    // 时钟源选择

    m_clockSourceCombo = new QComboBox(widget);
    m_clockSourceCombo->addItem(tr("内部时钟"), static_cast<int>(ClockSource::Internal));
    m_clockSourceCombo->addItem(tr("LTC"), static_cast<int>(ClockSource::LTC));
    m_clockSourceCombo->addItem(tr("MTC"), static_cast<int>(ClockSource::MTC));
    clockLayout->addWidget(m_clockSourceCombo);

    // 创建时钟设置堆栈
    m_clockSettingsStack = new QStackedWidget(widget);

    // 内部时钟设置页面（可以为空或添加其他设置）
    auto* internalPage = new QWidget();
    m_clockSettingsStack->addWidget(internalPage);
    // LTC设置页面
    auto* ltcPage = new QWidget();
   
    // 音频设备选择
    auto* audioDeviceLayout = new QHBoxLayout(ltcPage);
    audioDeviceLayout->addWidget(new QLabel(tr("音频设备:")),1);
    m_LtcInputDeviceCombo = new QComboBox(ltcPage);
    m_LtcInputDeviceCombo->addItems(m_inputAudioDeviceList);
    audioDeviceLayout->addWidget(m_LtcInputDeviceCombo,3);

    m_clockSettingsStack->addWidget(ltcPage);

    // MTC设置页面
    auto* mtcPage = new QWidget();
    auto* mtcLayout = new QVBoxLayout(mtcPage);
    mtcLayout->addWidget(new QLabel(tr("MIDI设备设置")));
    m_clockSettingsStack->addWidget(mtcPage);

    clockLayout->addWidget(m_clockSettingsStack);
    layout->addRow(clockGroup);

    // 连接信号
    connect(m_clockSourceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
                // 切换设置页面
                m_clockSettingsStack->setCurrentIndex(index);               
            });

    // 同步当前设置
    ClockSource currentSource = m_model->getTimecodeGenerator()->getClockSource();
    int sourceIndex = m_clockSourceCombo->findData(static_cast<int>(currentSource));
    if (sourceIndex >= 0) {
        m_clockSourceCombo->setCurrentIndex(sourceIndex);
        m_clockSettingsStack->setCurrentIndex(sourceIndex);
    }

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
    m_audioDeviceCombo->addItems(m_outputAudioDeviceList);
    layout->addRow(tr("音频输出设备:"), m_audioDeviceCombo);


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
    TimeCodeType newTimecodeType = static_cast<TimeCodeType>(m_fpsCombo->currentData().toInt());
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
        }
    } 
        // 只更改显示格式
    m_model->setTimeDisplayFormat(newDisplayFormat);

    auto newClockSource = static_cast<ClockSource>(m_clockSourceCombo->currentData().toInt());
    if (newClockSource != m_model->getTimecodeGenerator()->getClockSource()) {
        m_model->getTimecodeGenerator()->setClockSource(newClockSource);
        
        // 如果时钟源改变，则需要重新初始化时钟
        if (newClockSource == ClockSource::LTC ) {
            
            m_model->getTimecodeGenerator()->initLTCClock(m_LtcInputDeviceCombo->currentText().split(":")[0].toInt());
            }
        
        if (newClockSource == ClockSource::MTC) {
            // MTC相关初始化代码
        }
    }
    
    emit settingsChanged();
    accept();
    
}

void TimelineSettingsDialog::setSampleRate(int rate)
{
    int index = m_sampleRateCombo->findText(QString::number(rate));
    if (index >= 0) {
        m_sampleRateCombo->setCurrentIndex(index);
    }
}

void TimelineSettingsDialog::initAudioDeviceList(){
        m_outputAudioDeviceList.clear();
        m_inputAudioDeviceList.clear();
        PaError err;
        int numDevices;
        err = Pa_Initialize();
        if(err != paNoError)
        {
            Pa_Terminate();
            return;
        }
        numDevices = Pa_GetDeviceCount();
        if(numDevices < 0)
        {
            Pa_Terminate();
            return;
        }
        
        // 获取默认输入和输出设备
        const PaDeviceInfo *defaultOutputInfo = Pa_GetDeviceInfo(Pa_GetDefaultOutputDevice());
        const PaDeviceInfo *defaultInputInfo = Pa_GetDeviceInfo(Pa_GetDefaultInputDevice());
        
        // 只添加有效的音频设备，避免重复和无效设备
        QSet<QString> addedOutputDevices;
        QSet<QString> addedInputDevices;
        
        for(int i = 0; i < numDevices; i++)
        {
            const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(i);
            
            if(deviceInfo == nullptr)
                continue;
                
            // 添加输出设备（避免重复名称）
            if(deviceInfo->maxOutputChannels > 0)
            {
                QString deviceName = QString::fromUtf8(deviceInfo->name);
                if(!addedOutputDevices.contains(deviceName))
                {
                    m_outputAudioDeviceList << QString("%1: %2").arg(i).arg(deviceName);
                    addedOutputDevices.insert(deviceName);
                }
            }
            
            // 添加输入设备（避免重复名称）
            if(deviceInfo->maxInputChannels > 0)
            {
                QString deviceName = QString::fromUtf8(deviceInfo->name);
                if(!addedInputDevices.contains(deviceName))
                {
                    m_inputAudioDeviceList << QString("%1: %2").arg(i).arg(deviceName);
                    addedInputDevices.insert(deviceName);
                }
            }
        }
        
        Pa_Terminate();
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

    // 同步时钟源设置
    ClockSource currentSource = m_model->getTimecodeGenerator()->getClockSource();
    int sourceIndex = m_clockSourceCombo->findData(static_cast<int>(currentSource));
    if (sourceIndex >= 0) {
        m_clockSourceCombo->setCurrentIndex(sourceIndex);
        m_clockSettingsStack->setCurrentIndex(sourceIndex);
    }
}
