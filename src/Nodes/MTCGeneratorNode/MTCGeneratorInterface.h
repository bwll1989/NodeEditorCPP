#ifndef MTCGENERATORINTERFACE_H
#define MTCGENERATORINTERFACE_H

#include <QWidget>
#include "Common/AppConfig/TimeCodeDefines.h"
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include "Common/GUI/Elements/IntDragValueWidget/IntDragValueWidget.hpp"

namespace Nodes
{
    /**
     * @brief MTC 生成器 UI 面板
     * - 显示当前时间码与状态
     * - MIDI 输出设备选择 / 时间码类型 / 帧偏移 / 启停 / 归零
     */
    class MTCGeneratorInterface : public QWidget
    {
        Q_OBJECT

    public:
        explicit MTCGeneratorInterface(QWidget *parent = nullptr);
        ~MTCGeneratorInterface();

        QLabel *timeCodeLabel {nullptr};
        QLabel *timeCodeStatusLabel {nullptr};
        IntDragValueWidget *timeCodeOffsetSpinBox {nullptr};
        QComboBox *timeCodeTypeComboBox {nullptr};
        QComboBox *midiDeviceComboBox {nullptr};
        QPushButton *startButton {nullptr};
        QPushButton *resetButton {nullptr};

        void setRunningChecked(bool running);

        QStringList timecodeTypeLabels() const;
        QString timecodeTypeToLabel(TimeCodeType type) const;
        TimeCodeType timecodeTypeFromLabel(const QString& label, TimeCodeType defaultType = TimeCodeType::PAL) const;

    signals:
        /** @brief 启停切换（checkable 按钮 / 外部同步） */
        void runningToggled(bool running);
        /** @brief 请求时间码归零 */
        void resetRequested();
        void timeCodeTypeChanged(TimeCodeType type);
        void midiDeviceChanged(const QString &device);

    public slots:
        void setStatus(bool error, QString message);
        void setTimeStamp(TimeCodeFrame frame);
    };
}
#endif // MTCGENERATORINTERFACE_H
