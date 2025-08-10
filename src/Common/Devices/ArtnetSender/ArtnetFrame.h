//
// Created by bwll1 on 2024/9/1.
//

#ifndef NODEEDITORCPP_ARTNETFRAME_H
#define NODEEDITORCPP_ARTNETFRAME_H

#include <QByteArray>
#include <qmetatype.h>
#include <QString>
#include <QtCore/qglobal.h>

#if defined(ARTNETTRANSMITTER_LIBRARY)
#define ARTNETTRANSMITTER_EXPORT Q_DECL_EXPORT
#else
#define ARTNETTRANSMITTER_EXPORT Q_DECL_IMPORT
#endif

/**
 * @brief Art-Net数据帧结构
 * 
 * 包含发送Art-Net数据包所需的所有信息
 */
struct ARTNETTRANSMITTER_EXPORT ArtnetFrame
{
    /**
     * @brief 默认构造函数
     */
    ArtnetFrame();

    /**
     * @brief 构造函数
     * @param targetHost 目标主机地址
     * @param universe 宇宙编号
     * @param dmxData DMX数据（最多512字节）
     * @param sequence 序列号，默认为0
     */
    ArtnetFrame(const QString& targetHost, quint16 universe, const QByteArray& dmxData, quint8 sequence = 0);

    /**
     * @brief 拷贝构造函数
     */
    ArtnetFrame(const ArtnetFrame& other);

    /**
     * @brief 赋值操作符
     */
    ArtnetFrame& operator=(const ArtnetFrame& other);

    /**
     * @brief 检查数据帧是否有效
     * @return bool 数据帧是否有效
     */
    bool isValid() const;

    /**
     * @brief 设置DMX数据
     * @param data DMX数据，自动调整为512字节
     */
    void setDmxData(const QByteArray& data);

    /**
     * @brief 获取格式化的DMX数据（512字节）
     * @return QByteArray 格式化后的DMX数据
     */
    QByteArray getFormattedDmxData() const;

    // 数据成员
    QString host;           ///< 目标主机地址
    quint16 universe;       ///< 宇宙编号 (0-32767)
    QByteArray dmxData;     ///< DMX数据 (最多512字节)
    quint8 sequence;        ///< 序列号 (0-255)
    quint16 port;           ///< 目标端口，默认6454
    
    // 可选的元数据
    qint64 timestamp;       ///< 时间戳（毫秒）
    QString description;    ///< 描述信息
};

/**
 * @brief 注册ArtnetFrame为Qt元类型
 */
Q_DECLARE_METATYPE(ArtnetFrame)

#endif //NODEEDITORCPP_ARTNETFRAME_H