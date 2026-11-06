/**
 * @file GDTFDataModel.hpp
 * @brief GDTF 灯具文件解析 + 通道自动映射节点
 *
 * 从媒体库（Fixture 分类）选择 .gdtf 文件并选择 DMX 模式后，
 * 按起始地址自动完成通道映射（patch）：
 *  - 每个 DMXChannel 的 offsets（相对断点偏移，高位在前）解析为绝对 DMX 地址
 *  - 多断点（DMXBreak）按前断点占用顺序接续分配
 *  - 输出 addressMap（绝对地址→当前值，可直接接 DMXUniverse/Artnet）
 *
 * 输入端口（VariableData 向量，1 个）：
 *  - 连接后按通道序覆盖输出：向量第 i 个元素 → 起始地址第 i 通道（0-255）
 *  - 向量短于占用通道数（footprint）时尾部补 0，超出部分自动截断
 *  - 断开连接后回退到滑块手动值 / GDTF 默认值
 *
 * 输出 VariableData：
 *  - "default"      : 完整灯具描述 map，内含 addressMap / channelTable /
 *                     startAddress / footprint / mode
 *  - "channels"     : 当前模式通道数组（每项含解析后的 address/addresses/width/default8bit）
 *  - "attributeMap" : 属性名 → {address, width, addresses}
 *  - "meta"         : 厂商、名称、GUID 等元信息
 *
 * 外部控制地址：
 *  /file         .gdtf 文件（媒体库相对路径）
 *  /mode         DMX 模式名称
 *  /startAddress 映射起始地址（1-512）
 *  /ch/{行号}    按映射行序号设置通道输出值（0 起，如 /ch/0=255）；
 *                行号与节点界面"通道控制"区一一对应，拖动滑块同样生效
 */
#pragma once

#include "GDTFInterface.hpp"
#include "GDTFParser.hpp"

#include "NodeDataList.hpp"
#include "Common/AppConfig/ConfigManager.h"
#include "Common/BaseClass/AbstractDelegateModel.h"
#include "StatusContainer/GlobalEventBus.hpp"

#include <QtCore/QSignalBlocker>
#include <QtGui/QPixmap>
#include <QtNodes/NodeDelegateModel>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QSpinBox>

#include <memory>

struct GlobalEvent;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

namespace Nodes
{

class GDTFDataModel : public AbstractDelegateModel
{
    Q_OBJECT
    Q_PROPERTY(QString file READ file WRITE setFile NOTIFY fileChanged)
    Q_PROPERTY(QString dmxMode READ dmxMode WRITE setDmxMode NOTIFY dmxModeChanged)
    Q_PROPERTY(int startAddress READ startAddress WRITE setStartAddress NOTIFY startAddressChanged)

public:
    GDTFDataModel()
    {
        InPortCount = 1;    // 0 号口：通道值向量（按占用通道数裁切/补零后覆盖输出）
        OutPortCount = 1;
        CaptionVisible = true;
        Caption = QStringLiteral("GDTF Fixture");
        WidgetEmbeddable = false;
        Resizable = false;
        PortEditable = false;

        m_outData = std::make_shared<VariableData>();

        {
            NodeDelegateModel::ExternalBinding b;
            b.member = "file";
            b.control = widget->fileSelectComboBox;
            AbstractDelegateModel::registerExternalBinding("/file", this, b);
        }
        {
            NodeDelegateModel::ExternalBinding b;
            b.member = "dmxMode";
            b.control = widget->modeCombo;
            AbstractDelegateModel::registerExternalBinding("/mode", this, b);
        }
        {
            NodeDelegateModel::ExternalBinding b;
            b.member = "startAddress";
            b.control = widget->startAddressSpin;
            AbstractDelegateModel::registerExternalBinding("/startAddress", this, b);
        }

        connect(widget->fileSelectComboBox, &SelectorComboBox::textChanged,
                this, &GDTFDataModel::setFile);
        connect(this, &GDTFDataModel::fileChanged, this, [this](const QString &) {
            reloadFile();
        });

        connect(widget->startAddressSpin, QOverload<int>::of(&QSpinBox::valueChanged),
                this, &GDTFDataModel::setStartAddress);

        connect(widget->modeCombo, &QComboBox::currentTextChanged,
                this, &GDTFDataModel::setDmxMode);
    }

    ~GDTFDataModel() override = default;

    QString file() const { return m_file; }
    QString dmxMode() const { return m_dmxMode; }
    int startAddress() const { return m_startAddress; }

public slots:
    void setFile(const QString &fileName)
    {
        const QString trimmed = fileName.trimmed();
        if (trimmed == m_file)
            return;
        m_file = trimmed;
        if (widget) {
            const QSignalBlocker blocker(widget->fileSelectComboBox);
            widget->fileSelectComboBox->setText(m_file);
        }
        Q_EMIT fileChanged(m_file);
        AbstractDelegateModel::stateFeedBack(QStringLiteral("/file"), m_file);
    }

    void setDmxMode(const QString &mode)
    {
        if (mode == m_dmxMode)
            return;
        m_dmxMode = mode;
        m_userValues.clear();   // 模式变更：地址含义改变，用户值作废
        if (widget) {
            const QSignalBlocker blocker(widget->modeCombo);
            const int idx = widget->modeCombo->findText(mode);
            if (idx >= 0)
                widget->modeCombo->setCurrentIndex(idx);
        }
        rebuildOutput();
        Q_EMIT dmxModeChanged(m_dmxMode);
        AbstractDelegateModel::stateFeedBack(QStringLiteral("/mode"), m_dmxMode);
    }

    void setStartAddress(int addr)
    {
        const int clamped = qBound(1, addr, 512);
        if (clamped == m_startAddress)
            return;
        // 用户已调值随地址整体平移（保持通道值的语义）
        const int diff = clamped - m_startAddress;
        m_startAddress = clamped;
        if (diff != 0 && !m_userValues.isEmpty()) {
            QMap<int, int> shifted;
            for (auto it = m_userValues.constBegin(); it != m_userValues.constEnd(); ++it)
                shifted.insert(it.key() + diff, it.value());
            m_userValues = shifted;
        }
        if (widget) {
            const QSignalBlocker blocker(widget->startAddressSpin);
            widget->startAddressSpin->setValue(m_startAddress);
        }
        rebuildOutput();
        Q_EMIT startAddressChanged(m_startAddress);
        AbstractDelegateModel::stateFeedBack(QStringLiteral("/startAddress"), m_startAddress);
    }

    Q_SIGNALS:
        void fileChanged(const QString &file);
    void dmxModeChanged(const QString &mode);
    void startAddressChanged(int address);

protected:
    void afterModelReady() override
    {
        GlobalEventBus::instance()->subscribe(
            makeFullOscAddress("/file"), this, SLOT(onGlobalEvent(GlobalEvent)));
        GlobalEventBus::instance()->subscribe(
            makeFullOscAddress("/mode"), this, SLOT(onGlobalEvent(GlobalEvent)));
        GlobalEventBus::instance()->subscribe(
            makeFullOscAddress("/startAddress"), this, SLOT(onGlobalEvent(GlobalEvent)));
        m_subscriptionsReady = true;
        // 补订当前映射行（rebuildOutput 可能发生在本回调之前）
        updateValueSubscriptions(0, m_rowAddresses.size());
    }

private slots:
    void onGlobalEvent(const GlobalEvent &ev)
    {
        if (ev.kind != GlobalEventKind::Command)
            return;
        if (ev.address == makeFullOscAddress("/file"))
            setFile(ev.payload.toString());
        else if (ev.address == makeFullOscAddress("/mode"))
            setDmxMode(ev.payload.toString());
        else if (ev.address == makeFullOscAddress("/startAddress"))
            setStartAddress(ev.payload.toInt());
        else if (ev.address.startsWith(makeFullOscAddress("/ch/"))) {
            // /ch/{行号}：按映射行设置通道输出值
            const QString suffix = ev.address.mid(makeFullOscAddress("/ch/").size());
            bool ok = false;
            const int row = suffix.toInt(&ok);
            if (ok)
                setChannelValue(row, ev.payload.toInt());
        }
    }

public:
    NodeDataType dataType(PortType, PortIndex) const override
    {
        return VariableData().type();
    }

    std::shared_ptr<QtNodes::NodeData> outData(PortIndex) override
    {
        return m_outData;
    }

    void setInData(std::shared_ptr<QtNodes::NodeData> data, PortIndex) override
    {
        if (!data) {
            // 断开连接：清空输入向量，回退到手动值 / GDTF 默认值
            if (!m_inputConnected)
                return;
            m_inputConnected = false;
            m_inputVector.clear();
            rebuildOutput();
            return;
        }
        auto variableData = std::dynamic_pointer_cast<VariableData>(data);
        if (!variableData)
            return;
        m_inputConnected = true;
        // 存自然长度；应用时再按 footprint 裁切/补零
        m_inputVector = variableData->asFloats();
        rebuildOutput();
    }

    QWidget *embeddedWidget() override { return widget; }

    QJsonObject save() const override
    {
        QJsonObject json = NodeDelegateModel::save();
        json["file"] = m_file;
        json["dmxMode"] = m_dmxMode;
        json["startAddress"] = m_startAddress;
        QJsonObject vals;
        for (auto it = m_userValues.constBegin(); it != m_userValues.constEnd(); ++it)
            vals[QString::number(it.key())] = it.value();
        json["userValues"] = vals;
        return json;
    }

    void load(const QJsonObject &json) override
    {
        NodeDelegateModel::load(json);
        const int addr = json["startAddress"].toInt(1);
        if (addr != 1)
            setStartAddress(addr);
        const QString f = json["file"].toString();
        if (!f.isEmpty())
            setFile(f);
        const QString m = json["dmxMode"].toString();
        if (!m.isEmpty())
            setDmxMode(m);

        // 恢复用户已调的通道值（在 file/mode 重建完成后应用）
        m_userValues.clear();
        const QJsonObject uv = json["userValues"].toObject();
        for (auto it = uv.constBegin(); it != uv.constEnd(); ++it)
            m_userValues.insert(it.key().toInt(), it.value().toInt());
        if (!uv.isEmpty())
            rebuildOutput();
    }

private:
    /** 媒体库相对路径 → 绝对路径（兼容传入绝对路径） */
    QString resolvePath(const QString &name) const
    {
        const QFileInfo info(name);
        if (info.isAbsolute() && info.exists())
            return info.absoluteFilePath();
        return AppConstants::MEDIA_LIBRARY_STORAGE_DIR + "/" + name;
    }

    /** 重新读取并解析当前选择的 .gdtf 文件 */
    void reloadFile()
    {
        m_fixture = QVariantMap();
        m_userValues.clear();   // 文件变更：地址布局改变，用户值作废
        updateThumbnail();

        if (m_file.isEmpty()) {
            widget->modeCombo->clear();
            widget->updateRange(m_startAddress, 0);
            updateNodeState(QtNodes::NodeValidationState::State::Warning, QStringLiteral("no file"));
            rebuildOutput();
            return;
        }

        const QString path = resolvePath(m_file);
        GDTFParser parser;
        if (!parser.parseFile(path)) {
            m_fixture = QVariantMap();
            updateNodeState(QtNodes::NodeValidationState::State::Error, parser.lastError());
            qWarning() << "GDTFDataModel:" << parser.lastError();
            rebuildOutput();
            return;
        }

        m_fixture = parser.result();
        updateNodeState(QtNodes::NodeValidationState::State::Valid, QString());

        // 填充模式下拉框；优先恢复已保存的模式
        QStringList modes;
        const QVariantList modeList = m_fixture.value(QStringLiteral("dmxModes")).toList();
        for (const QVariant &v : modeList)
            modes << v.toMap().value(QStringLiteral("name")).toString();
        {
            const QSignalBlocker blocker(widget->modeCombo);
            widget->modeCombo->clear();
            widget->modeCombo->addItems(modes);
            if (!m_dmxMode.isEmpty() && modes.contains(m_dmxMode))
                widget->modeCombo->setCurrentText(m_dmxMode);
            else
                m_dmxMode = widget->modeCombo->currentText();
        }

        updateThumbnail(path);
        rebuildOutput();
    }

    /** 通道默认值（归一化）：通道级 Default → 首个 ChannelFunction 的 Default → 0 */
    static double channelDefaultNorm(const QVariantMap &ch)
    {
        const QVariantMap d = ch.value(QStringLiteral("default")).toMap();
        if (d.value(QStringLiteral("bits")).toInt() >= 0)
            return d.value(QStringLiteral("normalized")).toDouble();
        const QVariantList logicals = ch.value(QStringLiteral("logicalChannels")).toList();
        if (!logicals.isEmpty()) {
            const QVariantList fns = logicals.first().toMap()
                                         .value(QStringLiteral("functions")).toList();
            if (!fns.isEmpty()) {
                const QVariantMap fd = fns.first().toMap()
                                           .value(QStringLiteral("default")).toMap();
                if (fd.value(QStringLiteral("bits")).toInt() >= 0)
                    return fd.value(QStringLiteral("normalized")).toDouble();
            }
        }
        return 0.0;
    }

    /**
     * @brief 按当前模式 + 起始地址自动映射通道
     *
     * 规则：
     *  - Break 1 基址 = startAddress；后续断点按前序断点占用顺序接续
     *    （GDTF 的多断点通常对应多 Universe，此处按顺序接续成一段连续地址，
     *      满足单 Universe 灯具的绝大多数场景）
     *  - 每断点内以最小 offset 归零：绝对地址 = 断点基址 + (offset - minOffset)。
     *    规范未明确 offset 起点，实际文件 0-based（Offset="0,1"）与 1-based
     *    （Offset="1"）并存，归一化后两者都正确落到基址
     *  - 16bit 通道：offsets[0]=MSB（写入默认值），offsets[1]=LSB（默认 0）
     *  - offsets 为空（"None"）的通道视为未分配地址
     *
     * @param rawChannels 解析出的原始通道数组
     * @param addressMap  [out] 绝对地址(字符串) → 8bit 默认值
     * @param attributeMap[out] 属性 → {address, width, addresses}
     * @return 映射后的通道数组（每项追加 address/addresses/width/default8bit）
     */
    QVariantList buildChannelTable(const QVariantList &rawChannels,
                                   QVariantMap &addressMap,
                                   QVariantMap &attributeMap) const
    {
        // 1) 每断点占用 + 最小偏移（用于归一化 0/1-based 混用）
        QMap<int, int> breakFootprint;
        QMap<int, int> breakMinOffset;
        for (const QVariant &cv : rawChannels) {
            const QVariantMap c = cv.toMap();
            const int b = qMax(1, c.value(QStringLiteral("dmxBreak")).toInt());
            const QVariantList offsets = c.value(QStringLiteral("offsets")).toList();
            for (const QVariant &o : offsets) {
                const int off = o.toInt();
                breakFootprint[b] = qMax(breakFootprint.value(b, 0), off + 1);
                if (!breakMinOffset.contains(b) || off < breakMinOffset.value(b))
                    breakMinOffset.insert(b, off);
            }
        }
        // 归一化后每断点占用 = 最大 offset - 最小 offset + 1
        for (auto it = breakFootprint.begin(); it != breakFootprint.end(); ++it)
            it.value() -= breakMinOffset.value(it.key(), 0);

        // 2) 断点基址顺序接续
        QMap<int, int> breakBase;
        int cursor = m_startAddress;
        const int maxBreak = breakFootprint.isEmpty() ? 0 : breakFootprint.lastKey();
        for (int b = 1; b <= maxBreak; ++b) {
            breakBase.insert(b, cursor);
            cursor += breakFootprint.value(b, 0);
        }

        // 3) 逐通道解析绝对地址
        QVariantList table;
        for (const QVariant &cv : rawChannels) {
            QVariantMap c = cv.toMap();
            const int b = qMax(1, c.value(QStringLiteral("dmxBreak")).toInt());
            const int base = breakBase.value(b, m_startAddress);
            const int minOff = breakMinOffset.value(b, 0);

            QVariantList addresses;
            for (const QVariant &o : c.value(QStringLiteral("offsets")).toList())
                addresses.append(base + o.toInt() - minOff);

            const int width = addresses.size();
            const int def8 = qRound(channelDefaultNorm(c) * 255.0);

            if (!addresses.isEmpty()) {
                c.insert(QStringLiteral("address"), addresses.first());   // MSB/主地址
                c.insert(QStringLiteral("addresses"), addresses);
                c.insert(QStringLiteral("width"), width);
                // addressMap：MSB 带默认值，LSB 恒 0（DMXUniverse 直接可消费）
                addressMap.insert(QString::number(addresses.first().toInt()), def8);
                for (int i = 1; i < addresses.size(); ++i)
                    addressMap.insert(QString::number(addresses[i].toInt()), 0);
                // 属性映射（同名属性取首地址，后缀通道 _2 等按 GDTF 命名区分）
                const QString attr = c.value(QStringLiteral("attribute")).toString();
                if (!attributeMap.contains(attr))
                    attributeMap.insert(attr, QVariantMap{
                        { QStringLiteral("address"),  addresses.first() },
                        { QStringLiteral("width"),    width },
                        { QStringLiteral("addresses"), addresses },
                        { QStringLiteral("dmxBreak"), b }
                    });
            } else {
                c.insert(QStringLiteral("address"), -1);
                c.insert(QStringLiteral("addresses"), QVariantList());
                c.insert(QStringLiteral("width"), 0);
            }
            c.insert(QStringLiteral("default8bit"), def8);
            c.insert(QStringLiteral("defaultNorm"), channelDefaultNorm(c));
            table.append(c);
        }
        return table;
    }

    /** 按当前模式重建输出数据与界面映射表 */
    void rebuildOutput()
    {
        m_outData = std::make_shared<VariableData>();

        // 当前模式的原始通道数组
        QVariantList rawChannels;
        QVariantMap out = m_fixture;
        if (!m_fixture.isEmpty()) {
            const QVariantList modeList = m_fixture.value(QStringLiteral("dmxModes")).toList();
            for (const QVariant &v : modeList) {
                const QVariantMap mode = v.toMap();
                if (mode.value(QStringLiteral("name")).toString() == m_dmxMode) {
                    out.insert(QStringLiteral("mode"), mode.value(QStringLiteral("name")));
                    rawChannels = mode.value(QStringLiteral("channels")).toList();
                    out.insert(QStringLiteral("relations"), mode.value(QStringLiteral("relations")));
                    break;
                }
            }
        }

        // 自动映射
        QVariantMap addressMap;
        QVariantMap attributeMap;
        const QVariantList table = buildChannelTable(rawChannels, addressMap, attributeMap);

        // 用户已调值覆盖 addressMap 默认值（输出当前值而非默认值）
        for (auto it = m_userValues.constBegin(); it != m_userValues.constEnd(); ++it) {
            const QString key = QString::number(it.key());
            if (addressMap.contains(key))
                addressMap.insert(key, it.value());
        }

        int footprint = 0;
        for (const QVariant &v : table) {
            const QVariantList addrs = v.toMap().value(QStringLiteral("addresses")).toList();
            if (!addrs.isEmpty())
                footprint = qMax(footprint, addrs.last().toInt() - m_startAddress + 1);
        }

        // 输入向量优先级最高（覆盖手动值与默认值）：
        // 第 i 个元素 → 起始地址第 i 通道；短于 footprint 尾部补 0，超出截断
        if (m_inputConnected) {
            for (int i = 0; i < footprint; ++i) {
                const int v = (i < m_inputVector.size())
                    ? qBound(0, qRound(m_inputVector[i]), 255)
                    : 0;
                addressMap.insert(QString::number(m_startAddress + i), v);
            }
        }

        out.insert(QStringLiteral("startAddress"), m_startAddress);
        out.insert(QStringLiteral("footprint"), footprint);
        out.insert(QStringLiteral("addressMap"), addressMap);
        out.insert(QStringLiteral("channelTable"), table);

        QVariantMap meta;
        meta.insert(QStringLiteral("manufacturer"),  m_fixture.value(QStringLiteral("manufacturer")));
        meta.insert(QStringLiteral("name"),          m_fixture.value(QStringLiteral("name")));
        meta.insert(QStringLiteral("longName"),      m_fixture.value(QStringLiteral("longName")));
        meta.insert(QStringLiteral("shortName"),     m_fixture.value(QStringLiteral("shortName")));
        meta.insert(QStringLiteral("fixtureTypeId"), m_fixture.value(QStringLiteral("fixtureTypeId")));
        meta.insert(QStringLiteral("dataVersion"),   m_fixture.value(QStringLiteral("dataVersion")));

        m_outData->insert(QStringLiteral("default"), out);
        m_outData->insert(QStringLiteral("channels"), table);
        m_outData->insert(QStringLiteral("attributeMap"), attributeMap);
        m_outData->insert(QStringLiteral("meta"), meta);
        m_outData->insert(QStringLiteral("mode"), m_dmxMode);

        // UI 同步
        if (widget) {
            widget->updateRange(m_startAddress, footprint);
            syncValueControls(table);
        }

        Q_EMIT dataUpdated(0);
    }

    /**
     * 按映射表重建通道值控制行（DMXDevice 节点同款 IntDragValueWidget）。
     * 每个有地址的物理通道一行；16bit 拆 MSB/LSB 两行。初值 = 用户已设值，否则通道默认值。
     */
    void syncValueControls(const QVariantList &table)
    {
        QStringList labels;
        QVector<int> addresses;
        QVector<int> defaults;
        for (const QVariant &v : table) {
            const QVariantMap c = v.toMap();
            const QVariantList addrs = c.value(QStringLiteral("addresses")).toList();
            if (addrs.isEmpty())
                continue;   // 未分配（Offset="None"）通道不生成控件
            const QString attr = c.value(QStringLiteral("attribute")).toString();
            const int def8 = c.value(QStringLiteral("default8bit")).toInt();
            for (int i = 0; i < addrs.size(); ++i) {
                labels << (addrs.size() > 1
                    ? QStringLiteral("%1@%2 %3").arg(attr).arg(addrs[i].toInt())
                        .arg(i == 0 ? QStringLiteral("MSB") : QStringLiteral("LSB"))
                    : QStringLiteral("%1@%2").arg(attr).arg(addrs[i].toInt()));
                addresses << addrs[i].toInt();
                defaults << (i == 0 ? def8 : 0);
            }
        }

        const int oldRows = m_rowAddresses.size();
        m_rowAddresses = addresses;
        if (widget)
            widget->rebuildValueControls(labels, addresses, defaults);

        // 恢复初值 + 连接滑块信号（控件为新建，无旧连接残留）
        // 输入连接时输入值优先显示（footprint 内未覆盖的通道补 0）
        for (int row = 0; row < m_rowAddresses.size(); ++row) {
            IntDragValueWidget *sl = widget ? widget->sliderAt(row) : nullptr;
            if (!sl)
                continue;
            int initVal;
            if (m_inputConnected) {
                const int offset = m_rowAddresses[row] - m_startAddress;
                initVal = (offset >= 0 && offset < m_inputVector.size())
                    ? qBound(0, qRound(m_inputVector[offset]), 255)
                    : 0;
            } else {
                initVal = m_userValues.contains(m_rowAddresses[row])
                              ? m_userValues.value(m_rowAddresses[row])
                              : defaults[row];
            }
            {
                const QSignalBlocker blocker(sl);
                sl->setValue(qBound(0, initVal, 255));
            }
            const int rowIdx = row;
            connect(sl, &IntDragValueWidget::valueChanged, this,
                    [this, rowIdx](int value) { setChannelValue(rowIdx, value); });
        }

        if (m_subscriptionsReady)
            updateValueSubscriptions(oldRows, m_rowAddresses.size());
    }

    /** 设置某地址行输出值（滑块拖动 / OSC /ch/N），轻量更新 addressMap */
    void setChannelValue(int row, int value)
    {
        if (row < 0 || row >= m_rowAddresses.size())
            return;
        const int addr = m_rowAddresses[row];
        const int v = qBound(0, value, 255);
        if (m_userValues.value(addr, -1) == v)
            return;
        m_userValues.insert(addr, v);

        if (widget) {
            if (IntDragValueWidget *sl = widget->sliderAt(row)) {
                const QSignalBlocker blocker(sl);
                sl->setValue(v);
            }
        }

        // 只改 default.addressMap，不整体重建（避免拖动时 UI 闪烁）
        QVariantMap out = m_outData->value(QStringLiteral("default")).toMap();
        QVariantMap am = out.value(QStringLiteral("addressMap")).toMap();
        am.insert(QString::number(addr), v);
        out.insert(QStringLiteral("addressMap"), am);
        m_outData->insert(QStringLiteral("default"), out);
        Q_EMIT dataUpdated(0);

        AbstractDelegateModel::stateFeedBack(QStringLiteral("/ch/%1").arg(addr), v);
    }

    /** /ch/N 行订阅随映射行数增减 */
    void updateValueSubscriptions(int oldRows, int newRows)
    {
        for (int i = oldRows; i < newRows; ++i)
            GlobalEventBus::instance()->subscribe(
                makeFullOscAddress(QStringLiteral("/ch/%1").arg(i)),
                this, SLOT(onGlobalEvent(GlobalEvent)));
        for (int i = newRows; i < oldRows; ++i)
            GlobalEventBus::instance()->unsubscribe(
                makeFullOscAddress(QStringLiteral("/ch/%1").arg(i)), this);
    }

    void updateThumbnail(const QString &path = QString())
    {
        widget->thumbnailLabel->setPixmap(QPixmap());
        if (path.isEmpty()) {
            widget->thumbnailLabel->setText(QStringLiteral("No Thumbnail"));
            return;
        }
        const QByteArray png = GDTFParser::readThumbnail(path);
        if (png.isEmpty()) {
            widget->thumbnailLabel->setText(QStringLiteral("No Thumbnail"));
            return;
        }
        QPixmap pix;
        if (pix.loadFromData(png, "PNG")) {
            widget->thumbnailLabel->setPixmap(
                pix.scaled(232, 176, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
    }

private:
    GDTFInterface *widget = new GDTFInterface();
    std::shared_ptr<VariableData> m_outData;
    QVariantMap m_fixture;     ///< 解析后的完整灯具数据
    QString m_file;            ///< 媒体库相对路径
    QString m_dmxMode;         ///< 当前 DMX 模式名
    int m_startAddress = 1;    ///< 映射起始地址（DMX Break 1）
    QVector<int> m_rowAddresses;        ///< 值控制行 → 绝对 DMX 地址（与滑块平行）
    QMap<int, int> m_userValues;        ///< 用户已调值（绝对地址 → 0-255），优先于默认值
    QVector<float> m_inputVector;       ///< 输入端口通道值向量（自然长度，应用时裁切/补零）
    bool m_inputConnected = false;      ///< 输入端口是否有连接（空向量 + 连接 = 全部补零）
    bool m_subscriptionsReady = false;  ///< afterModelReady 后才可订阅 /ch/N
};

} // namespace Nodes
