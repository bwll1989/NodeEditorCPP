#pragma once

#include <QDialog>
#include <QSpinBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QListWidget>
#include <QStackedWidget>

class SettingWidget : public QDialog {
    Q_OBJECT
public:
    explicit SettingWidget(QWidget *parent = nullptr);
    static void showDialog(QWidget* parent);

private slots:
    void saveSettings();
    void resetToDefaults();

private:
    void loadCurrentSettings();

    // UI Structure
    QListWidget* m_listWidget;
    QStackedWidget* m_stackedWidget;
    QDialogButtonBox* m_buttonBox;

    // General Settings
    QSpinBox* m_maxRecentFilesSpin;
    QCheckBox* m_darkThemeCheck;

    // Network Settings
    QSpinBox* m_httpPortSpin;
    QLineEdit* m_extraFeedbackHostEdit;
    QSpinBox* m_extraFeedbackPortSpin;
    QSpinBox* m_extraControlPortSpin;
    QLineEdit* m_oscInternalHostEdit;

    // Log Settings
    QSpinBox* m_maxLogEntriesSpin;
};
