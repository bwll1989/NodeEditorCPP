/**
 * @file GDTFParser.hpp
 * @brief GDTF (General Device Type Format) 灯具文件解析器
 *
 * 基于 minizip（zip 解包）+ tinyxml2（XML 解析）实现 GDTF 1.0/1.1/1.2
 * description.xml 的结构化解析，输出 QVariantMap 供节点下游消费。
 *
 * 规范来源：src/Nodes/GDTFNode/gdtf-spec.md（GDTF 1.2 Draft 1）
 *
 * 解析范围：
 *  - FixtureType 元数据（厂商/名称/GUID/缩略图）
 *  - AttributeDefinitions（ActivationGroups / FeatureGroups / Attributes）
 *  - Wheels（色轮/ gobos 轮与轮片）
 *  - Models（几何模型尺寸与原始体类型）
 *  - Geometries（递归几何树，含 Position 4x4 矩阵与 Beam 光输出参数）
 *  - DMXModes（DMXChannel → LogicalChannel → ChannelFunction → ChannelSet
 *              完整链路，含 Relations / Macros）
 *  - Revisions（修订历史）
 */

#pragma once

#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QVariantList>
#include <QByteArray>
#include <tinyxml2.h>

namespace Nodes
{

class GDTFParser
{
public:
    GDTFParser() = default;

    /**
     * @brief 解析 .gdtf 文件（zip 容器）
     * @param gdtfPath .gdtf 文件绝对路径
     * @return 成功返回 true；失败原因见 lastError()
     */
    bool parseFile(const QString &gdtfPath);

    /** 解析已解出的 description.xml 内容（便于测试与外部调用） */
    bool parseDescriptionXml(const QByteArray &xml);

    QString lastError() const { return m_lastError; }

    /** 解析结果（完整键值表，见 Doc.md 键说明） */
    const QVariantMap &result() const { return m_result; }

    /** description.xml 原始字节（从 zip 中解出） */
    const QByteArray &rawXml() const { return m_rawXml; }

    /** 从 zip 中读取缩略图字节（thumbnail.png，不存在则返回空） */
    static QByteArray readThumbnail(const QString &gdtfPath);

    /** "255/1" / "65535/2" / "255/1s" → 归一化 double（0~1）；缺省时用 defaultValue/255 */
    static double parseDmxValue(const char *text, int defaultValue = 0);

    /** DMXValue 文本 → {value, bits, shifted, normalized} */
    static QVariantMap parseDmxValueMap(const QString &text);

    /** Matrix 文本 "{a,b,c,d} {...} {...} {...}" → 16 元素列表（行主序存储） */
    static QVariantList parseMatrix(const QString &text);

    /** "1,2,3" → [1,2,3]；"None"/空 → 空表 */
    static QVariantList parseIntArray(const QString &text);

private:
    void parseFixtureType(const tinyxml2::XMLElement *node);
    void parseAttributeDefinitions(const tinyxml2::XMLElement *node);
    void parseWheels(const tinyxml2::XMLElement *node);
    void parsePhysicalDescriptions(const tinyxml2::XMLElement *node);
    void parseModels(const tinyxml2::XMLElement *node);
    void parseGeometries(const tinyxml2::XMLElement *node);
    void parseDmxModes(const tinyxml2::XMLElement *node);
    void parseRevisions(const tinyxml2::XMLElement *node);

    QVariantList parseGeometryChildren(const tinyxml2::XMLElement *parentElement);
    void parseDmxChannel(const tinyxml2::XMLElement *element, QVariantMap &out);
    void parseLogicalChannel(const tinyxml2::XMLElement *element, QVariantMap &out);
    void parseChannelFunction(const tinyxml2::XMLElement *element, QVariantMap &out);

    QVariantMap m_result;
    QByteArray  m_rawXml;
    QString     m_lastError;
};

} // namespace Nodes
