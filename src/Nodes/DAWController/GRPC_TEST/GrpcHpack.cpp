#include "GrpcHpack.h"

#include "GrpcHuffmanTable.h"
#include "GrpcStaticTable.h"

#include <QDebug>

namespace Nodes::GrpcHpack {

namespace {

// HPACK 整数原语（RFC 7541 5.1）：prefixBits 位前缀，溢出部分续接 7bit varint
bool readInteger(const uchar *&p, const uchar *end, int prefixBits, quint8 firstByte, quint64 &out)
{
    const quint64 limit = (quint64(1) << prefixBits) - 1;
    quint64 value = firstByte & limit;
    if (value < limit) {
        out = value;
        return true;
    }
    int shift = 0;
    while (p < end) {
        const quint8 b = *p++;
        value += quint64(b & 0x7F) << shift;
        shift += 7;
        if (!(b & 0x80)) {
            out = value;
            return true;
        }
        if (shift > 56) {
            return false; // 防御：HPACK 中不会出现如此大的整数
        }
    }
    return false;
}

void appendInteger(QByteArray &out, quint8 highBits, int prefixBits, quint64 value)
{
    const quint64 limit = (quint64(1) << prefixBits) - 1;
    if (value < limit) {
        out.append(char(highBits | value));
        return;
    }
    out.append(char(highBits | limit));
    value -= limit;
    while (value >= 0x80) {
        out.append(char(0x80 | (value & 0x7F)));
        value >>= 7;
    }
    out.append(char(value));
}

// 字符串字面量读取（含 Huffman 标志位）
bool readString(const uchar *&p, const uchar *end, QByteArray &out)
{
    if (p >= end) {
        return false;
    }
    const quint8 b = *p++;
    const bool huffman = (b & 0x80) != 0;
    quint64 len = 0;
    if (!readInteger(p, end, 7, b, len)) {
        return false;
    }
    if (len > quint64(end - p)) {
        return false;
    }
    QByteArray raw(reinterpret_cast<const char *>(p), int(len));
    p += len;
    if (huffman) {
        return huffmanDecode(raw, out);
    }
    out = raw;
    return true;
}

struct HuffNode
{
    int child[2] = { -1, -1 };
    int sym = -1;
};

QList<HuffNode> buildHuffmanTree()
{
    QList<HuffNode> nodes;
    nodes.append(HuffNode{});
    for (int sym = 0; sym < 256; ++sym) {
        const auto &e = kHuffmanCodes[sym];
        int cur = 0;
        for (int i = e.bitLength - 1; i >= 0; --i) {
            const int bit = int((e.code >> i) & 1u);
            if (nodes[cur].child[bit] < 0) {
                nodes.append(HuffNode{});
                nodes[cur].child[bit] = nodes.size() - 1;
            }
            cur = nodes[cur].child[bit];
        }
        nodes[cur].sym = sym;
    }
    return nodes;
}

} // namespace

Decoder::Decoder(int maxDynamicTableSize)
    : m_maxSize(maxDynamicTableSize)
{
}

void Decoder::setDynamicTableSize(quint64 size)
{
    if (size > quint64(m_maxSize)) {
        // 我们宣告的上限即静态默认 4096；超出视为协议错误
        m_maxSize = -1; // 标记无效
        return;
    }
    m_maxSize = int(size);
    evict();
}

void Decoder::evict()
{
    int used = 0;
    for (const DynamicEntry &e : m_dynamic) {
        used += e.size;
    }
    while (used > m_maxSize && !m_dynamic.isEmpty()) {
        used -= m_dynamic.takeLast().size;
    }
}

void Decoder::addToDynamicTable(const QByteArray &name, const QByteArray &value)
{
    DynamicEntry e;
    e.name = name;
    e.value = value;
    e.size = name.size() + value.size() + 32;
    if (e.size > m_maxSize) {
        // 单条超过表上限：清空表且不插入（RFC 7541 4.4）
        m_dynamic.clear();
        return;
    }
    m_dynamic.prepend(e);
    evict();
}

bool Decoder::lookupName(quint64 index, QByteArray &name) const
{
    QByteArray value;
    return lookupEntry(index, name, value);
}

bool Decoder::lookupEntry(quint64 index, QByteArray &name, QByteArray &value) const
{
    if (index == 0) {
        return false;
    }
    if (index <= 61) {
        const auto &e = kStaticTable[index - 1];
        name = QByteArray(e.name);
        value = QByteArray(e.value);
        return true;
    }
    const int dynIndex = int(index) - 62;
    if (dynIndex >= m_dynamic.size()) {
        return false;
    }
    name = m_dynamic[dynIndex].name;
    value = m_dynamic[dynIndex].value;
    return true;
}

bool Decoder::decode(const QByteArray &block, QList<Header> &out)
{
    if (m_maxSize < 0) {
        return false; // 此前出现过非法表尺寸
    }

    const uchar *p = reinterpret_cast<const uchar *>(block.constData());
    const uchar *end = p + block.size();

    while (p < end) {
        const quint8 b = *p++;

        if (b & 0x80) {
            // 1xxxxxxx：索引字段
            quint64 idx = 0;
            if (!readInteger(p, end, 7, b, idx)) {
                return false;
            }
            Header h;
            if (!lookupEntry(idx, h.first, h.second)) {
                return false;
            }
            out.append(h);
            continue;
        }

        if ((b & 0xC0) == 0x40) {
            // 01xxxxxx：增量索引的字面量
            quint64 idx = 0;
            if (!readInteger(p, end, 6, b, idx)) {
                return false;
            }
            Header h;
            if (idx == 0) {
                if (!readString(p, end, h.first)) {
                    return false;
                }
            } else {
                QByteArray value;
                if (!lookupEntry(idx, h.first, value)) {
                    return false;
                }
            }
            if (!readString(p, end, h.second)) {
                return false;
            }
            addToDynamicTable(h.first, h.second);
            out.append(h);
            continue;
        }

        if ((b & 0xE0) == 0x20) {
            // 001xxxxx：动态表尺寸更新
            quint64 size = 0;
            if (!readInteger(p, end, 5, b, size)) {
                return false;
            }
            if (size > quint64(m_maxSize) && !m_dynamic.isEmpty()) {
                // 先按新上限收缩再判断
            }
            if (size > quint64(4096)) {
                return false; // 我们从未宣告更大的表
            }
            setDynamicTableSize(size);
            continue;
        }

        // 0000xxxx / 0001xxxx：不索引 / 永不索引的字面量
        quint64 idx = 0;
        if (!readInteger(p, end, 4, b, idx)) {
            return false;
        }
        Header h;
        if (idx == 0) {
            if (!readString(p, end, h.first)) {
                return false;
            }
        } else {
            QByteArray value;
            if (!lookupEntry(idx, h.first, value)) {
                return false;
            }
        }
        if (!readString(p, end, h.second)) {
            return false;
        }
        out.append(h);
        continue;
    }

    return true;
}

QByteArray encodeHeaderBlock(const QList<Header> &headers)
{
    static QHash<QByteArray, int> staticNameIndex;
    if (staticNameIndex.isEmpty()) {
        for (int i = 0; i < 61; ++i) {
            const QByteArray name(kStaticTable[i].name);
            if (!staticNameIndex.contains(name)) {
                staticNameIndex.insert(name, i + 1);
            }
        }
    }

    QByteArray out;
    for (const Header &h : headers) {
        const int idx = staticNameIndex.value(h.first, 0);
        if (idx > 0) {
            // literal-without-indexing + 索引名（4 位前缀）
            appendInteger(out, 0x00, 4, quint64(idx));
        } else {
            out.append(char(0x00)); // 新名字
            appendInteger(out, 0x00, 7, h.first.size());
            out.append(h.first);
        }
        appendInteger(out, 0x00, 7, h.second.size());
        out.append(h.second);
    }
    return out;
}

bool huffmanDecode(const QByteArray &in, QByteArray &out)
{
    static const QList<HuffNode> tree = buildHuffmanTree();

    out.clear();
    out.reserve(in.size() * 2);

    int cur = 0;            // 当前树节点，0 = 根
    int padBits = 0;        // 距上一个已解码符号的累计位数（尾部部分符号）
    bool padAllOnes = true; // 该尾段是否全为 1（EOS 编码前缀）

    for (uchar c : in) {
        for (int bit = 7; bit >= 0; --bit) {
            const int b = (c >> bit) & 1;
            const int next = tree[cur].child[b];
            if (next < 0) {
                return false; // 非法编码
            }
            cur = next;
            if (tree[cur].sym >= 0) {
                out.append(char(tree[cur].sym));
                cur = 0;
                padBits = 0;
                padAllOnes = true;
            } else {
                if (b != 1) {
                    padAllOnes = false;
                }
                ++padBits;
            }
        }
    }

    if (cur == 0) {
        return true; // 全部符号完整结束
    }
    // 尾部填充必须是 EOS 编码（全 1，30 位）的前缀且短于 8 位
    return padAllOnes && padBits < 8;
}

} // namespace Nodes::GrpcHpack
