/**
 * @file GDTFParser.cpp
 * @brief GDTF 灯具文件解析实现（minizip + tinyxml2）
 *
 * 输出键值约定（详见 Doc.md）：
 *  {
 *    dataVersion, manufacturer, name, shortName, longName, description,
 *    fixtureTypeId, thumbnail, canHaveChildren,
 *    attributes:      [ {name, pretty, activationGroup, feature, mainAttribute, physicalUnit} ],
 *    activationGroups:[ {name} ],
 *    featureGroups:   [ {name, pretty, features:[name...]} ],
 *    wheels:          [ {name, slots:[{name, color, dmxFrom, wheelSlotIndex}]} ],
 *    emitters / filters: [ {name, ...} ],
 *    models:          [ {name, length, width, height, primitiveType, file} ],
 *    geometries:      [ {type, name, model, position:[16], children:[...], beam:{...}} ],
 *    dmxModes:        [ {name, description, geometry, channelCount, footprint,
 *                        channels:[ {name, attribute, geometry, dmxBreak, offsets,
 *                                    highlight, snap, mibFade, default,
 *                                    functions:[ {name, attribute, dmxFrom, default,
 *                                                 physicalFrom, physicalTo, wheel,
 *                                                 customName, sets:[{name, dmxFrom}]} ]} ],
 *                        relations:[ {name, master, follower, type} ] } ],
 *    revisions:       [ {text, date, modifiedBy} ]
 *  }
 */
#include "GDTFParser.hpp"

#include <QFile>
#include <QFileInfo>
#include <QtMath>
#include <cmath>

// ---- minizip（zlib contrib）----
#include "unzip.h"
#include "ioapi.h"

using namespace tinyxml2;

namespace
{

using Nodes::GDTFParser;

QString attrStr(const XMLElement *e, const char *name, const QString &fallback = QString())
{
    if (!e) return fallback;
    const char *v = e->Attribute(name);
    return v ? QString::fromUtf8(v) : fallback;
}

double attrD(const XMLElement *e, const char *name, double fallback = 0.0)
{
    if (!e) return fallback;
    double v = 0.0;
    if (e->QueryDoubleAttribute(name, &v) == XML_SUCCESS) return v;
    return fallback;
}

int attrI(const XMLElement *e, const char *name, int fallback = 0)
{
    if (!e) return fallback;
    int v = 0;
    if (e->QueryIntAttribute(name, &v) == XML_SUCCESS) return v;
    return fallback;
}

/** 从 zip 内读取第一个名字匹配（忽略大小写、忽略前导 ./）的条目 */
QByteArray readZipEntry(const QString &zipPath, const QString &wantedName)
{
    QByteArray out;
    unzFile zf = unzOpen(zipPath.toUtf8().constData());
    if (!zf) return QByteArray();

    if (unzGoToFirstFile(zf) == UNZ_OK) {
        for (;;) {
            char name[1024] = {0};
            unz_file_info info;
            memset(&info, 0, sizeof(info));
            if (unzGetCurrentFileInfo(zf, &info, name, sizeof(name) - 1,
                                      nullptr, 0, nullptr, 0) == UNZ_OK) {
                QString qname = QString::fromUtf8(name);
                while (qname.startsWith(QStringLiteral("./")))  // 兼容 "./description.xml"
                    qname.remove(0, 2);
                if (qname.compare(wantedName, Qt::CaseInsensitive) == 0) {
                    if (unzOpenCurrentFile(zf) == UNZ_OK) {
                        char buf[8192];
                        int n = 0;
                        while ((n = unzReadCurrentFile(zf, buf, sizeof(buf))) > 0)
                            out.append(buf, n);
                        unzCloseCurrentFile(zf);
                    }
                    unzClose(zf);
                    return out;
                }
            }
            if (unzGoToNextFile(zf) != UNZ_OK) break;
        }
    }
    unzClose(zf);
    return out;
}

} // namespace

namespace Nodes
{

// ---------------------------------------------------------------- utilities

double GDTFParser::parseDmxValue(const char *text, int defaultValue)
{
    if (!text || !*text) return defaultValue / 255.0;
    const QVariantMap m = parseDmxValueMap(QString::fromUtf8(text));
    if (m.value(QStringLiteral("bits")).toInt() < 0)   // "None"
        return defaultValue / 255.0;
    return m.value(QStringLiteral("normalized")).toDouble();
}

QVariantMap GDTFParser::parseDmxValueMap(const QString &text)
{
    QVariantMap m;
    QString s = text.trimmed();
    if (s.isEmpty() || s.compare(QStringLiteral("None"), Qt::CaseInsensitive) == 0) {
        m.insert(QStringLiteral("value"), 0);
        m.insert(QStringLiteral("bits"), -1);          // -1 = "None"/未指定
        m.insert(QStringLiteral("shifted"), false);
        m.insert(QStringLiteral("normalized"), 0.0);
        return m;
    }

    quint32 value = 0;
    int bits = 1;
    bool shifted = false;
    const int slash = s.indexOf(QLatin1Char('/'));
    if (slash < 0) {
        value = s.toUInt();
    } else {
        value = s.left(slash).toUInt();
        QString rest = s.mid(slash + 1);
        if (rest.endsWith(QLatin1Char('s')) || rest.endsWith(QLatin1Char('S'))) {
            shifted = true;
            rest.chop(1);
        }
        bool ok = false;
        const int b = rest.toInt(&ok);
        bits = (ok && b > 0) ? qMin(b, 4) : 1;
    }
    const double max = std::pow(2.0, 8 * bits) - 1.0;
    m.insert(QStringLiteral("value"), value);
    m.insert(QStringLiteral("bits"), bits);
    m.insert(QStringLiteral("shifted"), shifted);
    m.insert(QStringLiteral("normalized"),
             max > 0 ? qBound(0.0, double(value) / max, 1.0) : 0.0);
    return m;
}

QVariantList GDTFParser::parseMatrix(const QString &text)
{
    // "{a,b,c,d} {...} {...} {...}"：文本按行主序存储；数学定义为列主序
    QVariantList out;
    QString s = text;
    s.remove(QLatin1Char('{')).remove(QLatin1Char('}'));
    const QStringList parts = s.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    for (const QString &p : parts) {
        bool ok = false;
        const double v = p.toDouble(&ok);
        out.append(ok ? v : 0.0);
    }
    while (out.size() < 16)
        out.append(0.0);
    return out;
}

QVariantList GDTFParser::parseIntArray(const QString &text)
{
    QVariantList out;
    const QString s = text.trimmed();
    if (s.isEmpty() || s.compare(QStringLiteral("None"), Qt::CaseInsensitive) == 0)
        return out;
    const QStringList parts = s.split(QLatin1Char(','), Qt::SkipEmptyParts);
    for (const QString &p : parts)
        out.append(p.trimmed().toInt());
    return out;
}

// ---------------------------------------------------------------- zip read

QByteArray GDTFParser::readThumbnail(const QString &gdtfPath)
{
    return readZipEntry(gdtfPath, QStringLiteral("thumbnail.png"));
}

// ---------------------------------------------------------------- main entry

bool GDTFParser::parseFile(const QString &gdtfPath)
{
    m_lastError.clear();
    m_result.clear();

    const QFileInfo fi(gdtfPath);
    if (!fi.exists()) {
        m_lastError = QStringLiteral("file not found: %1").arg(gdtfPath);
        return false;
    }

    m_rawXml = readZipEntry(gdtfPath, QStringLiteral("description.xml"));
    if (m_rawXml.isEmpty()) {
        m_lastError = QStringLiteral("description.xml not found in %1").arg(fi.fileName());
        return false;
    }
    return parseDescriptionXml(m_rawXml);
}

bool GDTFParser::parseDescriptionXml(const QByteArray &xml)
{
    m_lastError.clear();
    m_result.clear();

    XMLDocument doc;
    if (doc.Parse(xml.constData(), size_t(xml.size())) != XML_SUCCESS) {
        m_lastError = QStringLiteral("XML parse error: %1").arg(QString::fromUtf8(doc.GetErrorStr1()));
        return false;
    }

    const XMLElement *root = doc.RootElement();
    if (!root || qstrcmp(root->Name(), "GDTF") != 0) {
        m_lastError = QStringLiteral("not a GDTF file (root node is not <GDTF>)");
        return false;
    }

    m_result.insert(QStringLiteral("dataVersion"), attrStr(root, "DataVersion"));

    const XMLElement *ft = root->FirstChildElement("FixtureType");
    if (!ft) {
        m_lastError = QStringLiteral("<FixtureType> node missing");
        return false;
    }
    parseFixtureType(ft);
    return true;
}

// ------------------------------------------------------------- FixtureType

void GDTFParser::parseFixtureType(const XMLElement *ft)
{
    m_result.insert(QStringLiteral("name"),            attrStr(ft, "Name"));
    m_result.insert(QStringLiteral("shortName"),       attrStr(ft, "ShortName"));
    m_result.insert(QStringLiteral("longName"),        attrStr(ft, "LongName"));
    m_result.insert(QStringLiteral("manufacturer"),    attrStr(ft, "Manufacturer"));
    m_result.insert(QStringLiteral("description"),     attrStr(ft, "Description"));
    m_result.insert(QStringLiteral("fixtureTypeId"),   attrStr(ft, "FixtureTypeID"));
    m_result.insert(QStringLiteral("thumbnail"),       attrStr(ft, "Thumbnail"));
    m_result.insert(QStringLiteral("canHaveChildren"), attrStr(ft, "CanHaveChildren", "Yes"));

    // 规范表 4：段落顺序固定；缺省段落跳过
    for (const XMLElement *e = ft->FirstChildElement(); e; e = e->NextSiblingElement()) {
        const char *tag = e->Name();
        if      (qstrcmp(tag, "AttributeDefinitions") == 0) parseAttributeDefinitions(e);
        else if (qstrcmp(tag, "Wheels") == 0)               parseWheels(e);
        else if (qstrcmp(tag, "PhysicalDescriptions") == 0) parsePhysicalDescriptions(e);
        else if (qstrcmp(tag, "Models") == 0)               parseModels(e);
        else if (qstrcmp(tag, "Geometries") == 0)           parseGeometries(e);
        else if (qstrcmp(tag, "DMXModes") == 0)             parseDmxModes(e);
        else if (qstrcmp(tag, "Revisions") == 0)            parseRevisions(e);
        // FTPresets / Protocols：非核心控制数据，暂不展开
    }
}

// --------------------------------------------------- AttributeDefinitions

void GDTFParser::parseAttributeDefinitions(const XMLElement *node)
{
    QVariantList activationGroups;
    QVariantList featureGroups;
    QVariantList attributes;

    for (const XMLElement *e = node->FirstChildElement(); e; e = e->NextSiblingElement()) {
        if (qstrcmp(e->Name(), "ActivationGroups") == 0) {
            for (const XMLElement *g = e->FirstChildElement("ActivationGroup"); g;
                 g = g->NextSiblingElement("ActivationGroup")) {
                QVariantMap gm;
                gm.insert(QStringLiteral("name"), attrStr(g, "Name"));
                activationGroups.append(gm);
            }
        } else if (qstrcmp(e->Name(), "FeatureGroups") == 0) {
            for (const XMLElement *g = e->FirstChildElement("FeatureGroup"); g;
                 g = g->NextSiblingElement("FeatureGroup")) {
                QVariantList features;
                for (const XMLElement *f = g->FirstChildElement("Feature"); f;
                     f = f->NextSiblingElement("Feature"))
                    features.append(attrStr(f, "Name"));
                QVariantMap gm;
                gm.insert(QStringLiteral("name"),    attrStr(g, "Name"));
                gm.insert(QStringLiteral("pretty"),  attrStr(g, "Pretty"));
                gm.insert(QStringLiteral("features"), features);
                featureGroups.append(gm);
            }
        } else if (qstrcmp(e->Name(), "Attributes") == 0) {
            for (const XMLElement *a = e->FirstChildElement("Attribute"); a;
                 a = a->NextSiblingElement("Attribute")) {
                QVariantMap am;
                am.insert(QStringLiteral("name"),            attrStr(a, "Name"));
                am.insert(QStringLiteral("pretty"),          attrStr(a, "Pretty"));
                am.insert(QStringLiteral("activationGroup"), attrStr(a, "ActivationGroup"));
                am.insert(QStringLiteral("feature"),         attrStr(a, "Feature"));
                am.insert(QStringLiteral("mainAttribute"),   attrStr(a, "MainAttribute"));
                am.insert(QStringLiteral("physicalUnit"),    attrStr(a, "PhysicalUnit", "None"));
                const char *cie = a->Attribute("Color");
                if (cie) am.insert(QStringLiteral("colorCIE"), QString::fromUtf8(cie));
                attributes.append(am);
            }
        }
    }

    m_result.insert(QStringLiteral("activationGroups"), activationGroups);
    m_result.insert(QStringLiteral("featureGroups"),    featureGroups);
    m_result.insert(QStringLiteral("attributes"),       attributes);
}

// ------------------------------------------------------------------- Wheels

void GDTFParser::parseWheels(const XMLElement *node)
{
    QVariantList wheels;
    for (const XMLElement *w = node->FirstChildElement("Wheel"); w;
         w = w->NextSiblingElement("Wheel")) {
        QVariantMap wm;
        wm.insert(QStringLiteral("name"), attrStr(w, "Name"));
        QVariantList slotList;
        int slotIndex = 0;
        for (const XMLElement *s = w->FirstChildElement("Slot"); s;
             s = s->NextSiblingElement("Slot")) {
            QVariantMap sm;
            sm.insert(QStringLiteral("name"),   attrStr(s, "Name"));
            const char *color = s->Attribute("Color");
            if (color) sm.insert(QStringLiteral("color"), QString::fromUtf8(color));
            sm.insert(QStringLiteral("dmxFrom"),
                      parseDmxValueMap(attrStr(s, "DMXFrom", "0/1")));
            const int wsi = attrI(s, "WheelSlotIndex", 0);
            sm.insert(QStringLiteral("wheelSlotIndex"),
                      wsi > 0 ? wsi : slotIndex + 1);   // 规范：归一化为 1 起
            slotList.append(sm);
            ++slotIndex;
        }
        wm.insert(QStringLiteral("slots"), slotList);
        wheels.append(wm);
    }
    m_result.insert(QStringLiteral("wheels"), wheels);
}

// --------------------------------------------------- PhysicalDescriptions

void GDTFParser::parsePhysicalDescriptions(const XMLElement *node)
{
    QVariantList emitters;
    QVariantList filters;

    for (const XMLElement *e = node->FirstChildElement(); e; e = e->NextSiblingElement()) {
        if (qstrcmp(e->Name(), "EmitterCollect") == 0) {
            for (const XMLElement *em = e->FirstChildElement("Emitter"); em;
                 em = em->NextSiblingElement("Emitter")) {
                QVariantMap m;
                m.insert(QStringLiteral("name"), attrStr(em, "Name"));
                const char *cie = em->Attribute("Color");
                if (cie) m.insert(QStringLiteral("colorCIE"), QString::fromUtf8(cie));
                // 取第一个 Measurement 的 LuminousIntensity（cd）作为光通参考
                if (const XMLElement *meas = em->FirstChildElement("Measurement")) {
                    const char *li = meas->Attribute("LuminousIntensity");
                    bool ok = false;
                    const double v = li ? QString::fromUtf8(li).toDouble(&ok) : 0.0;
                    if (ok && v > 0) m.insert(QStringLiteral("luminousIntensity"), v);
                }
                emitters.append(m);
            }
        } else if (qstrcmp(e->Name(), "FilterCollect") == 0) {
            for (const XMLElement *f = e->FirstChildElement("Filter"); f;
                 f = f->NextSiblingElement("Filter")) {
                QVariantMap m;
                m.insert(QStringLiteral("name"), attrStr(f, "Name"));
                filters.append(m);
            }
        }
    }

    if (!emitters.isEmpty()) m_result.insert(QStringLiteral("emitters"), emitters);
    if (!filters.isEmpty())  m_result.insert(QStringLiteral("filters"),  filters);
}

// -------------------------------------------------------------------- Models

void GDTFParser::parseModels(const XMLElement *node)
{
    QVariantList models;
    for (const XMLElement *m = node->FirstChildElement("Model"); m;
         m = m->NextSiblingElement("Model")) {
        QVariantMap mm;
        mm.insert(QStringLiteral("name"),          attrStr(m, "Name"));
        mm.insert(QStringLiteral("length"),        attrD(m, "Length"));
        mm.insert(QStringLiteral("width"),         attrD(m, "Width"));
        mm.insert(QStringLiteral("height"),        attrD(m, "Height"));
        mm.insert(QStringLiteral("primitiveType"), attrStr(m, "PrimitiveType", "Undefined"));
        const char *file = m->Attribute("File");
        if (file) mm.insert(QStringLiteral("file"), QString::fromUtf8(file));
        models.append(mm);
    }
    m_result.insert(QStringLiteral("models"), models);
}

// ----------------------------------------------------------------- Geometries

QVariantList GDTFParser::parseGeometryChildren(const XMLElement *parent)
{
    QVariantList out;
    if (!parent) return out;

    static const QSet<QString> kGeometryTypes = {
        QStringLiteral("Geometry"), QStringLiteral("Axis"),
        QStringLiteral("FilterBeam"), QStringLiteral("FilterColor"),
        QStringLiteral("FilterGobo"), QStringLiteral("FilterShaper"),
        QStringLiteral("Beam"), QStringLiteral("MediaServerLayer"),
        QStringLiteral("MediaServerCamera"), QStringLiteral("MediaServerMaster"),
        QStringLiteral("Display"), QStringLiteral("GeometryReference"),
        QStringLiteral("Laser"), QStringLiteral("WiringObject"),
        QStringLiteral("Inventory"), QStringLiteral("Structure"),
        QStringLiteral("Support"), QStringLiteral("Magnet")
    };

    for (const XMLElement *g = parent->FirstChildElement(); g; g = g->NextSiblingElement()) {
        const QString type = QString::fromUtf8(g->Name());
        if (!kGeometryTypes.contains(type)) continue;

        QVariantMap gm;
        gm.insert(QStringLiteral("type"),  type);
        gm.insert(QStringLiteral("name"),  attrStr(g, "Name"));
        gm.insert(QStringLiteral("model"), attrStr(g, "Model"));
        gm.insert(QStringLiteral("position"),
                  parseMatrix(attrStr(g, "Position", "{1,0,0,0} {0,1,0,0} {0,0,1,0} {0,0,0,1}")));

        if (type == QStringLiteral("Beam")) {
            QVariantMap beam;
            beam.insert(QStringLiteral("lampType"),         attrStr(g, "LampType"));
            beam.insert(QStringLiteral("powerConsumption"), attrD(g, "PowerConsumption"));
            beam.insert(QStringLiteral("luminousFlux"),     attrD(g, "LuminousFlux"));
            beam.insert(QStringLiteral("colorTemperature"), attrD(g, "ColorTemperature"));
            beam.insert(QStringLiteral("beamAngle"),        attrD(g, "BeamAngle"));
            beam.insert(QStringLiteral("fieldAngle"),       attrD(g, "FieldAngle"));
            gm.insert(QStringLiteral("beam"), beam);
        } else if (type == QStringLiteral("Laser")) {
            QVariantMap lm;
            lm.insert(QStringLiteral("colorType"), attrStr(g, "ColorType"));
            lm.insert(QStringLiteral("emitter"),   attrStr(g, "Emitter"));
            gm.insert(QStringLiteral("laser"), lm);
        } else if (type == QStringLiteral("GeometryReference")) {
            gm.insert(QStringLiteral("linkedGeometry"), attrStr(g, "LinkedGeometry"));
        } else if (type == QStringLiteral("WiringObject")) {
            gm.insert(QStringLiteral("wiring"), QVariantMap{
                { QStringLiteral("deviceType"), attrStr(g, "DeviceType") },
                { QStringLiteral("id"),          attrStr(g, "ID") },
                { QStringLiteral("dmx_break"),   attrStr(g, "DMXBreak") }
            });
        }

        gm.insert(QStringLiteral("children"), parseGeometryChildren(g));
        out.append(gm);
    }
    return out;
}

void GDTFParser::parseGeometries(const XMLElement *node)
{
    m_result.insert(QStringLiteral("geometries"), parseGeometryChildren(node));
}

// ------------------------------------------------------------------ DMXModes

void GDTFParser::parseDmxModes(const XMLElement *node)
{
    QVariantList modes;
    for (const XMLElement *m = node->FirstChildElement("DMXMode"); m;
         m = m->NextSiblingElement("DMXMode")) {
        QVariantMap mm;
        mm.insert(QStringLiteral("name"),        attrStr(m, "Name"));
        mm.insert(QStringLiteral("description"), attrStr(m, "Description"));
        mm.insert(QStringLiteral("geometry"),    attrStr(m, "Geometry"));

        QVariantList channels;
        QVariantList relations;
        int footprint = 0;

        for (const XMLElement *e = m->FirstChildElement(); e; e = e->NextSiblingElement()) {
            if (qstrcmp(e->Name(), "DMXChannels") == 0) {
                for (const XMLElement *c = e->FirstChildElement("DMXChannel"); c;
                     c = c->NextSiblingElement("DMXChannel")) {
                    QVariantMap cm;
                    parseDmxChannel(c, cm);
                    channels.append(cm);
                }
            } else if (qstrcmp(e->Name(), "Relations") == 0) {
                for (const XMLElement *r = e->FirstChildElement("Relation"); r;
                     r = r->NextSiblingElement("Relation")) {
                    QVariantMap rm;
                    rm.insert(QStringLiteral("name"),     attrStr(r, "Name"));
                    rm.insert(QStringLiteral("master"),   attrStr(r, "Master"));
                    rm.insert(QStringLiteral("follower"), attrStr(r, "Follower"));
                    rm.insert(QStringLiteral("type"),     attrStr(r, "Type"));
                    relations.append(rm);
                }
            }
            // FTMacros：厂商私有宏，暂不展开
        }

        // 通道名 = Geometry + "_" + 主 Attribute（规范：XML 中无 Name 属性）；
        // footprint = offsets 最大值 + 1（相对 break 偏移；"None" 不占地址）
        for (int ci = 0; ci < channels.size(); ++ci) {
            QVariantMap cv = channels[ci].toMap();
            cv.insert(QStringLiteral("name"),
                      cv.value(QStringLiteral("geometry")).toString()
                          + QLatin1Char('_')
                          + cv.value(QStringLiteral("attribute")).toString());
            channels[ci] = cv;
            const QVariantList offsets = cv.value(QStringLiteral("offsets")).toList();
            for (const QVariant &o : offsets)
                footprint = qMax(footprint, o.toInt() + 1);
        }

        mm.insert(QStringLiteral("channels"),     channels);
        mm.insert(QStringLiteral("relations"),    relations);
        mm.insert(QStringLiteral("channelCount"), channels.size());
        mm.insert(QStringLiteral("footprint"),    footprint);
        modes.append(mm);
    }
    m_result.insert(QStringLiteral("dmxModes"), modes);
}

void GDTFParser::parseDmxChannel(const XMLElement *element, QVariantMap &out)
{
    // DMXBreak：1 起始；"Overwrite" = 由 GeometryReference 覆写，这里保留原值
    int dmxBreak = 1;
    {
        const char *b = element->Attribute("DMXBreak");
        if (b && qstricmp(b, "Overwrite") != 0) {
            bool ok = false;
            const int v = QString::fromUtf8(b).toInt(&ok);
            if (ok) dmxBreak = v;
        }
    }
    out.insert(QStringLiteral("dmxBreak"), dmxBreak);
    out.insert(QStringLiteral("offsets"),  parseIntArray(attrStr(element, "Offset", "None")));
    out.insert(QStringLiteral("geometry"), attrStr(element, "Geometry"));
    out.insert(QStringLiteral("highlight"),
               parseDmxValueMap(attrStr(element, "Highlight", "None")));

    QVariantList logicals;
    for (const XMLElement *lc = element->FirstChildElement("LogicalChannel"); lc;
         lc = lc->NextSiblingElement("LogicalChannel")) {
        QVariantMap lm;
        parseLogicalChannel(lc, lm);
        logicals.append(lm);
    }
    out.insert(QStringLiteral("logicalChannels"), logicals);

    // 规范：首个 LogicalChannel 的 Attribute 即本 DMXChannel 的主属性
    if (!logicals.isEmpty())
        out.insert(QStringLiteral("attribute"),
                   logicals.first().toMap().value(QStringLiteral("attribute")));

    // Default / Highlight 也可能在 ChannelFunction 上，这里仅取通道级
    out.insert(QStringLiteral("default"),
               parseDmxValueMap(attrStr(element, "Default", "None")));
}

void GDTFParser::parseLogicalChannel(const XMLElement *element, QVariantMap &out)
{
    out.insert(QStringLiteral("attribute"),       attrStr(element, "Attribute"));
    out.insert(QStringLiteral("snap"),            attrStr(element, "Snap", "No"));
    out.insert(QStringLiteral("master"),          attrStr(element, "Master", "None"));
    out.insert(QStringLiteral("mibFade"),         attrD(element, "MibFade"));
    out.insert(QStringLiteral("dmxChangeTimeLimit"), attrD(element, "DMXChangeTimeLimit"));

    QVariantList functions;
    for (const XMLElement *cf = element->FirstChildElement("ChannelFunction"); cf;
         cf = cf->NextSiblingElement("ChannelFunction")) {
        QVariantMap fm;
        parseChannelFunction(cf, fm);
        functions.append(fm);
    }
    out.insert(QStringLiteral("functions"), functions);
}

void GDTFParser::parseChannelFunction(const XMLElement *element, QVariantMap &out)
{
    out.insert(QStringLiteral("name"),              attrStr(element, "Name"));
    out.insert(QStringLiteral("attribute"),         attrStr(element, "Attribute"));
    out.insert(QStringLiteral("originalAttribute"), attrStr(element, "OriginalAttribute"));
    out.insert(QStringLiteral("customName"),        attrStr(element, "CustomName"));
    out.insert(QStringLiteral("dmxFrom"),
               parseDmxValueMap(attrStr(element, "DMXFrom", "0/1")));
    out.insert(QStringLiteral("default"),
               parseDmxValueMap(attrStr(element, "Default", "0/1")));
    out.insert(QStringLiteral("physicalFrom"),      attrD(element, "PhysicalFrom", 0.0));
    out.insert(QStringLiteral("physicalTo"),        attrD(element, "PhysicalTo", 1.0));
    out.insert(QStringLiteral("realFade"),          attrD(element, "RealFade"));
    out.insert(QStringLiteral("realAcceleration"),  attrD(element, "RealAcceleration"));
    out.insert(QStringLiteral("wheel"),             attrStr(element, "Wheel"));
    out.insert(QStringLiteral("emitter"),           attrStr(element, "Emitter"));
    out.insert(QStringLiteral("modeMaster"),        attrStr(element, "ModeMaster"));

    QVariantList sets;
    for (const XMLElement *cs = element->FirstChildElement("ChannelSet"); cs;
         cs = cs->NextSiblingElement("ChannelSet")) {
        QVariantMap sm;
        sm.insert(QStringLiteral("name"),    attrStr(cs, "Name"));
        sm.insert(QStringLiteral("dmxFrom"), parseDmxValueMap(attrStr(cs, "DMXFrom", "0/1")));
        sm.insert(QStringLiteral("physicalFrom"), attrD(cs, "PhysicalFrom", 0.0));
        sm.insert(QStringLiteral("physicalTo"),   attrD(cs, "PhysicalTo", 0.0));
        const int wsi = attrI(cs, "WheelSlotIndex", -1);
        if (wsi > 0) sm.insert(QStringLiteral("wheelSlotIndex"), wsi);
        sets.append(sm);
    }
    out.insert(QStringLiteral("sets"), sets);
}

// ------------------------------------------------------------------ Revisions

void GDTFParser::parseRevisions(const XMLElement *node)
{
    QVariantList revisions;
    for (const XMLElement *r = node->FirstChildElement("Revision"); r;
         r = r->NextSiblingElement("Revision")) {
        QVariantMap rm;
        rm.insert(QStringLiteral("text"),       attrStr(r, "Text"));
        rm.insert(QStringLiteral("date"),       attrStr(r, "Date"));
        rm.insert(QStringLiteral("modifiedBy"), attrStr(r, "ModifiedBy"));
        rm.insert(QStringLiteral("userId"),     attrStr(r, "UserID"));
        revisions.append(rm);
    }
    m_result.insert(QStringLiteral("revisions"), revisions);
}

} // namespace Nodes
