#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QWidget>

namespace Nodes
{
/**
 * @brief Web Render 节点嵌入面板（全部参数平铺显示）
 *
 * 原 Reload Source / Reload Page / Restart 三个按钮合并为一个 Reload：
 * 按当前状态自动选择最轻量的恢复路径（见 WebRenderDataModel::reloadNow），
 * Shift+点击 = 强制重启浏览器进程。
 */
class WebRenderInterface : public QGroupBox
{
    Q_OBJECT

public:
    explicit WebRenderInterface(QWidget* parent = nullptr)
        : QGroupBox(QStringLiteral("Web Render"), parent)
    {
        m_activeCheck = new QCheckBox(QStringLiteral("Active"), this);
        m_sourceCombo = new QComboBox(this);
        m_sourceCombo->addItem(QStringLiteral("URL or File"), QStringLiteral("url"));
        m_sourceCombo->addItem(QStringLiteral("HTML (DAT)"), QStringLiteral("html"));

        m_urlEdit = new QLineEdit(this);
        m_urlEdit->setPlaceholderText(QStringLiteral("https://… 或 file:///…"));
        m_urlEdit->setText(QStringLiteral("https://example.com"));

        m_htmlEdit = new QPlainTextEdit(this);
        m_htmlEdit->setPlaceholderText(QStringLiteral("<html>…</html>"));
        m_htmlEdit->setMaximumHeight(90);
        m_htmlEdit->setVisible(false);

        m_widthSpin = new QSpinBox(this);
        m_widthSpin->setRange(16, 7680);
        m_widthSpin->setValue(1280);
        m_heightSpin = new QSpinBox(this);
        m_heightSpin->setRange(16, 4320);
        m_heightSpin->setValue(720);

        // ---- 进阶参数（始终显示）----
        m_fpsSpin = new QDoubleSpinBox(this);
        m_fpsSpin->setRange(0.5, 60.0);
        m_fpsSpin->setDecimals(1);
        m_fpsSpin->setSingleStep(1.0);
        m_fpsSpin->setValue(15.0);

        m_dpiSpin = new QDoubleSpinBox(this);
        m_dpiSpin->setRange(0.25, 4.0);
        m_dpiSpin->setDecimals(2);
        m_dpiSpin->setSingleStep(0.25);
        m_dpiSpin->setValue(1.0);

        m_updateWhenLoadedCheck = new QCheckBox(QStringLiteral("Only Update when Loaded"), this);
        m_updateWhenLoadedCheck->setChecked(true);
        m_autoRestartCheck = new QCheckBox(QStringLiteral("Restart if Process Died"), this);
        m_autoRestartCheck->setChecked(true);

        m_reloadBtn = new QPushButton(QStringLiteral("Reload"), this);
        m_reloadBtn->setToolTip(QStringLiteral(
            "重新加载内容（按当前状态自动选择最轻量方式；进程异常时自动重启）\n"
            "按住 Shift 点击 = 强制重启浏览器进程"));

        m_statusLabel = new QLabel(QStringLiteral("状态: 未启动"), this);
        m_statusLabel->setWordWrap(true);
        m_statusLabel->setStyleSheet(QStringLiteral("color: #c44; font-weight: bold;"));

        auto* sizeRow = new QHBoxLayout();
        sizeRow->addWidget(m_widthSpin);
        sizeRow->addWidget(new QLabel(QStringLiteral("×"), this));
        sizeRow->addWidget(m_heightSpin);

        auto* mainForm = new QFormLayout();
        mainForm->setContentsMargins(0, 0, 0, 0);
        mainForm->setSpacing(4);
        mainForm->addRow(QStringLiteral("Source"), m_sourceCombo);
        mainForm->addRow(QStringLiteral("URL"), m_urlEdit);
        mainForm->addRow(QStringLiteral("HTML"), m_htmlEdit);
        mainForm->addRow(QStringLiteral("Resolution"), sizeRow);
        mainForm->addRow(QStringLiteral("Max FPS"), m_fpsSpin);
        mainForm->addRow(QStringLiteral("DPI Scale"), m_dpiSpin);

        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(6, 8, 6, 6);
        layout->setSpacing(6);
        layout->addWidget(m_activeCheck);
        layout->addLayout(mainForm);
        layout->addWidget(m_updateWhenLoadedCheck);
        layout->addWidget(m_autoRestartCheck);
        layout->addWidget(m_reloadBtn);
        layout->addWidget(m_statusLabel);

        setMinimumSize(300, 320);
        setMaximumWidth(400);

        connect(m_sourceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
            const bool htmlMode = m_sourceCombo->currentData().toString() == QLatin1String("html");
            m_urlEdit->setVisible(!htmlMode);
            m_htmlEdit->setVisible(htmlMode);
            emit sourceModeChanged(m_sourceCombo->currentData().toString());
        });
        connect(m_activeCheck, &QCheckBox::toggled, this, &WebRenderInterface::activeChanged);
        connect(m_urlEdit, &QLineEdit::editingFinished, this, [this]() {
            emit urlChanged(m_urlEdit->text());
        });
        connect(m_htmlEdit, &QPlainTextEdit::textChanged, this, [this]() {
            emit htmlChanged(m_htmlEdit->toPlainText());
        });
        connect(m_widthSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &WebRenderInterface::widthChanged);
        connect(m_heightSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &WebRenderInterface::heightChanged);
        connect(m_fpsSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &WebRenderInterface::maxFpsChanged);
        connect(m_dpiSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &WebRenderInterface::dpiScaleChanged);
        connect(m_updateWhenLoadedCheck, &QCheckBox::toggled, this, &WebRenderInterface::updateWhenLoadedChanged);
        connect(m_autoRestartCheck, &QCheckBox::toggled, this, &WebRenderInterface::autoRestartChanged);
        connect(m_reloadBtn, &QPushButton::clicked, this, &WebRenderInterface::reloadRequested);
    }

signals:
    void activeChanged(bool active);
    void sourceModeChanged(const QString& mode);
    void urlChanged(const QString& url);
    void htmlChanged(const QString& html);
    void widthChanged(int width);
    void heightChanged(int height);
    void maxFpsChanged(double fps);
    void dpiScaleChanged(double scale);
    void updateWhenLoadedChanged(bool enabled);
    void autoRestartChanged(bool enabled);
    /** @brief 一键重载（Shift+点击时模型侧转为强制重启进程） */
    void reloadRequested();

public slots:
    void setActiveChecked(bool active)
    {
        QSignalBlocker b(m_activeCheck);
        m_activeCheck->setChecked(active);
    }

    void setStatus(const QString& text, bool ok)
    {
        m_statusLabel->setText(text);
        m_statusLabel->setStyleSheet(ok
            ? QStringLiteral("color: #2a7; font-weight: bold;")
            : QStringLiteral("color: #c44; font-weight: bold;"));
    }

public:
    QCheckBox* m_activeCheck = nullptr;
    QComboBox* m_sourceCombo = nullptr;
    QLineEdit* m_urlEdit = nullptr;
    QPlainTextEdit* m_htmlEdit = nullptr;
    QSpinBox* m_widthSpin = nullptr;
    QSpinBox* m_heightSpin = nullptr;
    QDoubleSpinBox* m_fpsSpin = nullptr;
    QDoubleSpinBox* m_dpiSpin = nullptr;
    QCheckBox* m_updateWhenLoadedCheck = nullptr;
    QCheckBox* m_autoRestartCheck = nullptr;
    QPushButton* m_reloadBtn = nullptr;
    QLabel* m_statusLabel = nullptr;
};
} // namespace Nodes
