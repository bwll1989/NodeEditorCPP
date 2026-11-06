// 函数级注释：HPACK (RFC 7541) 编解码实现，供 GrpcChannel 在 HTTP/2 明文 (h2c) 上使用
// - 编码端只使用 literal-without-indexing + 非 Huffman 字面量（合法且无状态）
// - 解码端完整实现：索引/增量索引/动态表尺寸更新/Huffman 字符串
#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QPair>

namespace Nodes::GrpcHpack {

using Header = QPair<QByteArray, QByteArray>;

/**
 * HPACK 解码器。
 * 注意：HPACK 上下文是按连接（而非按流）共享的，解码对端发来的头块时
 * 必须复用同一个 Decoder 实例。
 */
class Decoder
{
public:
    explicit Decoder(int maxDynamicTableSize = 4096);

    // 解码一个完整头块（HEADERS + CONTINUATION 拼接后的字节序列）
    bool decode(const QByteArray &block, QList<Header> &out);

private:
    struct DynamicEntry
    {
        QByteArray name;
        QByteArray value;
        int size = 0; // len(name) + len(value) + 32
    };

    bool lookupName(quint64 index, QByteArray &name) const;
    bool lookupEntry(quint64 index, QByteArray &name, QByteArray &value) const;
    void addToDynamicTable(const QByteArray &name, const QByteArray &value);
    void evict();
    void setDynamicTableSize(quint64 size);

    QList<DynamicEntry> m_dynamic; // 首元素为最新
    int m_maxSize = 4096;
};

// 头块编码：全部使用 literal-without-indexing；名字命中静态表则用索引，
// 值一律不做 Huffman（合法且实现简单，动态表保持为空）。
QByteArray encodeHeaderBlock(const QList<Header> &headers);

// RFC 7541 Appendix B Huffman 解码；失败返回 false
bool huffmanDecode(const QByteArray &in, QByteArray &out);

} // namespace Nodes::GrpcHpack
