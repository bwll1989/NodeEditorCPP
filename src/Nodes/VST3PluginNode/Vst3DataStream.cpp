#include "Vst3DataStream.hpp"
#include <QDebug>

namespace Nodes {

/**
 * @brief 构造函数，初始化数据流适配器
 * @param stream Qt数据流引用
 */
Vst3DataStream::Vst3DataStream(QDataStream& stream)
    : stream_(stream)
{
    // 设置为小端字节序，确保跨平台兼容性
    stream_.setByteOrder(QDataStream::LittleEndian);
}

/**
 * @brief 查询接口实现（VST3接口要求）
 * @param _iid 接口ID
 * @param obj 输出对象指针
 * @return 始终返回kResultFalse，表示不支持其他接口
 */
Steinberg::tresult Vst3DataStream::queryInterface(const Steinberg::TUID _iid, void** obj)
{
    Q_UNUSED(_iid)
    Q_UNUSED(obj)
    return Steinberg::kResultFalse;
}

/**
 * @brief 增加引用计数（简化实现）
 * @return 固定返回1
 */
Steinberg::uint32 Vst3DataStream::addRef()
{
    return 1;
}

/**
 * @brief 释放引用计数（简化实现）
 * @return 固定返回1
 */
Steinberg::uint32 Vst3DataStream::release()
{
    return 1;
}

/**
 * @brief 从流中读取数据
 * @param buffer 目标缓冲区
 * @param numBytes 要读取的字节数
 * @param numBytesRead 实际读取的字节数（输出参数）
 * @return 操作结果
 */
Steinberg::tresult Vst3DataStream::read(void* buffer, Steinberg::int32 numBytes, Steinberg::int32* numBytesRead)
{
    if (!buffer || numBytes <= 0) {
        if (numBytesRead) {
            *numBytesRead = 0;
        }
        return Steinberg::kInvalidArgument;
    }

    try {
        int bytesRead = stream_.readRawData(static_cast<char*>(buffer), numBytes);
        
        if (numBytesRead) {
            *numBytesRead = bytesRead;
        }
        
        // 检查是否读取成功
        if (bytesRead < 0) {
            qWarning() << "Vst3DataStream::read - 读取数据失败";
            return Steinberg::kResultFalse;
        }
        
        return Steinberg::kResultTrue;
    }
    catch (const std::exception& e) {
        qWarning() << "Vst3DataStream::read - 异常:" << e.what();
        if (numBytesRead) {
            *numBytesRead = 0;
        }
        return Steinberg::kResultFalse;
    }
}

/**
 * @brief 向流中写入数据
 * @param buffer 源缓冲区
 * @param numBytes 要写入的字节数
 * @param numBytesWritten 实际写入的字节数（输出参数）
 * @return 操作结果
 */
Steinberg::tresult Vst3DataStream::write(void* buffer, Steinberg::int32 numBytes, Steinberg::int32* numBytesWritten)
{
    if (!buffer || numBytes <= 0) {
        if (numBytesWritten) {
            *numBytesWritten = 0;
        }
        return Steinberg::kInvalidArgument;
    }

    try {
        int bytesWritten = stream_.writeRawData(static_cast<const char*>(buffer), numBytes);
        
        if (numBytesWritten) {
            *numBytesWritten = bytesWritten;
        }
        
        // 检查是否写入成功
        if (bytesWritten < 0) {
            qWarning() << "Vst3DataStream::write - 写入数据失败";
            return Steinberg::kResultFalse;
        }
        
        return Steinberg::kResultTrue;
    }
    catch (const std::exception& e) {
        qWarning() << "Vst3DataStream::write - 异常:" << e.what();
        if (numBytesWritten) {
            *numBytesWritten = 0;
        }
        return Steinberg::kResultFalse;
    }
}

/**
 * @brief 设置流位置
 * @param pos 目标位置
 * @param mode 定位模式（当前实现忽略此参数）
 * @param result 实际位置（输出参数）
 * @return 操作结果
 */
Steinberg::tresult Vst3DataStream::seek(Steinberg::int64 pos, Steinberg::int32 mode, Steinberg::int64* result)
{
    Q_UNUSED(mode)  // 简化实现，忽略定位模式
    
    try {
        QIODevice* device = stream_.device();
        if (!device) {
            qWarning() << "Vst3DataStream::seek - 无效的设备";
            if (result) {
                *result = -1;
            }
            return Steinberg::kResultFalse;
        }
        
        bool success = device->seek(pos);
        
        if (result) {
            *result = device->pos();
        }
        
        return success ? Steinberg::kResultTrue : Steinberg::kResultFalse;
    }
    catch (const std::exception& e) {
        qWarning() << "Vst3DataStream::seek - 异常:" << e.what();
        if (result) {
            *result = -1;
        }
        return Steinberg::kResultFalse;
    }
}

/**
 * @brief 获取当前流位置
 * @param pos 当前位置（输出参数）
 * @return 操作结果
 */
Steinberg::tresult Vst3DataStream::tell(Steinberg::int64* pos)
{
    try {
        QIODevice* device = stream_.device();
        if (!device) {
            qWarning() << "Vst3DataStream::tell - 无效的设备";
            if (pos) {
                *pos = -1;
            }
            return Steinberg::kResultFalse;
        }
        
        if (pos) {
            *pos = device->pos();
        }
        
        return Steinberg::kResultTrue;
    }
    catch (const std::exception& e) {
        qWarning() << "Vst3DataStream::tell - 异常:" << e.what();
        if (pos) {
            *pos = -1;
        }
        return Steinberg::kResultFalse;
    }
}

} // namespace Nodes