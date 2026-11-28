/**
 * @file FaustDataModel.hpp
 * @brief FaustNode：加载用户 .dsp 文件 → libfaust JIT 编译 → 动态参数面板 + 音频端口。
 *
 * 功能（2026-11-20，2026-11-24 媒体库改造）：
 * - 从媒体库选 .dsp 文件（SelectorComboBox / Category::Faust）→ 读源码 → JIT 编译出 dsp 实例
 * - 遍历 dsp->buildUserInterface() → 动态生成节点内嵌参数面板
 * - 控件改值 → 直接写 *zone → DSP 参数实时生效
 * - 音频端口数由 dsp->getNumInputs()/getNumOutputs() 动态决定（单声道端口一一对应）
 * - FaustAudioWorker 独立线程追帧 → 一次 compute() 处理全部通道 → 各输出端口 pushFrame
 * - save/load：存媒体库文件名 + 内嵌源码 + 参数快照（.flow 自包含）
 *
 * 架构记忆：
 * - 素材统一走媒体库管理（2026-11-12 用户定案）：节点内只存文件名，
 *   加载时经 allFiles() 反查绝对路径（兜底 Documents/Flow/Medias），不存绝对路径。
 *   .dsp 由 MediaLibrary::Category::Faust 分类，媒体库面板可直接导入/拖放到画布。
 * - 中文路径：QString → QFile 读取（Qt 内部 UTF-16，不经 filesystem），
 *   源码以 UTF-8 std::string 喂 createDSPFactoryFromString，天然绕开 u8Path 坑。
 * - JIT 符号来自 faust.dll（LLVM 静态链入），链接 faust.lib import lib。
 * - 编译在 GUI 线程同步执行（加载是低频操作，几十~几百 ms 可接受）。
 * - 音频图只交换 ring queue 句柄，样本不经过 QtNodes 传播（项目既有惯例）。
 * - 重载必须先停 worker 线程再拆 dsp（避免 compute 与 delete 并发）。
 * - 失败路径：编译失败 qWarning + 面板显示错误，dsp 置空、端口归零（节点仍在图中）。
 */

#pragma once

#include "FaustParamUI.hpp"
#include "FaustAudioWorker.hpp"

#include "Common/AppConfig/ConstantDefines.h"
#include "Common/BaseClass/AbstractDelegateModel.h"
#include "NodeDataList.hpp"
#include "Elements/SelectorComboBox/SelectorComboBox.hpp"
#include "MediaLibrary/MediaLibrary.h"

#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QString>
#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtNodes/NodeDelegateModel>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtCore/QSignalBlocker>
#include <QtCore/QSize>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

#include "faust/dsp/dsp.h"
#include "faust/dsp/llvm-dsp.h"

#include <memory>
#include <string>
#include <vector>

using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

namespace Nodes
{

/** Faust 标准库目录（CMake 注入，含 stdfaust.lib） */
inline QString msFaustShareDir()
{
#ifdef FAUST_SHARE_DIR
    return QString::fromUtf8(FAUST_SHARE_DIR);
#else
    return QStringLiteral("C:/Program Files/Faust/share/faust");
#endif
}

class FaustDataModel final : public AbstractDelegateModel
{
    Q_OBJECT
public:
    FaustDataModel()
        : m_worker(new FaustAudioWorker())
    {
        InPortCount = 0;
        OutPortCount = 0;
        CaptionVisible = true;
        Caption = QStringLiteral("Faust DSP");
        WidgetEmbeddable = true;
        Resizable = true;
        PortEditable = false;

        buildUi();

        // 媒体库选择（SelectorComboBox）：候选项是 Category::Faust 下的 .dsp 文件名。
        // 模型→UI 写回一律用 QSignalBlocker，避免 selectionChanged 回环重编译。
        connect(m_fileSelector, &SelectorComboBox::selectionChanged, this,
                [this](const QString& entered) {
                    if (m_destructing) return;
                    const QString name = entered.trimmed();
                    if (name.isEmpty() || name == m_fileName) return;
                    const QString abs = resolveMediaFilePath(name);
                    if (abs.isEmpty()) {
                        setStatus(QCoreApplication::translate("Nodes::FaustDataModel", "媒体库中找不到: %1").arg(name), true);
                        qWarning() << "[FaustNode] media file not found:" << name;
                        return;
                    }
                    loadFile(abs);
                }, Qt::AutoConnection);
        connect(m_reloadButton, &QPushButton::clicked, this, [this]() {
            reload();
        }, Qt::AutoConnection);
        connect(m_paramUi, &FaustParamUI::parameterChanged, this,
                [this](int index, double value) {
                    if (m_destructing) return;
                    applyParam(index, value);
                }, Qt::AutoConnection);
    }

    ~FaustDataModel() override
    {
        m_destructing = true;
        // 【顺序铁律】先停工作线程，再拆 dsp（VST3 同款）
        if (m_worker) {
            m_worker->stopProcessing();
        }
        disconnectAll();
        teardownDsp();
        delete m_worker;
        m_worker = nullptr;
    }

    // ---- QtNodes::NodeDelegateModel 契约 ----
    QString caption() const override { return Caption; }
    bool captionVisible() const override { return CaptionVisible; }

    unsigned int nPorts(PortType portType) const override
    {
        return portType == PortType::In ? InPortCount : OutPortCount;
    }

    NodeDataType dataType(PortType portType, PortIndex) const override
    {
        Q_UNUSED(portType);
        return AudioData().type(); // id = "Audio"
    }

    QString portCaption(PortType portType, PortIndex portIndex) const override
    {
        switch (portType) {
        case PortType::In:
            return QStringLiteral("In %1").arg(portIndex + 1);
        case PortType::Out:
            return QStringLiteral("Out %1").arg(portIndex + 1);
        default:
            return {};
        }
    }

    std::shared_ptr<NodeData> outData(PortIndex port) override
    {
        if (port >= OutPortCount || !m_worker) {
            return std::make_shared<AudioData>();
        }
        auto buffer = m_worker->getOutputBuffer(static_cast<int>(port));
        auto data = std::make_shared<AudioData>();
        if (buffer) {
            data->setSharedAudioBuffer(buffer);
        }
        return data;
    }

    void setInData(std::shared_ptr<NodeData> data, PortIndex port) override
    {
        if (!m_worker || port >= InPortCount) {
            return;
        }
        auto audioData = std::dynamic_pointer_cast<AudioData>(data);
        std::shared_ptr<AudioTimestampRingQueue> buffer;
        if (audioData && audioData->isConnectedToSharedBuffer()) {
            buffer = audioData->getSharedAudioBuffer();
        }
        m_worker->setInputBuffer(static_cast<int>(port), buffer);
    }

    QWidget* embeddedWidget() override { return m_root; }

    // ---- 持久化：文件路径 + 内嵌源码 + 参数快照 ----
    QJsonObject save() const override
    {
        QJsonObject modelJson = NodeDelegateModel::save();
        QJsonObject faust;
        // 只存媒体库文件名（与 ISF/Gesture 等节点一致），不存绝对路径
        faust[QStringLiteral("file")] = m_fileName;
        faust[QStringLiteral("source")] = QString::fromUtf8(m_source);

        QJsonArray params;
        if (m_dsp && m_paramUi) {
            for (int i = 0; i < m_paramUi->paramCount(); ++i) {
                const FaustParamDesc& p = m_paramUi->params()[i];
                if (!p.zone || p.isPassive) continue;
                QJsonObject entry;
                entry[QStringLiteral("label")] = p.label;
                entry[QStringLiteral("value")] =
                    static_cast<double>(*p.zone);
                params.append(entry);
            }
        }
        faust[QStringLiteral("params")] = params;
        modelJson[QStringLiteral("faust")] = faust;
        return modelJson;
    }

    void load(const QJsonObject& data) override
    {
        if (m_destructing) return;
        const QJsonObject faust = data[QStringLiteral("faust")].toObject();
        const QString rawFile = faust.value(QStringLiteral("file")).toString().trimmed();
        // 兼容旧工程：旧版存的是绝对路径，这里只取文件名，路径统一由媒体库反查
        m_fileName = QFileInfo(rawFile).fileName();
        const QByteArray source = faust.value(QStringLiteral("source"))
                                      .toString().toUtf8();

        {
            QSignalBlocker blocker(m_fileSelector);
            m_fileSelector->setText(m_fileName);
            if (!m_fileName.isEmpty()) m_fileSelector->addIfNotExists(m_fileName);
        }
        const QString absPath = m_fileName.isEmpty()
                                    ? QString()
                                    : resolveMediaFilePath(m_fileName);
        if (!absPath.isEmpty()) m_filePath = absPath;

        if (!source.isEmpty()) {
            compileSource(source, m_fileName.isEmpty() ? QStringLiteral("(embedded)") : m_fileName);
        } else if (!absPath.isEmpty()) {
            loadFile(absPath);
        }

        // 参数快照灌回（按 label 匹配，zone 由新 dsp 实例提供）
        if (m_dsp && m_paramUi) {
            const QJsonArray params = faust.value(QStringLiteral("params")).toArray();
            for (const QJsonValue& v : params) {
                const QJsonObject entry = v.toObject();
                const QString label = entry.value(QStringLiteral("label")).toString();
                const double value = entry.value(QStringLiteral("value")).toDouble();
                const int idx = findParamByLabel(label);
                if (idx >= 0) {
                    applyParam(idx, value);
                    m_paramUi->setParameterValue(idx, value);
                }
            }
        }
    }

public slots:
    /** 选文件后调用：读源码 + JIT 编译 + 更新面板 */
    bool loadFile(const QString& path)
    {
        if (m_destructing || path.isEmpty()) return false;
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            setStatus(QCoreApplication::translate("Nodes::FaustDataModel", "无法打开: %1").arg(path), true);
            qWarning() << "FaustNode: cannot open" << path;
            return false;
        }
        const QByteArray source = f.readAll();
        f.close();

        m_filePath = path;
        const QFileInfo info(path);
        m_fileName = info.fileName();
        {
            QSignalBlocker blocker(m_fileSelector);
            m_fileSelector->setText(m_fileName);
            m_fileSelector->addIfNotExists(m_fileName);
            m_fileSelector->setToolTip(path);
        }
        return compileSource(source, m_fileName);
    }

    /** 用当前源码重新编译（Reload 按钮） */
    void reload()
    {
        if (m_destructing) return;
        if (!m_source.isEmpty()) {
            compileSource(m_source, m_fileName.isEmpty() ? m_filePath : m_fileName);
        } else if (!m_filePath.isEmpty()) {
            loadFile(m_filePath);
        } else if (!m_fileName.isEmpty()) {
            const QString abs = resolveMediaFilePath(m_fileName);
            if (!abs.isEmpty()) loadFile(abs);
        }
    }

private:
    /** 媒体库文件名（或旧工程里保存的绝对路径）→ 绝对路径，与 ISF/Gesture 惯例一致 */
    QString resolveMediaFilePath(const QString& value) const
    {
        const QString trimmed = value.trimmed();
        if (trimmed.isEmpty()) return {};

        const QFileInfo directInfo(trimmed);
        if (directInfo.isAbsolute() && directInfo.exists()) {
            return directInfo.absoluteFilePath(); // 兼容旧工程存的绝对路径 / 手输路径
        }

        const QStringList files = MediaLibrary::instance()->allFiles();
        for (const QString& p : files) {
            if (QFileInfo(p).fileName().compare(trimmed, Qt::CaseInsensitive) == 0) {
                return p;
            }
        }

        // 兜底：新文件刚入库但模型尚未刷新时，直接拼媒体库存储目录
        const QString inStorage =
            QDir(AppConstants::MEDIA_LIBRARY_STORAGE_DIR).absoluteFilePath(trimmed);
        if (QFileInfo::exists(inStorage)) return inStorage;

        if (directInfo.exists()) return directInfo.absoluteFilePath();
        return {};
    }

    void buildUi()
    {
        m_root = new QWidget();
        auto* layout = new QVBoxLayout(m_root);
        layout->setContentsMargins(4, 4, 4, 4);
        layout->setSpacing(4);

        auto* fileRow = new QWidget(m_root);
        auto* fileLayout = new QHBoxLayout(fileRow);
        fileLayout->setContentsMargins(0, 0, 0, 0);
        fileLayout->setSpacing(4);

        // 通用媒体选择控件：候选来自媒体库 Faust 分类，右侧 ⋯ 展开候选列表。
        // 素材导入统一在媒体库面板完成（本项目 2026-11-12 用户定案）。
        m_fileSelector = new SelectorComboBox(MediaLibrary::Category::Faust, fileRow);
        m_fileSelector->setPlaceholderText(QCoreApplication::translate("Nodes::FaustDataModel", "从媒体库选择 .dsp"));
        m_fileSelector->setToolTip(QCoreApplication::translate("Nodes::FaustDataModel", ".dsp 文件在媒体库 Faust DSP 分类中导入"));

        m_reloadButton = new QPushButton(fileRow);
        m_reloadButton->setIcon(QIcon(QStringLiteral(":/icons/icons/reload.png")));
        m_reloadButton->setIconSize(QSize(18, 18));
        m_reloadButton->setToolTip(QCoreApplication::translate("Nodes::FaustDataModel", "重新编译（重置参数）"));
        m_reloadButton->setMaximumWidth(28);

        fileLayout->addWidget(m_fileSelector, 1);
        fileLayout->addWidget(m_reloadButton);
        layout->addWidget(fileRow);

        m_statusLabel = new QLabel(m_root);
        m_statusLabel->setObjectName(QStringLiteral("faustStatus"));
        m_statusLabel->setWordWrap(true);
        layout->addWidget(m_statusLabel);

        m_paramUi = new FaustParamUI(m_root);
        layout->addWidget(m_paramUi, 1);
        setStatus(QCoreApplication::translate("Nodes::FaustDataModel", "载入 .dsp 文件以开始"), false);
    }

    void setStatus(const QString& text, bool isError)
    {
        if (!m_statusLabel) return;
        m_statusLabel->setText(text);
        m_statusLabel->setStyleSheet(
            isError ? QStringLiteral("color: #c62828;")
                    : QStringLiteral("color: gray;"));
    }

    bool compileSource(const QByteArray& source, const QString& displayPath)
    {
        // 1) 拆解旧实例（先停 UI 轮询 → 停音频线程 → 删 dsp → 删 factory）
        m_paramUi->clearAll();
        if (m_worker) {
            m_worker->stopProcessing();
        }
        teardownDsp();

        m_source = source;
        const std::string code(source.constData(),
                               static_cast<std::size_t>(source.size()));
        std::string errorMsg;
        // 【关键】JIT 编译必须传 "-I" + <share/faust> 两个参数，
        // 否则 import("stdfaust.lib") 失败（已用 Temp/faustnode/jit_probe.cpp 验证）
        const std::string includeDir = msFaustShareDir().toStdString();
        const char* argv[] = {"-I", includeDir.c_str()};
        // target 空串 = 当前机器；opt_level -1 = 最大优化
        m_factory = createDSPFactoryFromString(
            "FaustNode", code, 2, argv, "", errorMsg, -1);
        if (!m_factory) {
            const QString err = QString::fromStdString(errorMsg).trimmed();
            setStatus(QCoreApplication::translate("Nodes::FaustDataModel", "编译失败: %1")
                          .arg(err.isEmpty() ? QCoreApplication::translate("Nodes::FaustDataModel", "未知错误") : err),
                      true);
            qWarning() << "FaustNode: JIT compile failed:" << err;
            applyPortCount(PortType::In, 0, true);
            applyPortCount(PortType::Out, 0, true);
            Q_EMIT embeddedWidgetSizeUpdated();
            return false;
        }

        m_dsp = m_factory->createDSPInstance();
        if (!m_dsp) {
            setStatus(QCoreApplication::translate("Nodes::FaustDataModel", "编译成功但无法创建 DSP 实例"), true);
            qWarning() << "FaustNode: createDSPInstance failed";
            deleteDSPFactory(m_factory);
            m_factory = nullptr;
            applyPortCount(PortType::In, 0, true);
            applyPortCount(PortType::Out, 0, true);
            Q_EMIT embeddedWidgetSizeUpdated();
            return false;
        }

        // 项目音频总线惯例：固定 48kHz；块大小由全局时钟按帧率换算
        constexpr int kSampleRate = 48000;
        m_dsp->init(kSampleRate);

        // 2) 遍历 UI 回调 → 收集参数描述 → 生成控件
        m_dsp->buildUserInterface(m_paramUi);
        m_paramUi->rebuild();

        // 3) 端口数由 .dsp 的 process 签名决定（Faust 通道 ↔ 单声道端口一一对应）
        //    顺序铁律：先配 worker 缓冲区/实例，再通知图端口变化，
        //    保证端口通知触发的 outData()/setInData() 看到的是新缓冲区。
        const int inputCount = m_dsp->getNumInputs();
        const int outputCount = m_dsp->getNumOutputs();
        if (m_worker) {
            m_worker->configureChannels(inputCount, outputCount);
            m_worker->setDsp(m_dsp, kSampleRate);
        }
        applyPortCount(PortType::In, static_cast<unsigned int>(inputCount), true);
        applyPortCount(PortType::Out, static_cast<unsigned int>(outputCount), true);
        for (unsigned int i = 0; i < OutPortCount; ++i) {
            Q_EMIT dataUpdated(static_cast<PortIndex>(i));
        }
        if (m_worker) {
            m_worker->startProcessing();
        }

        const int params = m_paramUi->paramCount();
        setStatus(QCoreApplication::translate("Nodes::FaustDataModel", "已加载 %1 · %2 参数 · %3 入 %4 出")
                      .arg(displayPath).arg(params).arg(inputCount).arg(outputCount), false);
        qDebug() << "FaustNode: loaded" << displayPath
                 << "inputs=" << inputCount
                 << "outputs=" << outputCount
                 << "params=" << params;
        Q_EMIT embeddedWidgetSizeUpdated();
        return true;
    }

    void teardownDsp()
    {
        // 防御：worker 线程已停，但清掉其缓存的 dsp 指针，避免误用悬空指针
        if (m_worker) {
            m_worker->setDsp(nullptr, 48000);
        }
        delete m_dsp;              // llvm_dsp 虚析构
        m_dsp = nullptr;
        if (m_factory) {
            deleteDSPFactory(m_factory);
            m_factory = nullptr;
        }
    }

    /** 动态端口数变更（照 AudioGate 惯例：先通知删除/插入，再改计数） */
    void applyPortCount(PortType portType, unsigned int newCount, bool notifyPorts)
    {
        unsigned int& count = (portType == PortType::In) ? InPortCount : OutPortCount;
        const unsigned int oldCount = count;
        if (newCount == oldCount) {
            return;
        }
        if (notifyPorts) {
            if (newCount > oldCount) {
                Q_EMIT portsAboutToBeInserted(portType, oldCount, newCount - 1);
                count = newCount;
                Q_EMIT portsInserted();
            } else {
                Q_EMIT portsAboutToBeDeleted(portType, newCount, oldCount - 1);
                count = newCount;
                Q_EMIT portsDeleted();
            }
        } else {
            count = newCount;
        }
    }

    void disconnectAll()
    {
        if (m_fileSelector) {
            m_fileSelector->disconnect(this);
            this->disconnect(m_fileSelector);
        }
        if (m_root) {
            m_root->disconnect(this);
            this->disconnect(m_root);
        }
        if (m_paramUi) {
            m_paramUi->disconnect(this);
            this->disconnect(m_paramUi);
        }
    }

    int findParamByLabel(const QString& label) const
    {
        if (!m_paramUi) return -1;
        const auto& params = m_paramUi->params();
        for (int i = 0; i < params.size(); ++i) {
            if (params[i].label == label && params[i].zone && !params[i].isPassive)
                return i;
        }
        return -1;
    }

    void applyParam(int index, double value)
    {
        if (!m_dsp || !m_paramUi) return;
        if (index < 0 || index >= m_paramUi->paramCount()) return;
        FAUSTFLOAT* zone = m_paramUi->params()[index].zone;
        if (!zone) return;
        *zone = static_cast<FAUSTFLOAT>(value);
    }

    // ---- 成员 ----
    FaustAudioWorker* m_worker = nullptr;

    QWidget* m_root = nullptr;
    SelectorComboBox* m_fileSelector = nullptr;
    QPushButton* m_reloadButton = nullptr;
    QLabel* m_statusLabel = nullptr;
    FaustParamUI* m_paramUi = nullptr;

    llvm_dsp_factory* m_factory = nullptr;
    llvm_dsp* m_dsp = nullptr;

    QString m_filePath;   // 运行时解析出的绝对路径（不持久化）
    QString m_fileName;   // 持久化用：媒体库文件名
    QByteArray m_source;
    bool m_destructing = false;
};

} // namespace Nodes
