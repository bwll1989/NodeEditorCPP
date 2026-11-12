#pragma once

// open62541 类型头必须先于 Qt 头包含（winsock2 冲突）
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif
#include <open62541/types.h>

#include <QtCore/QVariant>
#include <QtCore/QVariantList>
#include <QtCore/QDateTime>
#include <QtCore/QMetaType>

// Client/Server 两个节点共用的 UA_Variant ↔ QVariant 转换工具
namespace Nodes::OpcUaValue
{
    inline QVariant scalarToQVariant(const void *data, const int typeKind)
    {
        switch (typeKind) {
        case UA_TYPES_BOOLEAN:  return QVariant(*static_cast<const UA_Boolean *>(data));
        case UA_TYPES_SBYTE:    return QVariant(static_cast<int>(*static_cast<const UA_SByte *>(data)));
        case UA_TYPES_BYTE:     return QVariant(static_cast<int>(*static_cast<const UA_Byte *>(data)));
        case UA_TYPES_INT16:    return QVariant(static_cast<int>(*static_cast<const UA_Int16 *>(data)));
        case UA_TYPES_UINT16:   return QVariant(static_cast<int>(*static_cast<const UA_UInt16 *>(data)));
        case UA_TYPES_INT32:    return QVariant(static_cast<qlonglong>(*static_cast<const UA_Int32 *>(data)));
        case UA_TYPES_UINT32:   return QVariant(static_cast<qlonglong>(*static_cast<const UA_UInt32 *>(data)));
        case UA_TYPES_INT64:    return QVariant(static_cast<qlonglong>(*static_cast<const UA_Int64 *>(data)));
        case UA_TYPES_UINT64:   return QVariant(static_cast<qulonglong>(*static_cast<const UA_UInt64 *>(data)));
        case UA_TYPES_FLOAT:    return QVariant(static_cast<double>(*static_cast<const UA_Float *>(data)));
        case UA_TYPES_DOUBLE:   return QVariant(*static_cast<const UA_Double *>(data));
        case UA_TYPES_STRING: {
            const UA_String *s = static_cast<const UA_String *>(data);
            return QVariant(QString::fromUtf8(reinterpret_cast<const char *>(s->data), static_cast<int>(s->length)));
        }
        case UA_TYPES_LOCALIZEDTEXT: {
            const UA_LocalizedText *lt = static_cast<const UA_LocalizedText *>(data);
            return QVariant(QString::fromUtf8(reinterpret_cast<const char *>(lt->text.data),
                                              static_cast<int>(lt->text.length)));
        }
        case UA_TYPES_DATETIME: {
            const UA_DateTime *dt = static_cast<const UA_DateTime *>(data);
            // UA_DateTime: 100ns since 1601-01-01 UTC → Qt msecs since epoch
            const qint64 epochMs = (*dt - UA_DATETIME_UNIX_EPOCH) / UA_DATETIME_MSEC;
            return QVariant(QDateTime::fromMSecsSinceEpoch(epochMs));
        }
        default: return QVariant();
        }
    }

    inline QVariant uaVariantToQVariant(const UA_Variant &v)
    {
        if (!v.type) return QVariant();
        if (UA_Variant_isScalar(&v)) return scalarToQVariant(v.data, v.type->typeKind);
        QVariantList list;
        const char *base = static_cast<const char *>(v.data);
        for (size_t i = 0; i < v.arrayLength; ++i) {
            list.append(scalarToQVariant(base + i * v.type->memSize, v.type->typeKind));
        }
        return list;
    }

    inline bool qVariantToUaVariant(const QVariant &in, UA_Variant &out)
    {
        switch (in.typeId()) {
        case QMetaType::Bool: {
            UA_Boolean *b = UA_Boolean_new();
            *b = in.toBool();
            UA_Variant_setScalar(&out, b, &UA_TYPES[UA_TYPES_BOOLEAN]);
            return true;
        }
        case QMetaType::Int:
        case QMetaType::Long:
        case QMetaType::LongLong: {
            const qlonglong v64 = in.toLongLong();
            if (v64 >= INT32_MIN && v64 <= INT32_MAX) {
                UA_Int32 *n = UA_Int32_new();
                *n = static_cast<UA_Int32>(v64);
                UA_Variant_setScalar(&out, n, &UA_TYPES[UA_TYPES_INT32]);
            } else {
                UA_Int64 *n = UA_Int64_new();
                *n = static_cast<UA_Int64>(v64);
                UA_Variant_setScalar(&out, n, &UA_TYPES[UA_TYPES_INT64]);
            }
            return true;
        }
        case QMetaType::UInt:
        case QMetaType::ULong:
        case QMetaType::ULongLong: {
            const qulonglong v64 = in.toULongLong();
            if (v64 <= UINT32_MAX) {
                UA_UInt32 *n = UA_UInt32_new();
                *n = static_cast<UA_UInt32>(v64);
                UA_Variant_setScalar(&out, n, &UA_TYPES[UA_TYPES_UINT32]);
            } else {
                UA_UInt64 *n = UA_UInt64_new();
                *n = static_cast<UA_UInt64>(v64);
                UA_Variant_setScalar(&out, n, &UA_TYPES[UA_TYPES_UINT64]);
            }
            return true;
        }
        case QMetaType::Float:
        case QMetaType::Double: {
            UA_Double *d = UA_Double_new();
            *d = in.toDouble();
            UA_Variant_setScalar(&out, d, &UA_TYPES[UA_TYPES_DOUBLE]);
            return true;
        }
        case QMetaType::QString: {
            const QByteArray ba = in.toString().toUtf8();
            UA_String *s = UA_String_new();
            *s = UA_String_fromChars(ba.constData());
            UA_Variant_setScalar(&out, s, &UA_TYPES[UA_TYPES_STRING]);
            return true;
        }
        case QMetaType::QVariantList: {
            const QVariantList list = in.toList();
            if (list.isEmpty()) return false;
            // 按首元素类型统一成数组：数值→Double 数组，字符串→String 数组
            if (list.first().typeId() == QMetaType::QString) {
                QVector<UA_String> arr;
                arr.reserve(list.size());
                for (const QVariant &e : list) {
                    const QByteArray ba = e.toString().toUtf8();
                    arr.append(UA_String_fromChars(ba.constData()));
                }
                UA_Variant_setArrayCopy(&out, arr.constData(), arr.size(), &UA_TYPES[UA_TYPES_STRING]);
                return true;
            }
            QVector<UA_Double> arr;
            arr.reserve(list.size());
            for (const QVariant &e : list) arr.append(e.toDouble());
            UA_Variant_setArrayCopy(&out, arr.constData(), arr.size(), &UA_TYPES[UA_TYPES_DOUBLE]);
            return true;
        }
        default:
            return false;
        }
    }
} // namespace Nodes::OpcUaValue
