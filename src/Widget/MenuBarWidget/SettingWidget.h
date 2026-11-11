#pragma once

#include <QDialog>
#include <QSpinBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QDialogButtonBox>
#include <QListWidget>
#include <QStackedWidget>
#include <QString>

#include "../GUI/Elements/IntDragValueWidget/IntDragValueWidget.hpp"
class SettingWidget : public QDialog {
    Q_OBJECT
public:
    explicit SettingWidget(QWidget *parent = nullptr);
    static void showDialog(QWidget* parent);

private slots:
    void saveSettings();

private:
    void loadCurrentSettings();

    // UI Structure
    QListWidget* m_listWidget;
    QStackedWidget* m_stackedWidget;
    QDialogButtonBox* m_buttonBox;

    // General Settings
    IntDragValueWidget* m_maxRecentFilesSpin;
    QCheckBox* m_darkThemeCheck;
    QCheckBox* m_autosaveEnabledCheck;
    IntDragValueWidget* m_autosaveIntervalSpin;
    QLabel* m_timestampFrameRateLabel;
    IntDragValueWidget* m_audioOutputDelaySpin;
    QComboBox* m_artnetMergeCombo; ///< Art-Net 多源合并策略（HTP/LTP）
    IntDragValueWidget* m_artnetFpsSpin; ///< Art-Net 发送节拍频率（Hz）

    // Network Settings
    IntDragValueWidget* m_httpPortSpin;
    QLineEdit* m_extraFeedbackHostEdit;
    IntDragValueWidget* m_extraFeedbackPortSpin;
    IntDragValueWidget* m_extraControlPortSpin;
    QLineEdit* m_oscInternalHostEdit;
    QCheckBox* m_oscEnabledCheck;
    QCheckBox* m_mqttEnabledCheck;
    QLineEdit* m_mqttHostEdit;
    IntDragValueWidget* m_mqttPortSpin;
    QLineEdit* m_mqttUsernameEdit;
    QLineEdit* m_mqttPasswordEdit;
    QLineEdit* m_mqttControlTopicEdit;
    QLineEdit* m_mqttFeedbackTopicEdit;
    QLineEdit* m_webPasswordEdit;

    // 授权（LicenseClient SDK：激活码生成 .ft 请求 → 导入 .lic → 状态展示）
    QLineEdit* m_activationCodeEdit;   ///< 激活码输入（持久化，生成 .ft 加密请求用）
    QLabel* m_licenseStatusLabel;      ///< 激活状态（载荷摘要 / 未授权 / 无效原因）
    QLabel* m_sdkStatusLabel;          ///< SDK 原生库缺失时的诊断提示

    void refreshLicenseStatus();       ///< 刷新激活状态显示（配置令牌 > exe 旁 license.lic）

    // Log Settings
    IntDragValueWidget* m_maxLogEntriesSpin;
    IntDragValueWidget* m_maxLogSaveEntriesSpin;
};
