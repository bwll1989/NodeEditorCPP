//
// Created by WuBin on 2025/12/25.
//

#ifndef SYSTEMINFOWIDGET_H
#define SYSTEMINFOWIDGET_H

#include <QDialog>
#include <QString>
#include <vector>

class SystemInfoWidget : public QDialog {
    Q_OBJECT
public:
    // 函数级注释：构造系统信息弹窗，初始化布局与控件
    explicit SystemInfoWidget(QWidget* parent = nullptr);
    // 函数级注释：静态便捷方法，创建并显示系统信息弹窗
    static void showDialog(QWidget* parent = nullptr);
private:
    // 函数级注释：采集并填充所有系统信息到界面
    void populate();
    // 函数级注释：获取操作系统信息（使用 Windows API RtlGetVersion）
    QString getOSInfo() const;
    // 函数级注释：获取CPU信息（使用 Windows Registry 和 GetLogicalProcessorInformation）
    QString getCPUInfo() const;
    // 函数级注释：获取内存信息（使用 Windows API GlobalMemoryStatusEx）
    QString getMemoryInfo() const;
    // 函数级注释：获取显卡信息（使用 Windows API EnumDisplayDevices，包括驱动版本）
    QString getGPUInfo() const;
    // 函数级注释：获取硬盘信息（使用 Windows API GetDiskFreeSpaceEx）
    QString getDiskInfo() const;
    // 函数级注释：获取网络信息（使用 Windows API GetAdaptersAddresses，包括网关）
    QString getNetworkInfo() const;
    // 函数级注释：获取 OpenCV 的 GPU 能力信息（CUDA/OpenCL/UMat）
    QString getOpenCvInfo() const;
    // 函数级注释：获取系统可调用的 FFmpeg 可执行文件信息（路径与版本）
    QString getFfmpegInfo() const;
};

#endif //SYSTEMINFOWIDGET_H
