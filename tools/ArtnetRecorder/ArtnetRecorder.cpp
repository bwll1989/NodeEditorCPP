#include "ArtnetRecorder.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QSizePolicy>
#include <QHBoxLayout>
#include <QStringList>
#include "Elements/IntDragValueWidget/IntDragValueWidget.hpp"
#include "Defines.h"

// ===================== Flow 品牌色板 =====================
// 取值遵循 docs/brand/Flow-Brand-Visual-Identity-Manual.html：
//  - 纸白与墨绿黑承担版面（60/30/10），流动绿 #79AE31 只做强调；
//  - 避免纯白 #FFFFFF 作背景基色，小字禁止排在流动绿上（用墨色文字）；
//  - 语义色：成功 #4C8C27 / 警告 #C98A1F / 错误 #C4432E；
//  - 深色监视面用墨绿黑 #141A0D + 浅文字 #F2F4EA；
//  - 间距为 4px 的倍数，卡片圆角 6px。
namespace {

const QColor kPaper      ("#FAFAF6");  // 纸白 · 窗口背景基色
const QColor kPaper2     ("#F1F2E9");  // 纸白 2 · 输入/卡片底
const QColor kPaper3     ("#E8EADB");  // 纸白 3 · 悬停底
const QColor kLine       ("#DFE2D3");  // 细分隔线
const QColor kLineStrong ("#C6CBBA");  // 输入描边
const QColor kInk        ("#1B2113");  // 墨绿黑 · 主文字
const QColor kInk2       ("#5C6353");  // 次级文字
const QColor kInk3       ("#9BA294");  // 弱文字
const QColor kInkDark    ("#141A0D");  // 绿底上的深色文字
const QColor kGreen      ("#79AE31");  // 流动绿 500 · 主强调
const QColor kGreen600   ("#629327");  // 主按钮悬停
const QColor kGreen700   ("#4A7021");  // 主按钮按下 / kicker 文字
const QColor kGreenBright("#8FCB3F");  // 亮绿 · 暗底时间码
const QColor kGreen100   ("#E8F1D2");  // 绿 100 · 状态条底
const QColor kGreen300   ("#B6D47B");  // 绿 300 · 状态条描边
const QColor kSuccess    ("#4C8C27");  // 语义·成功（信号正常）
const QColor kWarning    ("#C98A1F");  // 语义·警告（等待非零信号）
const QColor kError      ("#C4432E");  // 语义·错误（录制中）
const QColor kCover      ("#141A0D");  // 墨绿黑 · 时间码显示屏底
const QColor kCoverInk   ("#F2F4EA");  // 深色底上的浅文字
const QColor kCoverLine  ("#2C3520");  // 深色底描边（暗色主题线色）

/**
 * @brief 统一按钮样式（品牌圆角 6px，悬停/按下用同色系深一档）
 */
QString buttonSheet(const QColor &bg, const QColor &hover, const QColor &pressed,
                    const QColor &text, const QColor &border, bool withDisabled = false)
{
    QString s = QStringLiteral(
                    "QPushButton {"
                    "    background-color: %1;"
                    "    color: %2;"
                    "    border: 1px solid %3;"
                    "    border-radius: 6px;"
                    "    font-size: 13px;"
                    "    font-weight: 600;"
                    "}"
                    "QPushButton:hover { background-color: %4; }"
                    "QPushButton:pressed { background-color: %5; }")
            .arg(bg.name(), text.name(), border.name(), hover.name(), pressed.name());
    if (withDisabled) {
        s += QStringLiteral(
                 "QPushButton:disabled { background-color: %1; color: %2; border-color: %3; }")
             .arg(kPaper3.name(), kInk3.name(), kLine.name());
    }
    return s;
}

// Start：流动绿主按钮（墨色文字保证对比度，手册禁白小字于绿底）
const QString kStartSheet = buttonSheet(kGreen, kGreen600, kGreen700, kInkDark, kGreen, true);
// Stop：录制中 · 语义错误红
const QString kStopSheet  = buttonSheet(kError, QColor("#A93726"), QColor("#8F2E1F"), kCoverInk, kError);
// Waiting：等待非零信号 · 语义警告琥珀
const QString kWaitSheet  = buttonSheet(kWarning, QColor("#B37A19"), QColor("#9C6B15"), kInkDark, kWarning);
// New：纸白次级按钮
const QString kNewSheet   = buttonSheet(kPaper, kPaper3, kLine, kInk, kLineStrong, true);

// 时间码显示屏：墨绿黑底，待机亮绿 / 录制语义红
const QString kTimeIdleSheet =
    "QLabel {"
    "    background-color: #141A0D;"
    "    color: #8FCB3F;"
    "    border: 1px solid #2C3520;"
    "    border-radius: 6px;"
    "    padding: 8px 4px;"
    "}";
const QString kTimeRecSheet =
    "QLabel {"
    "    background-color: #141A0D;"
    "    color: #C4432E;"
    "    border: 1px solid #3A231C;"
    "    border-radius: 6px;"
    "    padding: 8px 4px;"
    "}";

/**
 * @brief 区块小标题（kicker）：绿色短划 + 墨灰文字，加宽字距
 */
QLabel *makeKicker(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(parent);
    QFont f = label->font();
    f.setPixelSize(11);
    f.setWeight(QFont::DemiBold);
    f.setLetterSpacing(QFont::AbsoluteSpacing, 1.2);
    label->setFont(f);
    label->setText(QStringLiteral("<span style=\"color:#79AE31;\">—</span>&nbsp;%1").arg(text));
    label->setStyleSheet(QStringLiteral("color:#5C6353;"));
    return label;
}

} // namespace

/**
 * @brief 构造函数
 * @param parent 父窗口指针
 */
ArtnetRecorder::ArtnetRecorder(QWidget *parent)
    : QWidget(parent)
    // , m_centralWidget(nullptr)
    , m_mainLayout(nullptr)
    // , m_controlLayout(nullptr)
    // , m_universeLayout(nullptr)
    , m_previewWidget(nullptr)
    , m_startUniverseLabel(nullptr)
    , m_endUniverseLabel(nullptr)
    , m_startUniverseBox(nullptr)
    , m_endUniverseBox(nullptr)
    , m_universeList(nullptr)
    , m_timeLabel(nullptr)
    , m_recordButton(nullptr)
    ,m_newButton(nullptr)
    , m_artnetReceiver(nullptr)
    , m_videoRecorder(nullptr)  // 初始化VideoRecorder
    , m_isRecording(false)      // 初始化录制状态
    , m_currentFileName("")            // 初始化文件名
    , m_hasValidFile(false)             // 初始化文件状态
    , m_waitTimer(nullptr)              // 初始化等待定时器
    , m_waitingForSignal(false)         // 初始化等待状态
    , m_statusLabel(nullptr)            // 信号状态标签
    , m_statsTimer(nullptr)             // 统计刷新定时器
    , m_blinkTimer(nullptr)             // 状态灯闪烁定时器
    , m_signalActive(false)             // 初始无信号
    , m_blinkPhase(true)                // 闪烁从亮点开始
    , m_blinkTick(0)                    // 闪烁节拍计数
    , m_lastPacketMs(-1)                // 尚未收到任何包
    , m_windowPackets(0)                // 包计数清零
    , m_droppedPackets(0)               // 丢包计数清零
{
    m_statsClock.start();   // 启动统计时钟基准
    setupUI();
    setupConnections();
    setupArtnetReceiver();
    setupVideoRecorder();  // 添加VideoRecorder初始化
    
    // 设置初始时间显示
    m_timeLabel->setText("00.00.00:00");
    
    this->setWindowTitle("Artnet Recorder");
    this->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);

    // 创建并配置等待定时器（非阻塞轮询）
    m_waitTimer = new QTimer(this);
    m_waitTimer->setInterval(50);
    m_waitTimer->setTimerType(Qt::PreciseTimer);
    connect(m_waitTimer, &QTimer::timeout, this, &ArtnetRecorder::handleWaitTimer);
}

/**
 * @brief 析构函数
 */
ArtnetRecorder::~ArtnetRecorder()
{
    if (m_isRecording) {
        m_videoRecorder->stopRecording();
        m_waitingForSignal = false;
        m_isRecording = false;      // 初始化录制状态
        m_previewWidget->clearData();
    }
}

/**
 * @brief 初始化界面UI
 */
void ArtnetRecorder::setupUI()
{
    // 设置窗口属性
    this->setGeometry(0, 0, 900, 540);
    this->setWindowTitle("Artnet Recorder");
    this->setWindowIcon(QIcon(":/ArtNet.png"));

    // 创建主布局：左侧功能/信号详情面板，右侧画面预览
    m_mainLayout = new QGridLayout(this);
    m_mainLayout->setContentsMargins(12, 12, 12, 12);   // 间距为 4px 的倍数
    m_mainLayout->setSpacing(12);
    m_mainLayout->setColumnStretch(0, 0);
    m_mainLayout->setColumnStretch(1, 1);

    // ---- 左侧面板：信号状态、信号详情、参数设置、控制按钮 ----
    QWidget *leftPanel = new QWidget(this);
    leftPanel->setFixedWidth(224);
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(8);

    // 等宽数字字体（品牌手册：时间码/地址/规格标注用 Mono）
    QFont monoFont;
    monoFont.setFamilies({QStringLiteral("Cascadia Code"),
                          QStringLiteral("Consolas"),
                          QStringLiteral("Courier New")});
    monoFont.setPixelSize(12);

    // 信号状态灯
    m_statusLabel = new QLabel(leftPanel);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    updateStatusLabel(false);   // 初始为无信号样式

    // 域列表（实际收到的 Universe，选中即预览）
    QLabel *listSection = makeKicker(QStringLiteral("域列表"), leftPanel);

    m_universeList = new QListWidget(leftPanel);
    m_universeList->setStyleSheet(
        "QListWidget {"
        "    background-color: #FAFAF6;"
        "    border: 1px solid #DFE2D3;"
        "    border-radius: 6px;"
        "    padding: 4px;"
        "    font-size: 12px;"
        "    font-family: \"Cascadia Code\",\"Consolas\",monospace;"
        "    color: #1B2113;"
        "}"
        "QListWidget::item { padding: 4px 8px; border-radius: 4px; }"
        "QListWidget::item:selected { background-color: #79AE31; color: #141A0D; font-weight: 600; }"
        "QListWidget::item:hover { background-color: #E8EADB; }");

    QLabel *paramSection = makeKicker(QStringLiteral("信号录制"), leftPanel);

    // Universe 设置（拖拽数值控件：横向拖动/滚轮调整，双击输入精确编辑）
    // 品牌纸白描边风格，悬停时流动绿描边；Universe 地址用等宽字体
    const QString dragBoxStyle =
        "IntDragValueWidget {"
        "    background-color: #FAFAF6;"
        "    border: 1px solid #C6CBBA;"
        "    border-radius: 6px;"
        "    color: #1B2113;"
        "}"
        "IntDragValueWidget:hover { border: 1px solid #79AE31; }";
    m_startUniverseLabel = new QLabel("Start", leftPanel);
    m_endUniverseLabel = new QLabel("End", leftPanel);
    m_startUniverseLabel->setStyleSheet("color:#5C6353; font-size:12px;");
    m_endUniverseLabel->setStyleSheet("color:#5C6353; font-size:12px;");

    m_startUniverseBox = new IntDragValueWidget(leftPanel);
    m_startUniverseBox->setStyleSheet(dragBoxStyle);
    m_startUniverseBox->setFont(monoFont);
    m_startUniverseBox->setFixedHeight(26);
    m_startUniverseBox->setRange(0, 65536);
    m_startUniverseBox->setValue(0);

    m_endUniverseBox = new IntDragValueWidget(leftPanel);
    m_endUniverseBox->setStyleSheet(dragBoxStyle);
    m_endUniverseBox->setFont(monoFont);
    m_endUniverseBox->setFixedHeight(26);
    m_endUniverseBox->setRange(0, 65536);
    m_endUniverseBox->setValue(64);

    QHBoxLayout *startRow = new QHBoxLayout();
    startRow->setSpacing(6);
    startRow->addWidget(m_startUniverseLabel);
    startRow->addWidget(m_startUniverseBox, 1);
    QHBoxLayout *endRow = new QHBoxLayout();
    endRow->setSpacing(6);
    endRow->addWidget(m_endUniverseLabel);
    endRow->addWidget(m_endUniverseBox, 1);

    // 时间码显示（墨绿黑屏面 + 等宽字体，录制时切换语义红）
    m_timeLabel = new QLabel("00:00:00:00", leftPanel);
    m_timeLabel->setStyleSheet(kTimeIdleSheet);
    QFont timeFont;
    timeFont.setFamilies({QStringLiteral("Cascadia Code"),
                          QStringLiteral("Consolas"),
                          QStringLiteral("Courier New")});
    timeFont.setPixelSize(24);
    timeFont.setWeight(QFont::DemiBold);
    timeFont.setLetterSpacing(QFont::AbsoluteSpacing, 1.5);
    m_timeLabel->setFont(timeFont);
    m_timeLabel->setFrameShape(QFrame::NoFrame);
    m_timeLabel->setAlignment(Qt::AlignCenter);

    // 控制按钮（Start 流动绿主按钮配墨色文字；New 纸白次级按钮）
    m_newButton = new QPushButton("New", leftPanel);
    m_newButton->setMinimumHeight(32);
    m_newButton->setStyleSheet(kNewSheet);
    m_recordButton = new QPushButton("Start", leftPanel);
    m_recordButton->setMinimumHeight(40);
    m_recordButton->setStyleSheet(kStartSheet);

    leftLayout->addWidget(m_statusLabel);
    leftLayout->addWidget(listSection);
    leftLayout->addWidget(m_universeList, 1);
    leftLayout->addWidget(paramSection);
    leftLayout->addLayout(startRow);
    leftLayout->addLayout(endRow);
    leftLayout->addWidget(m_timeLabel);
    leftLayout->addWidget(m_newButton);
    leftLayout->addWidget(m_recordButton);

    // ---- 右侧：DMX 通道网格预览 ----
    m_previewWidget = new DmxGridView(this);
    m_mainLayout->addWidget(leftPanel, 0, 0);
    m_mainLayout->addWidget(m_previewWidget, 0, 1);
    this->setLayout(m_mainLayout);

    // 初始域列表为空，收到数据后自动填充
    rebuildUniverseList();
}

/**
 * @brief 初始化界面连接
 */
void ArtnetRecorder::setupConnections()
{
    // 连接按钮信号
    connect(m_recordButton, &QPushButton::clicked, this, &ArtnetRecorder::onStartClicked);
    connect(m_newButton, &QPushButton::clicked, this, &ArtnetRecorder::onNewClicked);
    
    // 连接Universe设置控件（拖拽数值控件，拖动/滚轮/双击输入均触发 valueChanged）
    connect(m_startUniverseBox, &IntDragValueWidget::valueChanged,
            this, &ArtnetRecorder::onUniverseSettingsChanged);
    connect(m_endUniverseBox, &IntDragValueWidget::valueChanged,
            this, &ArtnetRecorder::onUniverseSettingsChanged);

    // 域列表选中 → 切换预览显示的 Universe
    connect(m_universeList, &QListWidget::currentRowChanged, this, [this](int row) {
        if (row < 0 || !m_previewWidget) {
            return;
        }
        const QList<int> active = m_previewWidget->activeUniverses();
        if (row < active.size()) {
            m_previewWidget->setCurrentUniverse(active.at(row));
        }
    });

    // 发现新 Universe（或清空数据）时同步刷新域列表
    connect(m_previewWidget, &DmxGridView::activeUniversesChanged,
            this, &ArtnetRecorder::rebuildUniverseList);

    // 信号统计刷新（500ms 批量刷新，避免每个 Art-Net 包都刷 UI）
    m_statsTimer = new QTimer(this);
    m_statsTimer->setInterval(500);
    connect(m_statsTimer, &QTimer::timeout, this, &ArtnetRecorder::refreshStats);
    m_statsTimer->start();

    // 状态灯闪烁（500ms 节拍）：有信号 1Hz 心跳，无信号 0.5Hz 缓闪
    m_blinkTimer = new QTimer(this);
    m_blinkTimer->setInterval(500);
    connect(m_blinkTimer, &QTimer::timeout, this, &ArtnetRecorder::onBlinkTick);
    m_blinkTimer->start();
}

/**
 * @brief 初始化Artnet接收器
 */
void ArtnetRecorder::setupArtnetReceiver()
{
    // 不设置父对象，让ArtnetReceiver管理自己的生命周期
    m_artnetReceiver = new ArtnetReceiver(nullptr);
    connect(m_artnetReceiver, &ArtnetReceiver::receiveArtnet,
            this, &ArtnetRecorder::onArtnetDataReceived);
    connect(this, &ArtnetRecorder::cleanUp, m_artnetReceiver, &ArtnetReceiver::cleanup);
}

/**
 * @brief 初始化视频录制器
 */
void ArtnetRecorder::setupVideoRecorder()
{
    // 创建VideoRecorder实例
    m_videoRecorder = new VideoRecorder(this);
    
    // 创建简单的帧捕获定时器
    m_captureTimer = new QTimer(this);
    int interval = static_cast<int>(1000.0f / FPS);  // 计算帧间隔
    m_captureTimer->setInterval(interval);
    m_captureTimer->setTimerType(Qt::PreciseTimer);  // 使用精确定时器模式
    
    // 连接定时器到捕获函数
    connect(m_captureTimer, &QTimer::timeout, this, &ArtnetRecorder::captureFrame);
    
    // 连接VideoRecorder的信号
    connect(m_videoRecorder, &VideoRecorder::recordingStarted, this, [this]() {
        m_recordButton->setText("Stop");
        m_recordButton->setStyleSheet(kStopSheet);
        m_timeLabel->setStyleSheet(kTimeRecSheet);       // 时间码切换录制红
        if (m_previewWidget) {
            m_previewWidget->setRecordingActive(true);   // 标题条显示 REC
        }

        // 禁用universe设置和New按钮
        m_startUniverseBox->setEnabled(false);
        m_endUniverseBox->setEnabled(false);
        m_newButton->setEnabled(false);

        // 启动帧捕获定时器
        m_captureTimer->start();
    });

    connect(m_videoRecorder, &VideoRecorder::recordingStopped, this, [this]() {
        m_recordButton->setText("Start");
        m_recordButton->setStyleSheet(kStartSheet);
        m_timeLabel->setStyleSheet(kTimeIdleSheet);
        if (m_previewWidget) {
            m_previewWidget->setRecordingActive(false);
        }
        m_captureTimer->stop();  // 停止帧捕获定时器
        m_isRecording = false;

        // 重新启用universe设置和New按钮
        m_startUniverseBox->setEnabled(true);
        m_endUniverseBox->setEnabled(true);
        m_newButton->setEnabled(true);

        // 更新按钮状态
        updateButtonStates();
    });

    connect(m_videoRecorder, &VideoRecorder::recordingError, this, [this](const QString &error) {
        QMessageBox::warning(this, "Recording Error", error);
        m_captureTimer->stop();  // 停止帧捕获定时器
        m_isRecording = false;
        m_recordButton->setText("Start");
        m_recordButton->setStyleSheet(kStartSheet);
        m_timeLabel->setStyleSheet(kTimeIdleSheet);
        if (m_previewWidget) {
            m_previewWidget->setRecordingActive(false);
        }

        // 重新启用universe设置和New按钮
        m_startUniverseBox->setEnabled(true);
        m_endUniverseBox->setEnabled(true);
        m_newButton->setEnabled(true);

        // 更新按钮状态
        updateButtonStates();
    });
    
    // 初始化按钮状态
    updateButtonStates();
}

// 移除复杂的checkFrameTime方法，简化captureFrame方法
/**
 * @brief 定时捕获帧图像
 */
void ArtnetRecorder::captureFrame()
{
    if (!m_isRecording || !m_previewWidget || !m_videoRecorder) {
        return;
    }
    
    // 更新时间显示（使用VideoRecorder的录制帧数）
    qint64 recordingFrames = m_videoRecorder->getRecordingFrames();
    m_timeLabel->setText(formatTime(recordingFrames));
    
    // 从OpenGL控件获取当前渲染的图像
    QImage frameImage = m_previewWidget->getTextureImage();
    
    if (!frameImage.isNull()) {
        // 将图像添加到视频录制器
        m_videoRecorder->addFrame(frameImage);
    }
}

/**
 * @brief 开始/停止按钮点击事件（支持录制前的非阻塞等待逻辑）
 * - 如果当前无非零DMX数据：进入等待模式（50ms轮询），UI不冻结
 * - 检测到非零数据：自动开始录制
 * - 等待期间再次点击按钮：取消等待
 */
void ArtnetRecorder::onStartClicked()
{
    // 如果正在等待非零信号，则此点击视为取消等待
    if (m_waitingForSignal) {
        m_waitTimer->stop();
        m_waitingForSignal = false;
        m_startUniverseBox->setEnabled(true);
        m_endUniverseBox->setEnabled(true);
        m_newButton->setEnabled(true);
        // 恢复按钮文本与品牌样式（非录制状态）
        updateButtonStates();
        return;
    }

    if (!m_isRecording) {
        // 开始录制
        if (!m_hasValidFile) {
            // 如果没有有效文件，提示用户先点击New按钮
            QMessageBox::information(this, "提示", "请先点击 'New' 按钮创建录制文件！");
            return;
        }

        if (m_previewWidget) {
            // 使用预览控件的录制图像尺寸（512 × Universe范围）
            int width = m_previewWidget->getTextureWidth();
            int height = m_previewWidget->getTextureHeight();

            // 开始前仅检查一次：如数据全零则进入等待模式，不阻塞UI
            if (!m_previewWidget->hasNonZeroDMX()) {
                m_waitingForSignal = true;
                m_recordButton->setText("Waiting...");
                m_recordButton->setStyleSheet(kWaitSheet);   // 等待态 · 语义警告琥珀
                // 等待期间禁用Universe设置与New按钮，避免设置变化导致高度变化
                m_startUniverseBox->setEnabled(false);
                m_endUniverseBox->setEnabled(false);
                m_newButton->setEnabled(false);
                m_waitTimer->start();
                return;
            }

            // 立刻开始录制（已有非零数据）
            if (m_videoRecorder->startRecording(m_currentFileName, width, height)) {
                m_isRecording = true;
            } else {
                QMessageBox::warning(this, "错误", "开始录制失败！");
            }
        }
    } else {
        // 停止录制
        m_videoRecorder->stopRecording();
        m_waitingForSignal = false;
        m_isRecording = false;      // 初始化录制状态
        m_previewWidget->clearData();
    }
}

/**
 * @brief Universe设置改变
 */
void ArtnetRecorder::onUniverseSettingsChanged()
{
    if (m_previewWidget) {
        m_previewWidget->setFirstUniverse(m_startUniverseBox->value());
        m_previewWidget->setEndUniverse(m_endUniverseBox->value());
    }
    rebuildUniverseList();
}

/**
 * @brief 重建左侧域列表：显示实际收到过数据的 Universe（升序），
 *        选中项即当前预览的域；当前域无数据时自动跳到第一个有数据的域
 */
void ArtnetRecorder::rebuildUniverseList()
{
    if (!m_universeList || !m_previewWidget) {
        return;
    }

    const QList<int> active = m_previewWidget->activeUniverses();
    const int currentUniverse = m_previewWidget->currentUniverse();

    {
        const QSignalBlocker blocker(m_universeList);
        m_universeList->clear();
        for (int u : active) {
            m_universeList->addItem(QStringLiteral("Universe %1").arg(u));
        }
        if (!active.isEmpty()) {
            const int row = active.contains(currentUniverse)
                                ? active.indexOf(currentUniverse)
                                : 0;
            m_universeList->setCurrentRow(row);
            m_previewWidget->setCurrentUniverse(active.at(row));
        }
    }
}

/**
 * @brief 处理接收到的Artnet数据
 * @param data Artnet数据
 */
void ArtnetRecorder::onArtnetDataReceived(QVariantMap &data)
{
    if (m_previewWidget) {
        m_previewWidget->updateArtnetData(data);
    }

    // ---- 信号源统计（此槽经队列连接运行于主线程；UI 由 500ms 定时器批量刷新）----
    const qint64 now = m_statsClock.elapsed();
    m_lastPacketMs = now;
    ++m_windowPackets;

    if (data.contains("host")) {
        m_hostLastSeenMs[data["host"].toString()] = now;
    }

    if (!data.contains("universe")) {
        return;
    }
    const int universe = data["universe"].toInt();
    m_universeLastSeenMs[universe] = now;

    // 丢包检测：Art-Net sequence 每包 +1（按 源主机+Universe 独立计数），向前跳变即视为丢包
    if (data.contains("host") && data.contains("sequence")) {
        const QString seqKey = data["host"].toString() + ":" + QString::number(universe);
        const quint8 seq = static_cast<quint8>(data["sequence"].toUInt());
        auto seqIt = m_lastSequence.find(seqKey);
        if (seqIt != m_lastSequence.end()) {
            const int delta = (seq - *seqIt + 256) % 256;
            if (delta > 0 && delta < 128) {
                m_droppedPackets += delta;
            }
            // delta == 0 视为重复包；delta >= 128 视为发送端重置/乱序，不累计
        }
        m_lastSequence.insert(seqKey, seq);
    }
}

/**
 * @brief 格式化时间显示
 * @param frameCounts 毫秒数
 * @return 格式化的时间字符串（格式：hh.mm.ss:frame）
 */
QString ArtnetRecorder::formatTime(qint64 frameCounts)
{

    int totalSeconds = frameCounts / FPS;
    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int seconds = totalSeconds % 60;
    int frames = frameCounts % int(FPS); // 假设25fps
    
    return QString("%1.%2.%3:%4")
           .arg(hours, 2, 10, QChar('0'))
           .arg(minutes, 2, 10, QChar('0'))
           .arg(seconds, 2, 10, QChar('0'))
           .arg(frames, 2, 10, QChar('0'));
}

/**
 * @brief New按钮点击事件 - 创建新的录制文件
 * 默认使用.dmx后缀，并且在用户选择后统一修正为.dmx
 */
void ArtnetRecorder::onNewClicked()
{
    if (m_isRecording) {
        QMessageBox::warning(this, "警告", "正在录制中，无法创建新文件！");
        return;
    }
    
    /**
     * @brief New按钮点击事件 - 创建新的录制文件
     * 默认使用.dmx后缀，并且在用户选择后统一修正为.dmx
     */
    QString fileName = QFileDialog::getSaveFileName(
        this,
        "创建新的录制文件",
        QDir::homePath() + "/" + DefaultFilenameP,
        "DMX 文件 (*.dmx)"
    );
    
    if (!fileName.isEmpty()) {
        // 统一修正为 .dmx 扩展名（无扩展或其他扩展都会被替换）
        QFileInfo fi(fileName);
        QString corrected = fi.path() + "/" + fi.completeBaseName() + ".dmx";
        m_currentFileName = corrected;
        m_hasValidFile = true;
        
        // 更新按钮状态
        updateButtonStates();
        
        // 显示当前文件信息（使用修正后的文件名）
        QFileInfo fileInfo(corrected);
        this->setWindowTitle(QString("Artnet Recorder - %1").arg(fileInfo.fileName()));
    }
}

/**
 * @brief 更新按钮状态
 */
void ArtnetRecorder::updateButtonStates()
{
    // 按钮样式：录制中 Stop 用语义错误红，待机 Start 用流动绿主按钮
    if (m_isRecording) {
        // 录制中：Start按钮显示Stop，New按钮禁用
        m_recordButton->setText("Stop");
        m_recordButton->setStyleSheet(kStopSheet);
        m_recordButton->setEnabled(true);
        m_newButton->setEnabled(false);
    } else if (m_hasValidFile) {
        // 有有效文件但未录制：Start按钮可用，New按钮可用
        m_recordButton->setText("Start");
        m_recordButton->setStyleSheet(kStartSheet);
        m_recordButton->setEnabled(true);
        m_newButton->setEnabled(true);
    } else {
        // 没有有效文件：Start按钮禁用，New按钮可用
        m_recordButton->setText("Start");
        m_recordButton->setStyleSheet(kStartSheet);
        m_recordButton->setEnabled(false);
        m_newButton->setEnabled(true);
    }
}

/**
 * @brief 等待非零DMX信号的定时器槽
 * 每次触发检查一次数据是否非零；一旦有非零数据则自动开始录制。
 * 仅在开始录制前轮询检查；录制开始后不再检查。
 */
void ArtnetRecorder::handleWaitTimer()
{
    if (!m_waitingForSignal || !m_previewWidget || !m_hasValidFile) {
        return;
    }

    // 检查是否出现非零DMX数据
    if (!m_previewWidget->hasNonZeroDMX()) {
        return; // 继续等待
    }

    // 检测到非零数据：停止等待并开始录制
    m_waitTimer->stop();
    m_waitingForSignal = false;

    int width = m_previewWidget->getTextureWidth();
    int height = m_previewWidget->getTextureHeight();

    if (m_videoRecorder->startRecording(m_currentFileName, width, height)) {
        m_isRecording = true;
        // recordingStarted 信号会进一步更新按钮文本/样式、时间码与标题条 REC 指示
    } else {
        // 启动失败：恢复UI
        QMessageBox::warning(this, "错误", "开始录制失败！");
        m_recordButton->setText("Start");
        m_recordButton->setStyleSheet(kStartSheet);
        m_startUniverseBox->setEnabled(true);
        m_endUniverseBox->setEnabled(true);
        m_newButton->setEnabled(true);
        updateButtonStates();
    }
}

/**
 * @brief 定时刷新（500ms）：更新信号状态灯 + 清理过期统计记录
 * 高频计数在 onArtnetDataReceived 中累加
 */
void ArtnetRecorder::refreshStats()
{
    const qint64 now = m_statsClock.elapsed();

    // 信号存活判定：许多控制台在 DMX 数据静止时会降低 Art-Net 重发频率，
    // 包间隔可能远大于运动画面的刷新率；窗口取 5s，防止静止画面在
    // "信号正常 / 无信号" 之间来回跳（真断线 5s 后才判无信号，可接受）。
    constexpr qint64 kSignalTimeoutMs = 5000;
    const bool hasSignal = (m_lastPacketMs >= 0) && (now - m_lastPacketMs < kSignalTimeoutMs);
    updateStatusLabel(hasSignal);

    // 清理 2 秒不活跃的源主机 / Universe 记录，防止 QHash 无限增长
    for (auto it = m_hostLastSeenMs.begin(); it != m_hostLastSeenMs.end();) {
        if (now - it.value() > 2000) {
            it = m_hostLastSeenMs.erase(it);
        } else {
            ++it;
        }
    }
    for (auto it = m_universeLastSeenMs.begin(); it != m_universeLastSeenMs.end();) {
        if (now - it.value() > 2000) {
            it = m_universeLastSeenMs.erase(it);
        } else {
            ++it;
        }
    }
}

/**
 * @brief 更新信号状态标签（正常/无信号）的文本与样式
 * @param hasSignal 最近5秒内是否收到过Art-Net包
 */
void ArtnetRecorder::updateStatusLabel(bool hasSignal)
{
    m_signalActive = hasSignal;
    if (hasSignal) {
        m_statusLabel->setStyleSheet(
            "QLabel {"
            "    background-color: #E8F1D2;"   // 绿 100
            "    border: 1px solid #B6D47B;"   // 绿 300
            "    border-radius: 6px;"
            "    padding: 8px 10px;"
            "    font-size: 13px;"
            "    font-weight: 600;"
            "    color: #4C8C27;"              // 语义·成功
            "}");
    } else {
        m_statusLabel->setStyleSheet(
            "QLabel {"
            "    background-color: #F1F2E9;"   // 纸白 2
            "    border: 1px solid #DFE2D3;"   // 细线
            "    border-radius: 6px;"
            "    padding: 8px 10px;"
            "    font-size: 13px;"
            "    color: #9BA294;"              // 弱文字
            "}");
    }
    updateStatusText();
}

/**
 * @brief 按闪烁相位刷新状态文本：圆点在亮/暗两色间切换，文字底色不变
 * 信号正常=成功绿 ↔ 绿 300 心跳；无信号=弱灰 ↔ 线色 缓慢呼吸
 */
void ArtnetRecorder::updateStatusText()
{
    const QString dotColor = m_signalActive
        ? (m_blinkPhase ? QStringLiteral("#4C8C27") : QStringLiteral("#B6D47B"))
        : (m_blinkPhase ? QStringLiteral("#9BA294") : QStringLiteral("#C6CBBA"));
    const QString text = m_signalActive ? QStringLiteral("信号正常") : QStringLiteral("无信号");
    m_statusLabel->setText(QStringLiteral(
        "<span style=\"color:%1;\">●</span>&nbsp;&nbsp;%2").arg(dotColor, text));
}

/**
 * @brief 状态灯闪烁节拍（500ms）
 * 有信号：每拍翻转，1Hz 心跳；无信号：每两拍翻转，0.5Hz 缓慢呼吸
 */
void ArtnetRecorder::onBlinkTick()
{
    ++m_blinkTick;
    const int period = m_signalActive ? 1 : 2;
    if (m_blinkTick % period == 0) {
        m_blinkPhase = !m_blinkPhase;
        updateStatusText();
    }
}
