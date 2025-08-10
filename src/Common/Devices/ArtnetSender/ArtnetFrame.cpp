//
// Created by bwll1 on 2024/9/1.
//

#include "ArtnetFrame.h"
#include <QDateTime>
#include <QHostAddress>

/**
 * @brief 默认构造函数
 */
ArtnetFrame::ArtnetFrame()
    : host("127.0.0.1")
    , universe(0)
    , sequence(0)
    , port(6454)
    , timestamp(QDateTime::currentMSecsSinceEpoch())
{
    dmxData.fill(0, 512); // 初始化为512字节的零数据
}

/**
 * @brief 构造函数
 */
ArtnetFrame::ArtnetFrame(const QString& targetHost, quint16 universe, const QByteArray& dmxData, quint8 sequence)
    : host(targetHost)
    , universe(universe)
    , sequence(sequence)
    , port(6454)
    , timestamp(QDateTime::currentMSecsSinceEpoch())
{
    setDmxData(dmxData);
}

/**
 * @brief 拷贝构造函数
 */
ArtnetFrame::ArtnetFrame(const ArtnetFrame& other)
    : host(other.host)
    , universe(other.universe)
    , dmxData(other.dmxData)
    , sequence(other.sequence)
    , port(other.port)
    , timestamp(other.timestamp)
    , description(other.description)
{
}

/**
 * @brief 赋值操作符
 */
ArtnetFrame& ArtnetFrame::operator=(const ArtnetFrame& other)
{
    if (this != &other) {
        host = other.host;
        universe = other.universe;
        dmxData = other.dmxData;
        sequence = other.sequence;
        port = other.port;
        timestamp = other.timestamp;
        description = other.description;
    }
    return *this;
}

/**
 * @brief 检查数据帧是否有效
 */
bool ArtnetFrame::isValid() const
{
    // 检查主机地址是否有效
    QHostAddress addr(host);
    if (addr.isNull()) {
        return false;
    }
    
    // 检查宇宙编号范围
    if (universe > 32767) {
        return false;
    }
    
    // 检查端口范围
    if (port == 0 || port > 65535) {
        return false;
    }
    
    // 检查DMX数据长度
    if (dmxData.size() > 512) {
        return false;
    }
    
    return true;
}

/**
 * @brief 设置DMX数据
 */
void ArtnetFrame::setDmxData(const QByteArray& data)
{
    if (data.size() <= 512) {
        // 填充到512字节
        dmxData = data.leftJustified(512, 0x00);
    } else {
        // 截取前512字节
        dmxData = data.left(512);
    }
}

/**
 * @brief 获取格式化的DMX数据（512字节）
 */
QByteArray ArtnetFrame::getFormattedDmxData() const
{
    return dmxData.leftJustified(512, 0x00);
}