
#pragma once

#include "IPlcRegisterMapping.hpp"

namespace PlcModBus
{
class BeckhoffMapping final : public IPlcRegisterMapping
{
public:
    /**
     * @brief 获取 PLC 类型编号
     * @return 0
     * 函数级注释：约定 0 表示 Beckhoff。
     */
    int plcType() const override { return 0; }

    /**
     * @brief 获取默认起始地址
     * @return 12789
     * 函数级注释：Beckhoff 保持寄存器起始地址按需求固定为 12789。
     */
    quint16 defaultBaseAddress() const override { return static_cast<quint16>(12789); }

    /**
     * @brief 获取控制寄存器读取数量
     * @return 16
     * 函数级注释：Beckhoff 需要读取 12789~12804 共 16 个地址；每个地址映射两个开关（低8位/高8位），合计 32 路。
     */
    int controlRegisterCount() const override { return 16; }

    /**
     * @brief 获取状态寄存器起始地址
     * @return 13291
     * 函数级注释：Beckhoff 状态寄存器起始地址按需求固定为 13291。
     */
    quint16 statusRegisterBase() const override { return static_cast<quint16>(13291); }

    /**
     * @brief 获取状态寄存器读取数量
     * @return 2
     * 函数级注释：Beckhoff 状态寄存器读取 13291~13292 共 2 个地址，每个地址 16 位，共 32 位。
     */
    int statusRegisterCount() const override { return 2; }

    /**
     * @brief 将界面地址转换为 Modbus 协议地址
     * @param uiAddress 界面显示的地址
     * @return 协议地址
     * 函数级注释：Beckhoff 地址按 1 基地址处理，转换为 0 基地址需减 1。
     */
    quint16 toProtocolAddress(quint16 uiAddress) const override
    {
        return uiAddress > 0 ? static_cast<quint16>(uiAddress - 1) : static_cast<quint16>(0);
    }

    /**
     * @brief 获取索引对应的控制寄存器偏移
     * @param index 逻辑输出索引（0..31）
     * @return regOffset（0..15）
     * 函数级注释：每个寄存器地址映射两个开关，因此 index/2 即为寄存器偏移。
     */
    int controlRegisterOffsetForIndex(int index) const override { return index / 2; }

    /**
     * @brief 生成 32 路控制标签
     * @param base 起始地址（通常为 12789）
     * @return 标签列表
     * 函数级注释：标签以“地址 + 低8位/高8位”展示，便于现场对照。
     */
    QStringList makeZLabels(quint16 base) const override
    {
        QStringList labels;
        labels.reserve(32);
        for (int i = 0; i < 32; ++i) {
            const int addr = static_cast<int>(base) + (i / 2);
            const QString part = (i % 2 == 0) ? QStringLiteral("L") : QStringLiteral("H");
            labels << QString("%1 %2").arg(addr).arg(part);
        }
        return labels;
    }

    /**
     * @brief 生成 32 路状态标签
     * @param base 起始地址（通常为 13291）
     * @return 标签列表
     * 函数级注释：标签以“地址.bit”展示，例如 13291.0 表示 13291 寄存器的 bit0。
     */
    QStringList makeSLabels(quint16 base) const override
    {
        QStringList labels;
        labels.reserve(32);
        for (int i = 0; i < 32; ++i) {
            const int addr = static_cast<int>(base) + (i / 16);
            const int bit = (i % 16);
            labels << QString("%1.%2").arg(addr).arg(bit);
        }
        return labels;
    }

    /**
     * @brief 将 16 个控制寄存器拆为 32 路状态
     * @param regs 寄存器数组（期望至少 16 个）
     * @param outStates 输出状态（32 路）
     * @param outCount 输出长度
     * 函数级注释：每个地址对应两个开关：低8位用 bit0(0x0001)，高8位用 bit8(0x0100)。
     */
    void applyControlReadRegisters(const QVector<quint16>& regs, bool* outStates, int outCount) const override
    {
        const int n = qMin(regs.size(), 16);
        for (int r = 0; r < n; ++r) {
            const quint16 v = regs.at(r);
            const int i0 = r * 2;
            const int i1 = r * 2 + 1;

            const bool low = (v & 0x0001) != 0;
            const bool high = (v & 0x0100) != 0;

            if (i0 < outCount) outStates[i0] = low;
            if (i1 < outCount) outStates[i1] = high;
        }
    }

    /**
     * @brief 将 2 个状态寄存器拆为 32 路状态
     * @param regs 寄存器数组（期望至少 2 个）
     * @param outStates 输出状态（32 路）
     * @param outCount 输出长度
     * 函数级注释：S0..S15 对应 13291 的 bit0..15，S16..S31 对应 13292 的 bit0..15。
     */
    void applyStatusReadRegisters(const QVector<quint16>& regs, bool* outStates, int outCount) const override
    {
        const int n = qMin(regs.size(), 2);
        for (int r = 0; r < n; ++r) {
            const quint16 v = regs.at(r);
            for (int b = 0; b < 16; ++b) {
                const int i = r * 16 + b;
                if (i >= outCount) break;
                outStates[i] = ((v >> b) & 0x1) != 0;
            }
        }
    }

    /**
     * @brief 组装指定控制寄存器回写值
     * @param regOffset 寄存器偏移（0..15）
     * @param states 32 路状态
     * @param stateCount 状态长度
     * @return 16 位寄存器值
     * 函数级注释：低8位开关写 bit0，高8位开关写 bit8；其余位保持为 0（由该节点控制）。
     */
    quint16 buildControlRegisterValue(int regOffset, const bool* states, int stateCount) const override
    {
        const int i0 = regOffset * 2;
        const int i1 = regOffset * 2 + 1;

        quint16 v = 0;
        if (i0 >= 0 && i0 < stateCount && states[i0]) v |= 0x0001;
        if (i1 >= 0 && i1 < stateCount && states[i1]) v |= 0x0100;
        return v;
    }
};
} // namespace PlcModBus