//
// Created by WuBin on 2025/12/25.
//
#include "SystemInfoWidget.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QProcess>
#include <QScrollArea>
#include <QCoreApplication>
#include <QFileInfo>
#include <QSettings>
#include <QGuiApplication>
#include <QScreen>
#include <QStandardPaths>
#include <QDebug>

#include <opencv2/core.hpp>
#include <opencv2/core/cuda.hpp>
#include <opencv2/core/ocl.hpp>
#ifdef Q_OS_WIN
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#include <iphlpapi.h>
#include <iptypes.h>
#include <malloc.h>
#include <stdio.h>
#endif

// 函数级注释：构造系统信息弹窗，设置标题与基础布局
SystemInfoWidget::SystemInfoWidget(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("系统信息"));
    setMinimumWidth(600);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);

    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    layout->addWidget(scrollArea);

    QWidget* container = new QWidget(scrollArea);
    scrollArea->setWidget(container);

    QVBoxLayout* root = new QVBoxLayout(container);
    root->setSpacing(15);
    root->setContentsMargins(25, 25, 25, 25);

    auto addSection = [&](const QString& label, const QString& content) {
        QHBoxLayout* row = new QHBoxLayout();
        row->setSpacing(15);
        row->setAlignment(Qt::AlignTop);

        // 标题标签
        QLabel* titleLabel = new QLabel(label, container);
        titleLabel->setFixedWidth(50);
        titleLabel->setAlignment(Qt::AlignTop | Qt::AlignRight);
        // 内容标签
        QLabel* contentLabel = new QLabel(content, container);
        contentLabel->setWordWrap(true);
        contentLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
        contentLabel->setTextInteractionFlags(Qt::TextSelectableByMouse); // 允许复制

        row->addWidget(titleLabel);
        row->addWidget(contentLabel, 1);
        root->addLayout(row);

        // 添加分割线（除了最后一项）
        if (label != QStringLiteral("FFmpeg")) {
            QFrame* line = new QFrame(container);
            line->setFrameShape(QFrame::HLine);
            line->setFrameShadow(QFrame::Plain);
            root->addWidget(line);
        }
    };

    addSection(QStringLiteral("系统"), getOSInfo());
    addSection(QStringLiteral("CPU"), getCPUInfo());
    addSection(QStringLiteral("内存"), getMemoryInfo());
    addSection(QStringLiteral("硬盘"), getDiskInfo());
    addSection(QStringLiteral("网络"), getNetworkInfo());
    addSection(QStringLiteral("显卡"), getGPUInfo());
    addSection(QStringLiteral("OpenCV"), getOpenCvInfo());
    addSection(QStringLiteral("FFmpeg"), getFfmpegInfo());

    root->addStretch();
}

// 函数级注释：静态便捷方法，创建并显示系统信息弹窗
static inline QPoint centerOnScreen(QWidget* w) {
    if (QGuiApplication::primaryScreen()) {
        QRect r = QGuiApplication::primaryScreen()->geometry();
        QSize s = w->sizeHint();
        return QPoint(r.center().x() - s.width()/2, r.center().y() - s.height()/2);
    }
    return QPoint(100, 100);
}
void SystemInfoWidget::showDialog(QWidget* parent) {
    SystemInfoWidget dlg(parent);
    dlg.move(centerOnScreen(&dlg));
    dlg.exec();
}

// 函数级注释：采集并填充所有系统信息（当前实现直接在构造时填充）
void SystemInfoWidget::populate() {
    // 预留：若后续采用异步采集，可在此集中更新UI
}

// 函数级注释：获取操作系统信息（使用 Registry 和 Windows API）
QString SystemInfoWidget::getOSInfo() const {
#ifdef Q_OS_WIN
    QString productName = "Windows";
    QString displayVersion;
    QString buildNumber;

    // 从注册表获取产品名称和版本号
    QSettings reg(R"(HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion)", QSettings::NativeFormat);
    productName = reg.value("ProductName").toString();
    displayVersion = reg.value("DisplayVersion").toString();
    if (displayVersion.isEmpty()) {
        displayVersion = reg.value("ReleaseId").toString();
    }

    // 获取构建号
    typedef LONG (WINAPI *RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);
    HMODULE hMod = GetModuleHandleW(L"ntdll.dll");
    if (hMod) {
        RtlGetVersionPtr fxPtr = (RtlGetVersionPtr)GetProcAddress(hMod, "RtlGetVersion");
        if (fxPtr != nullptr) {
            RTL_OSVERSIONINFOW rovi = { 0 };
            rovi.dwOSVersionInfoSize = sizeof(rovi);
            if (fxPtr(&rovi) == 0) {
                buildNumber = QString::number(rovi.dwBuildNumber);
            }
        }
    }

    // 获取系统架构
    QString arch = "x86";
    SYSTEM_INFO si;
    GetNativeSystemInfo(&si);
    if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) {
        arch = "64-bit";
    } else if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64) {
        arch = "ARM64";
    }

    QString fullInfo = productName;
    if (!displayVersion.isEmpty()) {
        fullInfo += " " + displayVersion;
    }
    if (!buildNumber.isEmpty()) {
        fullInfo += QString(" (Build %1)").arg(buildNumber);
    }
    fullInfo += " " + arch;
    
    return fullInfo;
#else
    return QStringLiteral("非Windows系统");
#endif
}

// 函数级注释：获取CPU信息（使用 Windows Registry 和 GetLogicalProcessorInformation）
QString SystemInfoWidget::getCPUInfo() const {
#ifdef Q_OS_WIN
    // 从注册表读取CPU名称
    QString cpuName;
    QSettings reg(R"(HKEY_LOCAL_MACHINE\HARDWARE\DESCRIPTION\System\CentralProcessor\0)", QSettings::NativeFormat);
    cpuName = reg.value("ProcessorNameString").toString();
    if (cpuName.isEmpty()) cpuName = QStringLiteral("未知CPU");

    // 获取核心数和逻辑处理器数
    DWORD returnLength = 0;
    GetLogicalProcessorInformation(nullptr, &returnLength);
    std::vector<SYSTEM_LOGICAL_PROCESSOR_INFORMATION> buffer(returnLength / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION));
    
    int physicalCores = 0;
    int logicalProcessors = 0;

    if (GetLogicalProcessorInformation(buffer.data(), &returnLength)) {
        for (const auto& info : buffer) {
            if (info.Relationship == RelationProcessorCore) {
                physicalCores++;
                // 统计该核心对应的逻辑处理器数
                // ProcessorMask 是位掩码，计算置位位数
                ULONG_PTR mask = info.ProcessorMask;
                int count = 0;
                while (mask) {
                    if (mask & 1) count++;
                    mask >>= 1;
                }
                logicalProcessors += count;
            }
        }
    }

    if (physicalCores > 0 && logicalProcessors > 0) {
        return QString("%1，%2核/%3线程").arg(cpuName).arg(physicalCores).arg(logicalProcessors);
    }
    return cpuName;
#else
    return QStringLiteral("未知CPU");
#endif
}

// 函数级注释：获取内存信息（使用 Windows API GlobalMemoryStatusEx）
QString SystemInfoWidget::getMemoryInfo() const {
#ifdef Q_OS_WIN
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    if (GlobalMemoryStatusEx(&statex)) {
        const double totalGB = static_cast<double>(statex.ullTotalPhys) / (1024.0*1024.0*1024.0);
        const double availGB = static_cast<double>(statex.ullAvailPhys) / (1024.0*1024.0*1024.0);
        return QString("总内存 %1 GB，可用 %2 GB (使用率 %3%)")
            .arg(totalGB, 0, 'f', 2)
            .arg(availGB, 0, 'f', 2)
            .arg(statex.dwMemoryLoad);
    }
#endif
    return QStringLiteral("未知内存");
}

// 函数级注释：获取硬盘信息（使用 Windows API GetDiskFreeSpaceEx）
QString SystemInfoWidget::getDiskInfo() const {
#ifdef Q_OS_WIN
    QStringList lines;
    DWORD drives = GetLogicalDrives();
    for (char letter = 'A'; letter <= 'Z'; letter++) {
        if (drives & 1) {
            QString root = QString("%1:\\").arg(letter);
            UINT type = GetDriveTypeW((LPCWSTR)root.utf16());
            if (type == DRIVE_FIXED) { // 仅显示固定磁盘
                ULARGE_INTEGER freeBytesAvailable, totalNumberOfBytes, totalNumberOfFreeBytes;
                if (GetDiskFreeSpaceExW((LPCWSTR)root.utf16(), &freeBytesAvailable, &totalNumberOfBytes, &totalNumberOfFreeBytes)) {
                    double totalGB = static_cast<double>(totalNumberOfBytes.QuadPart) / (1024.0 * 1024.0 * 1024.0);
                    double freeGB = static_cast<double>(totalNumberOfFreeBytes.QuadPart) / (1024.0 * 1024.0 * 1024.0);
                    lines << QString("%1: 总量 %2 GB，剩余 %3 GB").arg(root).arg(totalGB, 0, 'f', 1).arg(freeGB, 0, 'f', 1);
                }
            }
        }
        drives >>= 1;
    }
    return lines.isEmpty() ? QStringLiteral("未检测到固定磁盘") : lines.join("\n");
#endif
    return QStringLiteral("不支持的平台");
}

// 函数级注释：获取显卡信息（使用 Windows API EnumDisplayDevices，包括驱动版本）
QString SystemInfoWidget::getGPUInfo() const {
#ifdef Q_OS_WIN
    QStringList gpus;
    DISPLAY_DEVICEW dd;
    ZeroMemory(&dd, sizeof(dd));
    dd.cb = sizeof(dd);

    for (int i = 0; EnumDisplayDevicesW(NULL, i, &dd, 0); ++i) {
        if (dd.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER) {
            continue;
        }
        
        QString gpuName = QString::fromWCharArray(dd.DeviceString);
        QString driverVersion = QStringLiteral("未知版本");

        // 尝试从注册表获取驱动版本
        // DeviceKey 类似于 \Registry\Machine\System\CurrentControlSet\Control\Video\{GUID}\0000
        QString deviceKey = QString::fromWCharArray(dd.DeviceKey);
        
        // 转换格式：将 \Registry\Machine 替换为 HKEY_LOCAL_MACHINE
        if (deviceKey.startsWith(QLatin1String("\\Registry\\Machine"), Qt::CaseInsensitive)) {
            deviceKey.replace(0, 17, "HKEY_LOCAL_MACHINE");
        }
        
        if (!deviceKey.isEmpty()) {
            QSettings reg(deviceKey, QSettings::NativeFormat);
            QString ver = reg.value("DriverVersion").toString();
            if (!ver.isEmpty()) {
                driverVersion = ver;
            }
        }

        QString info = QString("%1 (驱动: %2)").arg(gpuName, driverVersion);
        if (!gpuName.isEmpty() && !gpus.contains(info)) {
            gpus << info;
        }
    }
    
    if (!gpus.isEmpty()) {
        return gpus.join("；\n");
    }
#endif
    return QStringLiteral("未知显卡");
}

// 函数级注释：获取网络信息（使用 Windows API GetAdaptersAddresses，包括网关）
QString SystemInfoWidget::getNetworkInfo() const {
#ifdef Q_OS_WIN
    QStringList lines;
    
    // 动态加载 iphlpapi.dll 以避免链接依赖
    typedef ULONG (WINAPI *GetAdaptersAddressesPtr)(ULONG, ULONG, PVOID, PIP_ADAPTER_ADDRESSES, PULONG);
    HMODULE hLib = LoadLibraryW(L"iphlpapi.dll");
    if (!hLib) return QStringLiteral("无法加载网络库");

    GetAdaptersAddressesPtr pGetAdaptersAddresses = (GetAdaptersAddressesPtr)GetProcAddress(hLib, "GetAdaptersAddresses");
    if (!pGetAdaptersAddresses) {
        FreeLibrary(hLib);
        return QStringLiteral("无法获取网络函数");
    }

    ULONG outBufLen = 15000;
    PIP_ADAPTER_ADDRESSES pAddresses = (PIP_ADAPTER_ADDRESSES)malloc(outBufLen);
    ULONG flags = GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_INCLUDE_GATEWAYS | GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST;

    if (pGetAdaptersAddresses(AF_INET, flags, NULL, pAddresses, &outBufLen) == ERROR_BUFFER_OVERFLOW) {
        free(pAddresses);
        pAddresses = (PIP_ADAPTER_ADDRESSES)malloc(outBufLen);
    }

    if (pGetAdaptersAddresses(AF_INET, flags, NULL, pAddresses, &outBufLen) == NO_ERROR) {
        PIP_ADAPTER_ADDRESSES pCurrAddresses = pAddresses;
        while (pCurrAddresses) {
            if (pCurrAddresses->IfType != IF_TYPE_SOFTWARE_LOOPBACK && 
                pCurrAddresses->OperStatus == IfOperStatusUp) {
                
                QString name = QString::fromWCharArray(pCurrAddresses->FriendlyName);
                QStringList ips;
                QStringList gateways;
                
                // 获取IP地址
                PIP_ADAPTER_UNICAST_ADDRESS pUnicast = pCurrAddresses->FirstUnicastAddress;
                while (pUnicast) {
                    if (pUnicast->Address.lpSockaddr->sa_family == AF_INET) {
                        sockaddr_in* sa_in = (sockaddr_in*)pUnicast->Address.lpSockaddr;
                        const auto& addr = sa_in->sin_addr;
                        QString ipStr = QString("%1.%2.%3.%4")
                                        .arg(addr.S_un.S_un_b.s_b1)
                                        .arg(addr.S_un.S_un_b.s_b2)
                                        .arg(addr.S_un.S_un_b.s_b3)
                                        .arg(addr.S_un.S_un_b.s_b4);
                        
                        // 计算子网掩码
                        ULONG prefixLen = pUnicast->OnLinkPrefixLength;
                        uint32_t maskVal = 0;
                        if (prefixLen > 0 && prefixLen <= 32) {
                            maskVal = 0xFFFFFFFF << (32 - prefixLen);
                        }
                        QString maskStr = QString("%1.%2.%3.%4")
                                          .arg((maskVal >> 24) & 0xFF)
                                          .arg((maskVal >> 16) & 0xFF)
                                          .arg((maskVal >> 8) & 0xFF)
                                          .arg(maskVal & 0xFF);
                        
                        // New format: Subnet mask on new line
                        ips << QString("  IP: %1\n  掩码: %2").arg(ipStr, maskStr);
                    }
                    pUnicast = pUnicast->Next;
                }

                // 获取网关
                PIP_ADAPTER_GATEWAY_ADDRESS pGateway = pCurrAddresses->FirstGatewayAddress;
                while (pGateway) {
                    if (pGateway->Address.lpSockaddr->sa_family == AF_INET) {
                        sockaddr_in* sa_in = (sockaddr_in*)pGateway->Address.lpSockaddr;
                        const auto& addr = sa_in->sin_addr;
                        QString gwStr = QString("%1.%2.%3.%4")
                                        .arg(addr.S_un.S_un_b.s_b1)
                                        .arg(addr.S_un.S_un_b.s_b2)
                                        .arg(addr.S_un.S_un_b.s_b3)
                                        .arg(addr.S_un.S_un_b.s_b4);
                        gateways << gwStr;
                    }
                    pGateway = pGateway->Next;
                }

                if (!ips.isEmpty()) {
                    QString info = QString("%1:\n%2").arg(name, ips.join("\n"));
                    if (!gateways.isEmpty()) {
                        info += QString("\n  网关: %1").arg(gateways.join(", "));
                    }
                    lines << info;
                }
            }
            pCurrAddresses = pCurrAddresses->Next;
        }
    }

    if (pAddresses) free(pAddresses);
    FreeLibrary(hLib);

    return lines.isEmpty() ? QStringLiteral("无活跃网络连接") : lines.join("\n");
#endif
    return QStringLiteral("不支持的平台");
}

/**
 * 函数级注释：运行外部程序并抓取输出（stdout+stderr）
 */
static QString runAndCapture(const QString& program, const QStringList& args, int timeoutMs, QString* errorOut = nullptr)
{
    QProcess proc;
    proc.setProgram(program);
    proc.setArguments(args);
    proc.start();
    if (!proc.waitForStarted(timeoutMs)) {
        if (errorOut) {
            *errorOut = QStringLiteral("启动失败: %1").arg(program);
        }
        return QString();
    }
    if (!proc.waitForFinished(timeoutMs)) {
        proc.kill();
        proc.waitForFinished(1000);
        if (errorOut) {
            *errorOut = QStringLiteral("执行超时: %1").arg(program);
        }
        return QString();
    }
    const QString out = QString::fromLocal8Bit(proc.readAllStandardOutput());
    const QString err = QString::fromLocal8Bit(proc.readAllStandardError());
    return (out + err).trimmed();
}

/**
 * 函数级注释：打印 OpenCV 的 GPU 能力信息（CUDA/OpenCL/UMat），返回可展示的文本
 */
static QString PrintOpenCvGpuCapabilities()
{
    QStringList lines;
    lines << QStringLiteral("OpenCV 版本: %1").arg(QString::fromLatin1(CV_VERSION));

    const QString buildInfo = QString::fromLocal8Bit(cv::getBuildInformation().c_str());
    const bool buildCuda = buildInfo.contains(QStringLiteral("NVIDIA CUDA: YES"), Qt::CaseInsensitive);
    const bool buildOpenCL = buildInfo.contains(QStringLiteral("OpenCL: YES"), Qt::CaseInsensitive);
    lines << QStringLiteral("Build CUDA: %1").arg(buildCuda ? "YES" : "NO");
    lines << QStringLiteral("Build OpenCL: %1").arg(buildOpenCL ? "YES" : "NO");

    int cudaCount = 0;
    QString cudaError;
    try {
        cudaCount = cv::cuda::getCudaEnabledDeviceCount();
    } catch (const cv::Exception& e) {
        cudaError = QString::fromLocal8Bit(e.what());
    }
    if (cudaError.isEmpty()) {
        lines << QStringLiteral("CUDA 设备数: %1").arg(cudaCount);
    } else {
        lines << QStringLiteral("CUDA 探测异常: %1").arg(cudaError);
    }

    const bool haveOpenCL = cv::ocl::haveOpenCL();
    lines << QStringLiteral("OpenCL 可用(运行时): %1").arg(haveOpenCL ? "YES" : "NO");
    if (haveOpenCL) {
        cv::ocl::setUseOpenCL(true);
        lines << QStringLiteral("OpenCL 已启用: %1").arg(cv::ocl::useOpenCL() ? "YES" : "NO");

        cv::ocl::Context ctx;
        if (ctx.create(cv::ocl::Device::TYPE_ALL) && ctx.ndevices() > 0) {
            const auto dev = ctx.device(0);
            lines << QStringLiteral("OpenCL 设备: %1").arg(QString::fromLocal8Bit(dev.name().c_str()));
            lines << QStringLiteral("OpenCL 厂商: %1").arg(QString::fromLocal8Bit(dev.vendorName().c_str()));
        } else {
            lines << QStringLiteral("OpenCL 设备: 未能创建 Context");
        }
    }

    return lines.join("\n");
}

/**
 * 函数级注释：获取 OpenCV 的 GPU 能力信息（CUDA/OpenCL/UMat）
 */
QString SystemInfoWidget::getOpenCvInfo() const
{
    return PrintOpenCvGpuCapabilities();
}

/**
 * 函数级注释：获取系统可调用的 FFmpeg 可执行文件信息（路径与版本）
 */
QString SystemInfoWidget::getFfmpegInfo() const
{
    QStringList lines;

    QString ffmpegExePath;
    QString ffmpegVersionLine;

#ifdef Q_OS_WIN
    auto findLoadedFfmpegDllPath = []() -> QString {
        const wchar_t* candidates[] = {
            L"avcodec-61.dll",
            L"avformat-61.dll",
            L"avutil-59.dll",
            L"swscale-8.dll",
            L"swresample-5.dll",
            L"avcodec-60.dll",
            L"avformat-60.dll",
            L"avutil-58.dll",
        };

        for (const auto* name : candidates) {
            HMODULE mod = GetModuleHandleW(name);
            if (!mod) {
                continue;
            }
            wchar_t buf[MAX_PATH] = {0};
            const DWORD len = GetModuleFileNameW(mod, buf, MAX_PATH);
            if (len > 0) {
                return QString::fromWCharArray(buf, static_cast<int>(len));
            }
        }
        return QString();
    };

    const QString ffmpegDllPath = findLoadedFfmpegDllPath();
    if (!ffmpegDllPath.isEmpty()) {
        const QFileInfo dllInfo(ffmpegDllPath);
        const QString dllDir = dllInfo.absolutePath();
        const QString candidateExe = dllDir + QLatin1String("/ffmpeg.exe");
        if (QFileInfo::exists(candidateExe)) {
            ffmpegExePath = candidateExe;
        }
        if (ffmpegExePath.isEmpty()) {
            lines << QStringLiteral("FFmpeg DLL: %1").arg(ffmpegDllPath);
        }
    }
#endif

    if (ffmpegExePath.isEmpty()) {
        const QString appDir = QCoreApplication::applicationDirPath();
        const QStringList candidates = {
            appDir + QLatin1String("/ffmpeg.exe"),
            appDir + QLatin1String("/bin/ffmpeg.exe"),
            appDir + QLatin1String("/ffmpeg/bin/ffmpeg.exe"),
            appDir + QLatin1String("/../ffmpeg/bin/ffmpeg.exe"),
            appDir + QLatin1String("/../bin/ffmpeg.exe"),
        };
        for (const auto& c : candidates) {
            if (QFileInfo::exists(c)) {
                ffmpegExePath = QFileInfo(c).absoluteFilePath();
                break;
            }
        }
    }

    if (ffmpegExePath.isEmpty()) {
        lines << QStringLiteral("ffmpeg 路径: 未找到（未使用系统 PATH）");
        lines << QStringLiteral("ffmpeg 版本: 未知");
        return lines.join("\n");
    }

    lines << QStringLiteral("ffmpeg 路径: %1").arg(ffmpegExePath);

    QString err;
    const QString raw = runAndCapture(ffmpegExePath, {QStringLiteral("-version")}, 2000, &err);
    if (!raw.isEmpty()) {
        ffmpegVersionLine = raw.split('\n').value(0).trimmed();
    }

    if (!ffmpegVersionLine.isEmpty()) {
        lines << QStringLiteral("ffmpeg 版本: %1").arg(ffmpegVersionLine);
    } else if (!err.isEmpty()) {
        lines << QStringLiteral("ffmpeg 版本: 获取失败（%1）").arg(err);
    } else {
        lines << QStringLiteral("ffmpeg 版本: 未知");
    }

    return lines.join("\n");
}
