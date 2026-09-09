#pragma once

/**
 * @file SyncPairHub.hpp
 * @brief 本机口数同步：Out.port 与 In.port 相同则对齐口数。
 */

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QPointer>

namespace Nodes {

class SyncOutDataModel;
class SyncInDataModel;

class SyncPairHub : public QObject
{
    Q_OBJECT
public:
    static SyncPairHub *instance();

    void registerOut(SyncOutDataModel *out);
    void unregisterOut(SyncOutDataModel *out);
    void registerIn(SyncInDataModel *in);
    void unregisterIn(SyncInDataModel *in);

    /** Out 口数/监听端口变化时，推给 port 相同的本机 In */
    void syncPortCountFromOut(SyncOutDataModel *out);

    /** In 的 port 变化时，从监听 port 相同的本机 Out 拉口数 */
    void syncPortCountForListenPort(int listenPort);

private:
    explicit SyncPairHub(QObject *parent = nullptr);

    QList<QPointer<SyncOutDataModel>> _outs;
    QList<QPointer<SyncInDataModel>> _ins;
};

} // namespace Nodes
