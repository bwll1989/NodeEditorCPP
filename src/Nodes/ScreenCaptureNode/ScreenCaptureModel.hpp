#pragma once

/**
 * ScreenCaptureModel — 显示器 / 窗口捕获 → ImageTimestampRingQueue
 *
 * 优先使用 Windows Graphics Capture（与 OBS 窗口捕获、显示器捕获相同的系统接口）。
 * 不可用时退回 PrintWindow / BitBlt。帧在 GUI 线程经 ImageFrame::fromBgra8Mat 上传。
 * 下游按 TimestampGenerator tick 取最新帧，不逐帧 dataUpdated。
 */

#include "ScreenCapture.h"

#include "Elements/IntDragValueWidget/IntDragValueWidget.hpp"

#include "Common/BaseClass/AbstractDelegateModel.h"
#include "Common/DataTypes/ImageGpuUpload.h"
#include "Common/DataTypes/NodeDataList.hpp"
#include "TimestampGenerator/TimestampGenerator.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QJsonObject>
#include <QLabel>
#include <QMutex>
#include <QPointer>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>
#include <atomic>

using namespace NodeDataTypes;

namespace Nodes
{
class ScreenCaptureModel final : public AbstractDelegateModel {
    Q_OBJECT

public:
    ScreenCaptureModel()
    {
        InPortCount = 0;
        OutPortCount = 1;
        CaptionVisible = true;
        Caption = QStringLiteral("Screen Capture");
        WidgetEmbeddable = false;
        Resizable = false;
        PortEditable = false;

        qRegisterMetaType<cv::Mat>("cv::Mat");
        ensureImageDataBuffer(m_outImageData, m_outputBuffer);
        ImageGpuUpload::instance().warmup();

        m_thread = new ScreenCaptureThread(this);
        connect(m_thread, &ScreenCaptureThread::frameAvailable, this, &ScreenCaptureModel::onFrameAvailable, Qt::QueuedConnection);
        connect(m_thread, &ScreenCaptureThread::captureStateChanged, this, &ScreenCaptureModel::onCaptureStateChanged, Qt::QueuedConnection);
        connect(m_thread, &ScreenCaptureThread::errorOccurred, this, &ScreenCaptureModel::onCaptureError, Qt::QueuedConnection);
    }

    ~ScreenCaptureModel() override
    {
        stopCapture();
        if (m_thread) {
            m_thread->stop();
            m_thread->wait(8000);
        }
    }

    QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
    {
        Q_UNUSED(portType);
        Q_UNUSED(portIndex);
        return ImageData().type();
    }

    QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
    {
        Q_UNUSED(portIndex);
        if (portType == QtNodes::PortType::Out) {
            return QStringLiteral("IMAGE");
        }
        return {};
    }

    void setInData(std::shared_ptr<QtNodes::NodeData>, const QtNodes::PortIndex portIndex) override
    {
        Q_UNUSED(portIndex);
    }

    std::shared_ptr<QtNodes::NodeData> outData(const QtNodes::PortIndex port) override
    {
        Q_UNUSED(port);
        return m_outImageData;
    }

    QJsonObject save() const override
    {
        QJsonObject modelJson = NodeDelegateModel::save();
        QJsonObject values;
        values[QStringLiteral("kind")] = m_kind == ScreenSource::Kind::Window ? QStringLiteral("window") : QStringLiteral("display");
        values[QStringLiteral("sourceId")] = m_sourceId;
        values[QStringLiteral("displayName")] = m_displayName;
        values[QStringLiteral("cursor")] = m_cursor;
        values[QStringLiteral("fps")] = m_fps;
        modelJson[QStringLiteral("values")] = values;
        return modelJson;
    }

    void load(const QJsonObject& jsonObject) override
    {
        NodeDelegateModel::load(jsonObject);
        const QJsonObject values = jsonObject.value(QStringLiteral("values")).toObject();
        const QString kind = values.value(QStringLiteral("kind")).toString();
        if (!kind.isEmpty()) {
            m_kind = kind == QStringLiteral("window") ? ScreenSource::Kind::Window : ScreenSource::Kind::Display;
        }
        m_sourceId = values.value(QStringLiteral("sourceId")).toString();
        m_displayName = values.value(QStringLiteral("displayName")).toString();
        if (values.contains(QStringLiteral("cursor"))) {
            m_cursor = values.value(QStringLiteral("cursor")).toBool(true);
        }
        if (values.contains(QStringLiteral("fps"))) {
            m_fps = std::clamp(values.value(QStringLiteral("fps")).toInt(30), 1, 60);
        }

        // 视图会在 nodeCreated 时先创建属性面板，此时还是默认的 30 fps / 显示光标。
        // load 发生在面板创建之后，必须把已读出的值写回控件，否则界面仍显示默认值。
        restoreWidgetsFromState();

        QTimer::singleShot(0, this, [self = QPointer<ScreenCaptureModel>(this)]() {
            if (self) {
                self->ensureCaptureStarted();
            }
        });
    }

    QWidget* embeddedWidget() override
    {
        if (!m_widget) {
            m_widget = new QWidget();
            auto* layout = new QVBoxLayout(m_widget);
            layout->setContentsMargins(0, 0, 0, 0);
            layout->setSpacing(4);

            auto* kindRow = new QHBoxLayout();
            m_kindBox = new QComboBox(m_widget);
            m_kindBox->addItem(QStringLiteral("显示器"), QStringLiteral("display"));
            m_kindBox->addItem(QStringLiteral("窗口"), QStringLiteral("window"));
            m_refreshButton = new QPushButton(QStringLiteral("刷新"), m_widget);
            kindRow->addWidget(m_kindBox, 1);
            kindRow->addWidget(m_refreshButton);
            layout->addLayout(kindRow);

            m_sourceBox = new QComboBox(m_widget);
            m_sourceBox->setMinimumWidth(220);
            layout->addWidget(m_sourceBox);

            auto* optionRow = new QHBoxLayout();
            m_cursorBox = new QCheckBox(QStringLiteral("鼠标光标"), m_widget);
            m_fpsBox = new IntDragValueWidget(m_widget);
            m_fpsBox->setRange(1, 60);
            m_fpsBox->setSingleStep(1);
            m_fpsBox->setSuffix(QStringLiteral(" fps"));
            m_fpsBox->setMinimumWidth(72);
            optionRow->addWidget(m_cursorBox);
            optionRow->addStretch(1);
            optionRow->addWidget(m_fpsBox);
            layout->addLayout(optionRow);

            m_status = new QLabel(QStringLiteral("未采集"), m_widget);
            m_status->setWordWrap(true);
            layout->addWidget(m_status);

            {
                QSignalBlocker kindBlocker(m_kindBox);
                QSignalBlocker cursorBlocker(m_cursorBox);
                QSignalBlocker fpsBlocker(m_fpsBox);
                m_kindBox->setCurrentIndex(m_kind == ScreenSource::Kind::Window ? 1 : 0);
                m_cursorBox->setChecked(m_cursor);
                m_fpsBox->setValue(m_fps);
            }
            refillSources();
            selectSavedSource();

            connect(m_kindBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ScreenCaptureModel::onKindChanged);
            connect(m_sourceBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ScreenCaptureModel::onSourceChanged);
            connect(m_refreshButton, &QPushButton::clicked, this, &ScreenCaptureModel::onRefresh);
            connect(m_cursorBox, &QCheckBox::toggled, this, &ScreenCaptureModel::onCursorChanged);
            connect(m_fpsBox, &IntDragValueWidget::valueChanged, this, &ScreenCaptureModel::onFpsChanged);

            ensureCaptureStarted();
            updateStatusLabel();
        }
        return m_widget;
    }

private slots:
    void onKindChanged(int index)
    {
        if (!m_kindBox || index < 0) {
            return;
        }
        const QString kind = m_kindBox->itemData(index).toString();
        m_kind = kind == QStringLiteral("window") ? ScreenSource::Kind::Window : ScreenSource::Kind::Display;
        refillSources();
        if (m_listed.isEmpty()) {
            m_sourceId.clear();
            m_displayName.clear();
            stopCapture();
            m_lastError = m_kind == ScreenSource::Kind::Window ? QStringLiteral("无可用窗口")
                                                              : QStringLiteral("无可用显示器");
            updateStatusLabel();
            return;
        }
        if (m_sourceBox) {
            QSignalBlocker blocker(m_sourceBox);
            m_sourceBox->setCurrentIndex(0);
        }
        m_sourceId = m_listed.first().id;
        m_displayName = m_listed.first().label;
        beginCapture(m_listed.first());
    }

    void onSourceChanged(int index)
    {
        if (!m_sourceBox || index < 0) {
            return;
        }
        const QString id = m_sourceBox->itemData(index).toString();
        if (id.isEmpty()) {
            return;
        }
        for (const ScreenSource& source : m_listed) {
            if (source.id == id) {
                m_sourceId = source.id;
                m_displayName = source.label;
                m_kind = source.kind;
                beginCapture(source);
                return;
            }
        }
    }

    void onRefresh()
    {
        const QString keep = m_sourceId;
        refillSources();
        if (m_sourceBox) {
            QSignalBlocker blocker(m_sourceBox);
            const int index = m_sourceBox->findData(keep);
            if (index >= 0) {
                m_sourceBox->setCurrentIndex(index);
            }
        }
        ensureCaptureStarted();
    }

    void onCursorChanged(bool checked)
    {
        m_cursor = checked;
        ensureCaptureStarted();
    }

    void onFpsChanged(int fps)
    {
        m_fps = std::clamp(fps, 1, 60);
        if (m_thread) {
            m_thread->setFps(m_fps);
        }
    }

    void onFrameAvailable(const cv::Mat& frame)
    {
        if (frame.empty()) {
            return;
        }
        {
            QMutexLocker locker(&m_pendingMutex);
            m_pendingFrame = frame;
        }
        scheduleUploadIfNeeded();
    }

    void onCaptureStateChanged(bool capturing, const QString& method)
    {
        m_running = capturing;
        m_method = method;
        if (capturing) {
            m_lastError.clear();
        }
        updateStatusLabel();
    }

    void onCaptureError(const QString& message)
    {
        qWarning() << "Screen Capture:" << message;
        m_lastError = message;
        updateStatusLabel();
    }

    void publishPendingFrame()
    {
        m_uploadScheduled.store(false);

        cv::Mat mat;
        {
            QMutexLocker locker(&m_pendingMutex);
            mat = std::move(m_pendingFrame);
        }
        if (!m_outputBuffer || mat.empty()) {
            return;
        }

        ensureImageDataBuffer(m_outImageData, m_outputBuffer);
        const qint64 timestamp = mediaOutputTimestamp(TimestampGenerator::getInstance()->getCurrentFrameCount());
        ImageFrame imageFrame = ImageFrame::fromBgra8Mat(std::move(mat), timestamp);
        if (!imageFrame.texture.valid()) {
            if (!m_uploadFailed) {
                m_uploadFailed = true;
                qWarning() << "Screen Capture: GPU upload failed";
            }
            return;
        }
        m_uploadFailed = false;
        if (imageFrame.texture.width != m_frameW || imageFrame.texture.height != m_frameH) {
            m_frameW = imageFrame.texture.width;
            m_frameH = imageFrame.texture.height;
            updateStatusLabel();
        }
        m_outputBuffer->pushFrame(std::move(imageFrame));

        bool hasPending = false;
        {
            QMutexLocker locker(&m_pendingMutex);
            hasPending = !m_pendingFrame.empty();
        }
        if (hasPending) {
            scheduleUploadIfNeeded();
        }
    }

private:
    void refillSources()
    {
        m_listed.clear();
        if (!m_sourceBox) {
            return;
        }
        QSignalBlocker blocker(m_sourceBox);
        m_sourceBox->clear();
        m_listed = m_kind == ScreenSource::Kind::Window ? ScreenCaptureThread::listWindows()
                                                        : ScreenCaptureThread::listDisplays();
        if (m_listed.isEmpty()) {
            m_sourceBox->addItem(m_kind == ScreenSource::Kind::Window ? QStringLiteral("无可用窗口")
                                                                      : QStringLiteral("无可用显示器"));
            return;
        }
        for (const ScreenSource& source : m_listed) {
            m_sourceBox->addItem(source.label, source.id);
        }
    }

    void selectSavedSource()
    {
        if (!m_sourceBox || m_sourceId.isEmpty()) {
            return;
        }
        int index = m_sourceBox->findData(m_sourceId);
        if (index < 0 && !m_displayName.isEmpty()) {
            index = m_sourceBox->findText(m_displayName);
        }
        if (index < 0) {
            m_sourceBox->addItem(m_displayName.isEmpty() ? m_sourceId : m_displayName, m_sourceId);
            index = m_sourceBox->count() - 1;
        }
        QSignalBlocker blocker(m_sourceBox);
        m_sourceBox->setCurrentIndex(index);
    }

    bool findSource(ScreenSource& source) const
    {
        source.kind = m_kind;
        source.id = m_sourceId;
        source.label = m_displayName;
        for (const ScreenSource& listed : m_listed) {
            if (listed.id == m_sourceId) {
                source = listed;
                return true;
            }
        }
        return ScreenCaptureThread::resolve(source);
    }

    void syncOptionWidgets()
    {
        if (m_kindBox) {
            QSignalBlocker blocker(m_kindBox);
            const int kindIndex = m_kind == ScreenSource::Kind::Window ? 1 : 0;
            if (m_kindBox->currentIndex() != kindIndex) {
                m_kindBox->setCurrentIndex(kindIndex);
            }
        }
        if (m_cursorBox) {
            QSignalBlocker blocker(m_cursorBox);
            if (m_cursorBox->isChecked() != m_cursor) {
                m_cursorBox->setChecked(m_cursor);
            }
        }
        if (m_fpsBox) {
            QSignalBlocker blocker(m_fpsBox);
            if (m_fpsBox->value() != m_fps) {
                m_fpsBox->setValue(m_fps);
            }
        }
    }

    void restoreWidgetsFromState()
    {
        if (!m_widget || !m_kindBox) {
            return;
        }
        const int kindIndex = m_kind == ScreenSource::Kind::Window ? 1 : 0;
        const bool kindDiffers = m_kindBox->currentIndex() != kindIndex;
        syncOptionWidgets();
        if (kindDiffers || m_listed.isEmpty()) {
            refillSources();
        }
        selectSavedSource();
    }

    void ensureCaptureStarted()
    {
        restoreWidgetsFromState();
        if (m_sourceId.isEmpty()) {
            const QList<ScreenSource> displays = ScreenCaptureThread::listDisplays();
            if (displays.isEmpty()) {
                m_lastError = QStringLiteral("没有可用显示器");
                updateStatusLabel();
                return;
            }
            m_kind = ScreenSource::Kind::Display;
            m_sourceId = displays.first().id;
            m_displayName = displays.first().label;
            if (m_kindBox) {
                QSignalBlocker blocker(m_kindBox);
                m_kindBox->setCurrentIndex(0);
            }
            refillSources();
            selectSavedSource();
        }

        ScreenSource source;
        if (!findSource(source)) {
            qWarning() << "Screen Capture: source not found" << m_sourceId;
            m_lastError = QStringLiteral("找不到捕获源");
            updateStatusLabel();
            return;
        }
        m_sourceId = source.id;
        m_displayName = source.label;
        m_kind = source.kind;
        selectSavedSource();
        beginCapture(source);
    }

    void beginCapture(const ScreenSource& source)
    {
        if (!m_thread) {
            return;
        }
        if (source.id == m_activeId && source.nativeHandle == m_activeHandle && m_cursor == m_activeCursor && m_thread->isRunning()) {
            m_thread->setFps(m_fps);
            return;
        }
        m_activeId = source.id;
        m_activeHandle = source.nativeHandle;
        m_activeCursor = m_cursor;
        m_lastError.clear();
        m_thread->startCapture(source, m_cursor, m_fps);
    }

    void stopCapture()
    {
        m_activeId.clear();
        m_activeHandle = 0;
        if (m_thread) {
            m_thread->stop();
        }
        {
            QMutexLocker locker(&m_pendingMutex);
            m_pendingFrame.release();
        }
        m_uploadScheduled.store(false);
        if (m_outputBuffer) {
            m_outputBuffer->clear();
        }
    }

    void scheduleUploadIfNeeded()
    {
        if (!m_uploadScheduled.exchange(true)) {
            QMetaObject::invokeMethod(this, "publishPendingFrame", Qt::QueuedConnection);
        }
    }

    void updateStatusLabel()
    {
        if (!m_status) {
            return;
        }
        if (!m_running && !m_lastError.isEmpty()) {
            m_status->setText(m_lastError);
            return;
        }
        if (m_running) {
            QString text = QStringLiteral("采集中");
            if (!m_method.isEmpty()) {
                text += QStringLiteral(" · ") + m_method;
            }
            if (m_frameW > 0 && m_frameH > 0) {
                text += QStringLiteral(" · %1×%2").arg(m_frameW).arg(m_frameH);
            }
            m_status->setText(text);
            return;
        }
        m_status->setText(m_lastError.isEmpty() ? QStringLiteral("未采集") : m_lastError);
    }

    QPointer<QWidget> m_widget;
    QComboBox* m_kindBox = nullptr;
    QComboBox* m_sourceBox = nullptr;
    QPushButton* m_refreshButton = nullptr;
    QCheckBox* m_cursorBox = nullptr;
    IntDragValueWidget* m_fpsBox = nullptr;
    QLabel* m_status = nullptr;
    QList<ScreenSource> m_listed;

    ScreenCaptureThread* m_thread = nullptr;
    ScreenSource::Kind m_kind = ScreenSource::Kind::Display;
    QString m_sourceId;
    QString m_displayName;
    bool m_cursor = true;
    int m_fps = 30;

    QString m_activeId;
    quintptr m_activeHandle = 0;
    bool m_activeCursor = true;
    bool m_running = false;
    QString m_method;
    QString m_lastError;
    int m_frameW = 0;
    int m_frameH = 0;
    bool m_uploadFailed = false;

    std::shared_ptr<ImageData> m_outImageData;
    std::shared_ptr<ImageTimestampRingQueue> m_outputBuffer;
    QMutex m_pendingMutex;
    cv::Mat m_pendingFrame;
    std::atomic<bool> m_uploadScheduled{false};
};
} // namespace Nodes
