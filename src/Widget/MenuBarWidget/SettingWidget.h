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
    void resetToDefaults();

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

    // Log Settings
    IntDragValueWidget* m_maxLogEntriesSpin;
};
