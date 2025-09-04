#pragma once
#include <QDataStream>
#include <QIODevice>
#include <pluginterfaces/base/ibstream.h>

namespace Nodes {

/**
 * @brief VST3数据流适配器类
 * 
 * 将Qt的QDataStream适配为VST3 SDK要求的Steinberg::IBStream接口，
 * 用于VST3插件状态的序列化和反序列化操作。
 * 确保数据以小端字节序存储，保证跨平台兼容性。
 */
class Vst3DataStream : public Steinberg::IBStream
{
public:
    /**
     * @brief 构造函数
     * @param stream Qt数据流引用
     */
    explicit Vst3DataStream(QDataStream& stream);
    
    /**
     * @brief 析构函数
     */
    virtual ~Vst3DataStream() = default;

    // 实现Steinberg::IBStream接口
    
    /**
     * @brief 查询接口（VST3接口要求）
     * @param _iid 接口ID
     * @param obj 输出对象指针
     * @return 查询结果
     */
    Steinberg::tresult queryInterface(const Steinberg::TUID _iid, void** obj) override;
    
    /**
     * @brief 增加引用计数（VST3接口要求）
     * @return 引用计数
     */
    Steinberg::uint32 addRef() override;
    
    /**
     * @brief 释放引用计数（VST3接口要求）
     * @return 引用计数
     */
    Steinberg::uint32 release() override;

    /**
     * @brief 从流中读取数据
     * @param buffer 目标缓冲区
     * @param numBytes 要读取的字节数
     * @param numBytesRead 实际读取的字节数（输出参数）
     * @return 操作结果
     */
    Steinberg::tresult read(void* buffer, Steinberg::int32 numBytes, Steinberg::int32* numBytesRead) override;

    /**
     * @brief 向流中写入数据
     * @param buffer 源缓冲区
     * @param numBytes 要写入的字节数
     * @param numBytesWritten 实际写入的字节数（输出参数）
     * @return 操作结果
     */
    Steinberg::tresult write(void* buffer, Steinberg::int32 numBytes, Steinberg::int32* numBytesWritten) override;

    /**
     * @brief 设置流位置
     * @param pos 目标位置
     * @param mode 定位模式
     * @param result 实际位置（输出参数）
     * @return 操作结果
     */
    Steinberg::tresult seek(Steinberg::int64 pos, Steinberg::int32 mode, Steinberg::int64* result) override;

    /**
     * @brief 获取当前流位置
     * @param pos 当前位置（输出参数）
     * @return 操作结果
     */
    Steinberg::tresult tell(Steinberg::int64* pos) override;

private:
    QDataStream& stream_;  ///< Qt数据流引用
};

} // namespace Nodes