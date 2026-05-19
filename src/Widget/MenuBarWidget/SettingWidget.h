#pragma once

#include <QDialog>
#include <QSpinBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QListWidget>
#include <QStackedWidget>
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

    // Log Settings
    IntDragValueWidget* m_maxLogEntriesSpin;
};
