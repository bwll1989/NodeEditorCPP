//
// Created by WuBin on 24-10-29.
//
#pragma once
#include "QtNodes/NodeData"
#include <QMetaType>
#include <memory>
#include <QObject>  // 添加QObject头文件
#include "AudioTimestampRingQueue.h"
using QtNodes::NodeData;
using QtNodes::NodeDataType;
namespace NodeDataTypes
{
    /**
     * @brief 音频数据类，直接持有共享环形缓冲区指针
     */
    class DATATYPES_EXPORT AudioData : public NodeData
    {
    public:
        AudioData() ;

        NodeDataType type() const override ;

        /**
         * @brief 设置共享环形缓冲区（用于连接建立时）
         * @param buffer 共享的环形缓冲区指针
         */
        void setSharedAudioBuffer(std::shared_ptr<AudioTimestampRingQueue> buffer) ;

        /**
         * @brief 获取共享环形缓冲区指针
         * @return 环形缓冲区指针，nullptr表示未连接
         */
        std::shared_ptr<AudioTimestampRingQueue> getSharedAudioBuffer() const ;

        /**
         * @brief 检查是否已连接到共享缓冲区
         * @return true表示已连接，false表示未连接
         */
        bool isConnectedToSharedBuffer() const ;

        /**
         * @brief 断开连接
         */
        void disconnect() ;

        
    private:
        // std::shared_ptr<AudioTimestampQueue> sharedAudioBuffer_; // 共享环形缓冲区指针
        std::shared_ptr<AudioTimestampRingQueue> sharedAudioBuffer_; // 共享环形缓冲区指针
    };
}


    