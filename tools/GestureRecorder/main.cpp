/**
 * @file main.cpp
 * @brief Flow Gesture Recorder — 手势动作库录制工具（独立 exe）
 *
 * 工作流：
 *   1. 打开摄像头（或视频文件），画面内检测 MediaPipe 手部 21 关键点
 *   2. 添加标签并选中，对着镜头比划，按住「按住采样」以 ~5 帧/秒采集特征
 *   3. 保存为 .gesture 动作库文件（JSON），供 Flow 的 Gesture 节点加载匹配
 *
 * 特征：worldLandmarks 21 点米制坐标展平 (63) + handedness (1)，见 GestureFeature.hpp
 * 模型：运行目录下 plugins/Models/palm_detection_mediapipe_2023feb.onnx
 *       与 handpose_estimation_mediapipe_2023feb.onnx（与 Flow 共用，无需额外部署）
 */

#include <QApplication>
#include <QComboBox>
#include <QElapsedTimer>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QHBoxLayout>
#include <QIcon>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include "Elements/FloatDragValueWidget/FloatDragValueWidget.hpp"

#include <memory>
#include <optional>

#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

#include "GestureFeature.hpp"
#include "GestureLibrary.hpp"
#include "MediaPipeHandEngine.hpp"

namespace {

constexpr int kPreviewMaxWidth = 960;

// ---- Flow 品牌色（docs/brand/Flow-Brand-Visual-Identity-Manual.html）----
// Primary 流动绿 #79AE31（500）；墨绿黑 #1B2113 正文；纸白 #FAFAF6 背景（避免纯白）；
// 绿阶 200 #D2E4A9 / 300 #B6D47B / 600 #629327 / 700 #4A7021 / 950 #18270E
// 状态：成功 #4C8C27 / 警告 #C98A1F / 错误 #C4432E / 信息 #3F6E8E
// 原则：4px 间距倍数、圆头圆角、流动绿只做强调、避免绿底白小字
const QLatin1String kStyleSheet(R"(
RecorderWindow {
    background-color: #FAFAF6;
}
QLabel {
    color: #1B2113;
    font-size: 13px;
}
QPushButton {
    background-color: #FAFAF6;
    color: #4A7021;
    border: 1px solid #B6D47B;
    border-radius: 6px;
    padding: 6px 16px;
    font-weight: 600;
}
QPushButton:hover {
    background-color: #F5F9EB;
    border-color: #79AE31;
}
QPushButton:pressed {
    background-color: #E8F1D2;
}
QPushButton:disabled {
    color: #B6D47B;
    border-color: #D2E4A9;
}
QPushButton#primaryBtn, QPushButton#sampleBtn {
    background-color: #79AE31;
    color: #1B2113;
    border: none;
    font-weight: bold;
}
QPushButton#primaryBtn:hover {
    background-color: #629327;
}
QPushButton#primaryBtn:pressed {
    background-color: #4A7021;
}
QPushButton#sampleBtn:hover {
    background-color: #629327;
}
QPushButton#sampleBtn:pressed {
    background-color: #4A7021;
}
QPushButton#sampleBtn:checked {
    background-color: #4C8C27;
    color: #FFFFFF;
}
QLabel#secondaryLabel {
    color: #333333;
    font-size: 12px;
}
QLabel#statusLabel {
    color: #333333;
    font-size: 12px;
}
QLabel#hintLabel {
    color: #4A7021;
    font-size: 12px;
}
QComboBox, QLineEdit {
    background-color: #FFFFFF;
    color: #1B2113;
    border: 1px solid #D2E4A9;
    border-radius: 6px;
    padding: 4px 8px;
    selection-background-color: #D2E4A9;
    selection-color: #1B2113;
}
QComboBox:hover, QLineEdit:hover {
    border-color: #B6D47B;
}
QComboBox:focus, QLineEdit:focus {
    border: 1px solid #79AE31;
}
QComboBox QAbstractItemView {
    background-color: #FFFFFF;
    color: #1B2113;
    selection-background-color: #E8F1D2;
    selection-color: #1B2113;
    border: 1px solid #D2E4A9;
}
QListWidget {
    background-color: #FFFFFF;
    color: #1B2113;
    border: 1px solid #D2E4A9;
    border-radius: 6px;
    padding: 4px;
}
QListWidget::item {
    border-radius: 4px;
    padding: 4px 8px;
}
QListWidget::item:selected {
    background-color: #E8F1D2;
    color: #1B2113;
}
QListWidget::item:hover:!selected {
    background-color: #F5F9EB;
}
QToolTip {
    background-color: #1B2113;
    color: #F5F9EB;
    border: none;
    padding: 4px 8px;
}
)");

QImage matToQImage(const cv::Mat& bgr)
{
    if (bgr.empty()) {
        return {};
    }
    cv::Mat rgb;
    cv::cvtColor(bgr, rgb, cv::COLOR_BGR2RGB);
    return QImage(rgb.data, rgb.cols, rgb.rows, static_cast<qsizetype>(rgb.step),
                  QImage::Format_RGB888)
        .copy();
}

} // namespace

class RecorderWindow final : public QWidget
{
    Q_OBJECT
public:
    explicit RecorderWindow(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setWindowTitle(QStringLiteral("Flow Gesture Recorder — 手势动作库录制"));
        setWindowIcon(QIcon(QStringLiteral(":/recorder/finger.png")));
        resize(1180, 760);
        setStyleSheet(kStyleSheet);

        auto* rootLayout = new QHBoxLayout(this);
        rootLayout->setContentsMargins(16, 16, 16, 16);
        rootLayout->setSpacing(16);

        // 左侧：预览
        auto* leftLayout = new QVBoxLayout;
        leftLayout->setSpacing(12);
        auto* sourceRow = new QHBoxLayout;
        sourceRow->setSpacing(8);
        cameraCombo = new QComboBox(this);
        cameraCombo->setMinimumWidth(110);
        for (int i = 0; i < 6; ++i) {
            cameraCombo->addItem(QStringLiteral("摄像头 %1").arg(i));
        }
        openCameraBtn = new QPushButton(QStringLiteral("打开摄像头"), this);
        openCameraBtn->setObjectName("primaryBtn");
        openVideoBtn = new QPushButton(QStringLiteral("打开视频文件"), this);
        stopBtn = new QPushButton(QStringLiteral("停止"), this);
        sourceRow->addWidget(cameraCombo);
        sourceRow->addWidget(openCameraBtn);
        sourceRow->addWidget(openVideoBtn);
        sourceRow->addWidget(stopBtn);
        leftLayout->addLayout(sourceRow);

        preview = new QLabel(this);
        preview->setMinimumSize(760, 480);
        preview->setAlignment(Qt::AlignCenter);
        preview->setStyleSheet(
            "background:#141A0D; color:#B6D47B; border-radius:8px; font-size:14px;");
        preview->setText(QStringLiteral("先打开摄像头或视频文件"));
        leftLayout->addWidget(preview, 1);

        hintLabel = new QLabel(this);
        hintLabel->setObjectName("hintLabel");
        hintLabel->setWordWrap(true);
        leftLayout->addWidget(hintLabel);

        rootLayout->addLayout(leftLayout, 3);

        // 右侧：库管理
        auto* rightLayout = new QVBoxLayout;
        rightLayout->setSpacing(8);
        fileLabel = new QLabel(QStringLiteral("未选择库文件"), this);
        fileLabel->setObjectName("secondaryLabel");
        fileLabel->setWordWrap(true);
        rightLayout->addWidget(fileLabel);

        auto* fileRow1 = new QHBoxLayout;
        openLibBtn = new QPushButton(QStringLiteral("打开库"), this);
        saveBtn = new QPushButton(QStringLiteral("保存"), this);
        fileRow1->addWidget(openLibBtn);
        fileRow1->addWidget(saveBtn);
        rightLayout->addLayout(fileRow1);

        rightLayout->addWidget(new QLabel(QStringLiteral("标签列表（选中后采样）："), this));
        gestureList = new QListWidget(this);
        rightLayout->addWidget(gestureList, 1);

        auto* addRow = new QHBoxLayout;
        labelEdit = new QLineEdit(this);
        labelEdit->setPlaceholderText(QStringLiteral("新手势标签，如 fist"));
        addLabelBtn = new QPushButton(QStringLiteral("添加标签"), this);
        addRow->addWidget(labelEdit, 1);
        addRow->addWidget(addLabelBtn);
        rightLayout->addLayout(addRow);

        auto* editRow = new QHBoxLayout;
        removeLabelBtn = new QPushButton(QStringLiteral("删除标签"), this);
        undoSampleBtn = new QPushButton(QStringLiteral("撤销上一帧"), this);
        editRow->addWidget(removeLabelBtn);
        editRow->addWidget(undoSampleBtn);
        rightLayout->addLayout(editRow);

        auto* thresholdRow = new QHBoxLayout;
        thresholdRow->addWidget(new QLabel(QStringLiteral("选中标签阈值:"), this));
        thresholdSpin = new FloatDragValueWidget(this);
        thresholdSpin->setRange(0.0, 1.0);
        thresholdSpin->setSingleStep(0.01);
        thresholdSpin->setDecimals(2);
        thresholdSpin->setValue(GestureFeature::kDefaultThreshold);
        thresholdRow->addWidget(thresholdSpin);
        rightLayout->addLayout(thresholdRow);

        sampleBtn = new QPushButton(QStringLiteral("按住采样"), this);
        sampleBtn->setObjectName("sampleBtn");
        sampleBtn->setMinimumHeight(48);
        sampleBtn->setToolTip(QStringLiteral("按住期间以 ~5 帧/秒采集当前画面的手部特征"));
        rightLayout->addWidget(sampleBtn);

        statusLabel = new QLabel(QStringLiteral("模型未加载"), this);
        statusLabel->setObjectName("statusLabel");
        statusLabel->setWordWrap(true);
        rightLayout->addWidget(statusLabel);

        rootLayout->addLayout(rightLayout, 1);

        frameTimer = new QTimer(this);
        frameTimer->setInterval(33);
        connect(frameTimer, &QTimer::timeout, this, &RecorderWindow::onFrame);

        connect(openCameraBtn, &QPushButton::clicked, this, &RecorderWindow::openCamera);
        connect(openVideoBtn, &QPushButton::clicked, this, &RecorderWindow::openVideo);
        connect(stopBtn, &QPushButton::clicked, this, &RecorderWindow::stopSource);
        connect(openLibBtn, &QPushButton::clicked, this, &RecorderWindow::openLibrary);
        connect(saveBtn, &QPushButton::clicked, this, [this] { saveLibrary(m_libraryPath); });
        connect(addLabelBtn, &QPushButton::clicked, this, &RecorderWindow::addLabel);
        connect(labelEdit, &QLineEdit::returnPressed, this, &RecorderWindow::addLabel);
        connect(removeLabelBtn, &QPushButton::clicked, this, &RecorderWindow::removeLabel);
        connect(undoSampleBtn, &QPushButton::clicked, this, &RecorderWindow::undoSample);
        connect(sampleBtn, &QPushButton::pressed, this, &RecorderWindow::samplingPressed);
        connect(sampleBtn, &QPushButton::released, this, &RecorderWindow::samplingReleased);
        connect(gestureList, &QListWidget::itemSelectionChanged, this,
                &RecorderWindow::onSelectionChanged);
        connect(thresholdSpin, &FloatDragValueWidget::valueChanged,
                this, &RecorderWindow::onThresholdChanged);

        loadModels();
    }

    ~RecorderWindow() override
    {
        frameTimer->stop();
        if (capture) {
            capture->release();
        }
    }

private Q_SLOTS:
    void openCamera()
    {
        stopSource();
        capture = std::make_unique<cv::VideoCapture>(cameraCombo->currentIndex());
        capture->set(cv::CAP_PROP_FRAME_WIDTH, 1280);
        capture->set(cv::CAP_PROP_FRAME_HEIGHT, 720);
        if (!capture->isOpened()) {
            capture.reset();
            QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("无法打开摄像头"));
            return;
        }
        frameTimer->start();
    }

    void openVideo()
    {
        const QString path = QFileDialog::getOpenFileName(
            this, QStringLiteral("打开视频文件"), QString(),
            QStringLiteral("视频 (*.mp4 *.avi *.mov *.mkv);;所有文件 (*)"));
        if (path.isEmpty()) {
            return;
        }
        stopSource();
        capture = std::make_unique<cv::VideoCapture>(path.toLocal8Bit().toStdString());
        if (!capture->isOpened()) {
            capture.reset();
            QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("无法打开视频文件"));
            return;
        }
        frameTimer->start();
    }

    void stopSource()
    {
        frameTimer->stop();
        samplingReleased();
        if (capture) {
            capture->release();
            capture.reset();
        }
        preview->setText(QStringLiteral("已停止"));
    }

    void openLibrary()
    {
        if (!confirmDiscardIfDirty()) {
            return;
        }
        const QString path = QFileDialog::getOpenFileName(
            this, QStringLiteral("打开动作库"), QString(),
            QStringLiteral("手势动作库 (*.gesture);;所有文件 (*)"));
        if (path.isEmpty()) {
            return;
        }
        GestureLibrary::Library lib;
        QString err;
        if (!GestureLibrary::load(path, lib, &err)) {
            QMessageBox::warning(this, QStringLiteral("错误"), err);
            return;
        }
        m_library = std::move(lib);
        m_libraryPath = path;
        m_dirty = false;
        refreshLibraryUi();
    }

    void saveLibrary(QString path)
    {
        if (m_library.gestures.isEmpty()) {
            QMessageBox::information(this, QStringLiteral("提示"),
                                     QStringLiteral("动作库为空，先添加标签并采样"));
            return;
        }
        if (path.isEmpty()) {
            path = QFileDialog::getSaveFileName(
                this, QStringLiteral("保存动作库"),
                QStringLiteral("gestures.gesture"),
                QStringLiteral("手势动作库 (*.gesture)"));
            if (path.isEmpty()) {
                return;
            }
            if (!path.endsWith(QStringLiteral(".gesture"), Qt::CaseInsensitive)) {
                path += QStringLiteral(".gesture");
            }
        }
        QString err;
        if (!GestureLibrary::save(path, m_library, &err)) {
            QMessageBox::warning(this, QStringLiteral("错误"), err);
            return;
        }
        m_libraryPath = path;
        m_dirty = false;
        refreshLibraryUi();
    }

    void addLabel()
    {
        const QString label = labelEdit->text().trimmed();
        if (label.isEmpty()) {
            return;
        }
        if (m_library.indexOf(label) >= 0) {
            gestureList->setCurrentRow(m_library.indexOf(label));
            labelEdit->clear();
            return;
        }
        GestureLibrary::Gesture g;
        g.label = label;
        g.threshold = thresholdSpin->value();
        m_library.gestures.push_back(g);
        m_dirty = true;
        labelEdit->clear();
        refreshLibraryUi();
        gestureList->setCurrentRow(m_library.gestures.size() - 1);
    }

    void removeLabel()
    {
        const int row = gestureList->currentRow();
        if (row < 0 || row >= m_library.gestures.size()) {
            return;
        }
        m_library.gestures.removeAt(row);
        m_dirty = true;
        refreshLibraryUi();
    }

    void undoSample()
    {
        const int row = gestureList->currentRow();
        if (row < 0 || row >= m_library.gestures.size()) {
            return;
        }
        auto& samples = m_library.gestures[row].samples;
        if (samples.isEmpty()) {
            return;
        }
        // 撤销一个完整采集批次（一次按帧入库 5 个增广模板）
        const qsizetype removeCount = std::min<qsizetype>(samples.size(), 5);
        samples.resize(samples.size() - removeCount);
        m_dirty = true;
        refreshLibraryUi();
    }

    void samplingPressed()
    {
        if (m_library.gestures.isEmpty()) {
            statusLabel->setText(QStringLiteral("请先添加标签再采样"));
            return;
        }
        m_sampling = true;
        m_sampleTimer.invalidate();
    }

    void samplingReleased()
    {
        m_sampling = false;
        if (m_dirty && !m_libraryPath.isEmpty()) {
            saveLibrary(m_libraryPath);
        }
    }

    void onThresholdChanged(double value)
    {
        const int row = gestureList->currentRow();
        if (row < 0 || row >= m_library.gestures.size()) {
            return;
        }
        m_library.gestures[row].threshold = thresholdSpin->value();
        m_dirty = true;
    }

    void onSelectionChanged()
    {
        const int row = gestureList->currentRow();
        if (row < 0 || row >= m_library.gestures.size()) {
            return;
        }
        QSignalBlocker blocker(thresholdSpin);
        thresholdSpin->setValue(m_library.gestures[row].threshold);
    }

    void onFrame()
    {
        if (!capture || !capture->isOpened()) {
            return;
        }
        cv::Mat frame;
        if (!capture->read(frame) || frame.empty()) {
            return;
        }

        // 长边 1280 上限（与 HandPose 节点一致，保证特征坐标系同源）
        if (std::max(frame.cols, frame.rows) > 1280) {
            const double scale = 1280.0 / static_cast<double>(std::max(frame.cols, frame.rows));
            cv::resize(frame, frame, cv::Size(), scale, scale, cv::INTER_AREA);
        }

        m_lastFeature.reset();
        if (modelsLoaded) {
            try {
                const auto hands = MediaPipeHand::detectHands(*palmDetector, *handEstimator, frame);
                const MediaPipeHand::HandResult* best = nullptr;
                for (const auto& hand : hands) {
                    if (!hand.valid) {
                        continue;
                    }
                    if (!best || hand.confidence > best->confidence) {
                        best = &hand;
                    }
                }
                if (best && !best->worldLandmarks.empty()) {
                    cv::Mat overlay = frame.clone();
                    MediaPipeHand::drawHandOverlay(overlay, *best);
                    std::array<float, GestureFeature::kDim> feature;
                    if (GestureFeature::extract(best->worldLandmarks, best->handedness, feature)) {
                        m_lastFeature = feature;
                    }
                    if (m_sampling && m_lastFeature.has_value()) {
                        sampleCurrent(m_lastFeature.value());
                    }
                    showPreview(overlay, liveNearest(m_lastFeature));
                    return;
                }
            } catch (const cv::Exception& e) {
                statusLabel->setText(QStringLiteral("推理异常: %1").arg(e.what()));
            }
        }
        if (m_sampling && !m_lastFeature.has_value()) {
            statusLabel->setText(QStringLiteral("采样中但未检测到手，保持手在画面内"));
        }
        showPreview(frame);
    }

private:
    void loadModels()
    {
        const QString dir = QCoreApplication::applicationDirPath();
        const QString palmPath = dir + QStringLiteral("/plugins/Models/palm_detection_mediapipe_2023feb.onnx");
        const QString handPath = dir + QStringLiteral("/plugins/Models/handpose_estimation_mediapipe_2023feb.onnx");
        if (!QFileInfo::exists(palmPath) || !QFileInfo::exists(handPath)) {
            statusLabel->setText(
                QStringLiteral("找不到手部模型：请把本程序放在 Flow 的 bin 目录下运行（需要 plugins/Models/ 中的两个 ONNX）"));
            hintLabel->setText(QString());
            return;
        }
        try {
            palmDetector = std::make_unique<MediaPipeHand::PalmDetector>(
                palmPath.toStdString(), 0.3f, 0.3f, 5000, false);
            handEstimator = std::make_unique<MediaPipeHand::HandPoseEstimator>(
                handPath.toStdString(), 0.3f, false);
            if (palmDetector->isLoaded() && handEstimator->isLoaded()) {
                modelsLoaded = true;
                statusLabel->setText(QStringLiteral("模型已加载（CPU）"));
            }
        } catch (const cv::Exception& e) {
            statusLabel->setText(QStringLiteral("模型加载失败: %1").arg(e.what()));
        }
    }

    void sampleCurrent(const std::array<float, GestureFeature::kDim>& feature)
    {
        const int row = gestureList->currentRow();
        if (row < 0 || row >= m_library.gestures.size()) {
            statusLabel->setText(QStringLiteral("请先在列表中选中要采样的标签"));
            return;
        }
        if (!m_sampleTimer.isValid() || m_sampleTimer.elapsed() >= 200) {
            m_sampleTimer.start();
            auto& g = m_library.gestures[row];
            // 一次采集入库 5 个增广模板（旋转/缩放/噪声），等效多角度采样
            GestureFeature::augmentVariants(feature, g.samples);
            m_dirty = true;
            refreshLibraryUi();
            statusLabel->setText(
                QStringLiteral("已采样 → %1（+5 模板，共 %2 帧）")
                    .arg(g.label)
                    .arg(g.samples.size()));
        }
    }

    void showPreview(const cv::Mat& frame, const QString& liveMatch = QString())
    {
        QImage image = matToQImage(frame);
        if (image.isNull()) {
            return;
        }
        QPainter painter(&image);
        painter.setFont(QFont(QString(), 14, QFont::Bold));
        QString text;
        const int row = gestureList->currentRow();
        if (row >= 0 && row < m_library.gestures.size()) {
            const auto& g = m_library.gestures[row];
            text = QStringLiteral("采样目标: %1（%2 帧）").arg(g.label).arg(g.samples.size());
        } else {
            text = QStringLiteral("未选中标签");
        }
        if (!m_libraryPath.isEmpty()) {
            text += QStringLiteral("    库: %1").arg(QFileInfo(m_libraryPath).fileName());
        }
        painter.setPen(Qt::yellow);
        painter.drawText(12, 28, text);
        // 实时显示当前画面与全库的最近匹配（调阈值用）
        if (!liveMatch.isEmpty()) {
            painter.setFont(QFont(QString(), 12));
            painter.setPen(Qt::green);
            painter.drawText(12, 52, liveMatch);
        }
        painter.end();

        preview->setPixmap(QPixmap::fromImage(image).scaled(
            preview->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }

    /** 当前画面特征与全库最近匹配（双朝向，预览提示，不影响录制） */
    QString liveNearest(const std::optional<std::array<float, GestureFeature::kDim>>& feature)
    {
        if (!feature.has_value() || m_library.gestures.isEmpty()) {
            return QString();
        }
        const auto mirror = GestureFeature::mirrored(*feature);
        QString bestLabel;
        float bestScore = -1.f;
        for (const auto& g : m_library.gestures) {
            for (const auto& sample : g.samples) {
                const float s = std::max(GestureFeature::cosine(*feature, sample),
                                         GestureFeature::cosine(mirror, sample));
                if (s > bestScore) {
                    bestScore = s;
                    bestLabel = g.label;
                }
            }
        }
        if (bestLabel.isEmpty()) {
            return QStringLiteral("库为空");
        }
        return QStringLiteral("最接近: %1（%2%）")
            .arg(bestLabel)
            .arg(static_cast<int>(bestScore * 100.f + 0.5f));
    }

    void refreshLibraryUi()
    {
        const int selected = gestureList->currentRow();
        gestureList->clear();
        for (const auto& g : m_library.gestures) {
            gestureList->addItem(QStringLiteral("%1（%2 帧）").arg(g.label).arg(g.samples.size()));
        }
        if (selected >= 0 && selected < gestureList->count()) {
            gestureList->setCurrentRow(selected);
        }
        fileLabel->setText(m_libraryPath.isEmpty()
                               ? QStringLiteral("未选择库文件（采样前会提示保存）")
                               : m_libraryPath);
        hintLabel->setText(m_library.gestures.isEmpty()
                               ? QStringLiteral("步骤：① 添加标签 ② 选中 ③ 比划并按住采样 ④ 保存")
                               : QStringLiteral("换个角度/距离再采几帧，识别会更稳。"));
        update();
    }

    bool confirmDiscardIfDirty()
    {
        if (!m_dirty) {
            return true;
        }
        const auto ret = QMessageBox::question(
            this, QStringLiteral("未保存"),
            QStringLiteral("当前动作库有未保存的修改，是否继续？"),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        if (ret == QMessageBox::Save) {
            saveLibrary(m_libraryPath);
            return !m_dirty;
        }
        return ret == QMessageBox::Discard;
    }

    QLabel* preview = nullptr;
    QLabel* hintLabel = nullptr;
    QLabel* fileLabel = nullptr;
    QLabel* statusLabel = nullptr;
    QComboBox* cameraCombo = nullptr;
    QPushButton* openCameraBtn = nullptr;
    QPushButton* openVideoBtn = nullptr;
    QPushButton* stopBtn = nullptr;
    QPushButton* openLibBtn = nullptr;
    QPushButton* saveBtn = nullptr;
    QListWidget* gestureList = nullptr;
    QLineEdit* labelEdit = nullptr;
    QPushButton* addLabelBtn = nullptr;
    QPushButton* removeLabelBtn = nullptr;
    QPushButton* undoSampleBtn = nullptr;
    FloatDragValueWidget* thresholdSpin = nullptr;
    QPushButton* sampleBtn = nullptr;
    QTimer* frameTimer = nullptr;

    std::unique_ptr<cv::VideoCapture> capture;
    std::unique_ptr<MediaPipeHand::PalmDetector> palmDetector;
    std::unique_ptr<MediaPipeHand::HandPoseEstimator> handEstimator;
    bool modelsLoaded = false;

    GestureLibrary::Library m_library;
    QString m_libraryPath;
    bool m_dirty = false;
    bool m_sampling = false;
    QElapsedTimer m_sampleTimer;
    std::optional<std::array<float, GestureFeature::kDim>> m_lastFeature;
};

#include "main.moc"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("FlowGestureRecorder"));
    QApplication::setWindowIcon(QIcon(QStringLiteral(":/recorder/finger.png")));
    RecorderWindow window;
    window.show();
    return QApplication::exec();
}
