#pragma once

#include <QStringList>
#include <QVector>
#include <QtGlobal>

namespace PlcModBus
{
class IPlcRegisterMapping
{
public:
    /**
     * @brief 析构函数
     * 函数级注释：保证通过基类指针释放派生类时行为正确。
     */
    virtual ~IPlcRegisterMapping() = default;

    /**
     * @brief 获取 PLC 类型编号
     * @return PLC 类型编号（例如：0=Beckhoff，1=Siemens）
     * 函数级注释：用于在数据模型中识别当前映射实现。
     */
    virtual int plcType() const = 0;

    /**
     * @brief 获取该 PLC 的默认保持寄存器起始地址
     * @return 起始地址
     * 函数级注释：切换 PLC 类型时用于刷新界面显示与读写起始地址。
     */
    virtual quint16 defaultBaseAddress() const = 0;

    /**
     * @brief 获取控制寄存器读取时的寄存器数量
     * @return 读取数量
     * 函数级注释：不同 PLC/不同映射所需的寄存器数量不同（例如 Siemens 2 个寄存器表示 32 位）。
     */
    virtual int controlRegisterCount() const = 0;

    /**
     * @brief 获取状态寄存器起始地址
     * @return 起始地址
     * 函数级注释：用于读取状态寄存器。
     */
    virtual quint16 statusRegisterBase() const = 0;

    /**
     * @brief 获取状态寄存器读取时的寄存器数量
     * @return 读取数量
     * 函数级注释：不同 PLC/不同映射所需的寄存器数量不同。
     */
    virtual int statusRegisterCount() const = 0;

    /**
     * @brief 将界面地址转换为 Modbus 协议地址
     * @param uiAddress 界面显示的地址
     * @return 协议地址
     * 函数级注释：不同 PLC 的地址转换规则不同（如 0 基、1 基、或 Siemens 40001 偏移）。
     */
    virtual quint16 toProtocolAddress(quint16 uiAddress) const = 0;

    /**
     * @brief 将逻辑输出索引（Z0..Z31）映射到控制寄存器偏移
     * @param index 逻辑输出索引（0..31）
     * @return 寄存器偏移（0 表示 base，1 表示 base+1 ...）
     * 函数级注释：用于写入时确定该索引应该回写哪个寄存器。
     */
    virtual int controlRegisterOffsetForIndex(int index) const = 0;

    /**
     * @brief 生成 32 路 Z 的标签文本
     * @param base 起始地址
     * @return 长度为 32 的标签列表
     * 函数级注释：界面仅负责显示，标签由映射类生成，避免 UI 耦合具体 PLC 规则。
     */
    virtual QStringList makeZLabels(quint16 base) const = 0;

    /**
     * @brief 生成 32 路状态的标签文本
     * @param base 起始地址
     * @return 长度为 32 的标签列表
     * 函数级注释：界面仅负责显示，标签由映射类生成，避免 UI 耦合具体 PLC 规则。
     */
    virtual QStringList makeSLabels(quint16 base) const = 0;

    /**
     * @brief 将读取到的控制寄存器值拆分为 32 路逻辑状态
     * @param regs 读取到的保持寄存器数组（与 controlRegisterCount 一致或更长）
     * @param outStates 输出状态数组指针
     * @param outCount outStates 的长度（应为 32）
     * 函数级注释：负责把寄存器数据解析为每个 Z 勾选框的开关状态。
     */
    virtual void applyControlReadRegisters(const QVector<quint16>& regs, bool* outStates, int outCount) const = 0;

    /**
     * @brief 将读取到的状态寄存器值拆分为 32 路逻辑状态
     * @param regs 读取到的保持寄存器数组（与 statusRegisterCount 一致或更长）
     * @param outStates 输出状态数组指针
     * @param outCount outStates 的长度（应为 32）
     * 函数级注释：负责把寄存器数据解析为每个状态勾选框的开关状态。
     */
    virtual void applyStatusReadRegisters(const QVector<quint16>& regs, bool* outStates, int outCount) const = 0;

    /**
     * @brief 根据 32 路控制状态组装指定寄存器的回写值
     * @param regOffset 寄存器偏移
     * @param states 32 路状态数组指针
     * @param stateCount states 长度（应为 32）
     * @return 要写入该寄存器的 16 位值
     * 函数级注释：用于写入时“按位更新后回写整寄存器”，确保 PLC 侧数据一致。
     */
    virtual quint16 buildControlRegisterValue(int regOffset, const bool* states, int stateCount) const = 0;
};
} // namespace PlcModBus