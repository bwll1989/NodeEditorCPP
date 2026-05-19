
#pragma once

#include "IPlcRegisterMapping.hpp"

namespace PlcModBus
{
class SiemensMapping final : public IPlcRegisterMapping
{
public:
    /**
     * @brief 获取 PLC 类型编号
     * @return 1
     * 函数级注释：约定 1 表示 Siemens。
     */
    int plcType() const override { return 1; }

    /**
     * @brief 获取默认起始地址
     * @return 40001
     * 函数级注释：Siemens 保持寄存器起始地址按需求固定为 0001。
     */
    quint16 defaultBaseAddress() const override { return static_cast<quint16>(40001); }

    /**
     * @brief 获取控制寄存器读取数量
     * @return 4
     * 函数级注释：控制寄存器位于 40001 与 40004，中间存在空洞。
     * 为了保持读取为连续区间，这里返回 4 表示读取 40001..40004。
     */
    int controlRegisterCount() const override { return 4; }

    /**
     * @brief 获取状态寄存器起始地址
     * @return 40007
     * 函数级注释：状态寄存器位于 40007 与 40012，中间存在空洞。
     */
    quint16 statusRegisterBase() const override { return static_cast<quint16>(40007); }

    /**
     * @brief 获取状态寄存器读取数量
     * @return 6
     * 函数级注释：为了连续读取，这里读取 40007..40012 共 6 个寄存器，
     * 其中 40007 与 40012 有效，其余寄存器忽略。
     */
    int statusRegisterCount() const override { return 6; }

    /**
     * @brief 将界面地址转换为 Modbus 协议地址
     * @param uiAddress 界面显示的地址
     * @return 协议地址
     * 函数级注释：Siemens 保持寄存器前缀 4，如 40001 表示偏移 0，40006 表示偏移 5。
     */
    quint16 toProtocolAddress(quint16 uiAddress) const override
    {
        if (uiAddress >= 40001) {
            return static_cast<quint16>(uiAddress - 40001);
        }
        return uiAddress > 0 ? static_cast<quint16>(uiAddress - 1) : static_cast<quint16>(0);
    }

    /**
     * @brief 获取索引对应的控制寄存器偏移
     * @param index 逻辑输出索引（0..31）
     * @return regOffset（0 或 3）
     * 函数级注释：控制寄存器使用 40001(偏移0) 与 40004(偏移3)。
     */
    int controlRegisterOffsetForIndex(int index) const override { return (index < 16) ? 0 : 3; }

    /**
     * @brief 生成 32 路控制标签
     * @param base 起始地址（通常为 40001）
     * @return 标签列表
     * 函数级注释：标签以“地址.bit”展示，例如 40001.0 表示 40001 寄存器的 bit0。
     */
    QStringList makeZLabels(quint16 base) const override
    {
        QStringList labels;
        labels.reserve(32);
        for (int i = 0; i < 32; ++i) {
            const int addr = (i < 16) ? static_cast<int>(base) : (static_cast<int>(base) + 3);
            const int bit = (i % 16);
            labels << QString("%1.%2").arg(addr).arg(bit);
        }
        return labels;
    }

    /**
     * @brief 生成 32 路状态标签
     * @param base 起始地址（通常为 40007）
     * @return 标签列表
     * 函数级注释：标签以“地址.bit”展示，例如 40007.0 表示 40007 寄存器的 bit0。
     */
    QStringList makeSLabels(quint16 base) const override
    {
        Q_UNUSED(base);
        QStringList labels;
        labels.reserve(32);
        for (int i = 0; i < 32; ++i) {
            const int addr = (i < 16) ? 40007 : 40012;
            const int bit = (i % 16);
            labels << QString("%1.%2").arg(addr).arg(bit);
        }
        return labels;
    }

    /**
     * @brief 将两个控制寄存器拆为 32 路状态
     * @param regs 寄存器数组（期望至少 2 个）
     * @param outStates 输出状态（32 路）
     * @param outCount 输出长度
     * 函数级注释：Z0..Z15 对应 40001 的 bit0..15，Z16..Z31 对应 40002 的 bit0..15。
     */
    void applyControlReadRegisters(const QVector<quint16>& regs, bool* outStates, int outCount) const override
    {
        if (outCount <= 0) return;

        if (regs.size() >= 1) {
            const quint16 v0 = regs.at(0);
            for (int b = 0; b < 16; ++b) {
                const int i = b;
                if (i >= outCount) break;
                outStates[i] = ((v0 >> b) & 0x1) != 0;
            }
        }

        if (regs.size() >= 4) {
            const quint16 v1 = regs.at(3);
            for (int b = 0; b < 16; ++b) {
                const int i = 16 + b;
                if (i >= outCount) break;
                outStates[i] = ((v1 >> b) & 0x1) != 0;
            }
        }
    }

    /**
     * @brief 将两个状态寄存器拆为 32 路状态
     * @param regs 寄存器数组（读取范围为 40007..40012）
     * @param outStates 输出状态（32 路）
     * @param outCount 输出长度
     * 函数级注释：S0..S15 对应 40007 的 bit0..15，S16..S31 对应 40012 的 bit0..15。
     */
    void applyStatusReadRegisters(const QVector<quint16>& regs, bool* outStates, int outCount) const override
    {
        if (outCount <= 0) return;

        if (regs.size() >= 1) {
            const quint16 v0 = regs.at(0);
            for (int b = 0; b < 16; ++b) {
                const int i = b;
                if (i >= outCount) break;
                outStates[i] = ((v0 >> b) & 0x1) != 0;
            }
        }

        if (regs.size() >= 6) {
            const quint16 v1 = regs.at(5);
            for (int b = 0; b < 16; ++b) {
                const int i = 16 + b;
                if (i >= outCount) break;
                outStates[i] = ((v1 >> b) & 0x1) != 0;
            }
        }
    }

    /**
     * @brief 组装指定控制寄存器回写值
     * @param regOffset 寄存器偏移（0 或 1）
     * @param states 32 路状态
     * @param stateCount 状态长度
     * @return 16 位寄存器值
     * 函数级注释：把对应 16 路的状态按 bit 组装为寄存器值，用于写单寄存器回写。
     */
    quint16 buildControlRegisterValue(int regOffset, const bool* states, int stateCount) const override
    {
        quint16 v = 0;

        const int start = (regOffset == 0) ? 0 : ((regOffset == 3) ? 16 : -1);
        if (start < 0) {
            return 0;
        }

        for (int b = 0; b < 16; ++b) {
            const int i = start + b;
            if (i >= stateCount) break;
            if (states[i]) v |= (quint16(1) << b);
        }
        return v;
    }
};
} // namespace PlcModBus