/**
 * @file SyncPairHub.cpp
 */

#include "SyncPairHub.hpp"
#include "SyncOutDataModel.hpp"
#include "SyncInDataModel.hpp"

namespace Nodes {

SyncPairHub *SyncPairHub::instance()
{
    static SyncPairHub hub;
    return &hub;
}

SyncPairHub::SyncPairHub(QObject *parent)
    : QObject(parent)
{
}

void SyncPairHub::registerOut(SyncOutDataModel *out)
{
    if (!out)
        return;
    for (auto const &p : _outs) {
        if (p.data() == out)
            return;
    }
    _outs.append(out);
}

void SyncPairHub::unregisterOut(SyncOutDataModel *out)
{
    for (int i = _outs.size() - 1; i >= 0; --i) {
        if (!_outs.at(i) || _outs.at(i).data() == out)
            _outs.removeAt(i);
    }
}

void SyncPairHub::registerIn(SyncInDataModel *in)
{
    if (!in)
        return;
    for (auto const &p : _ins) {
        if (p.data() == in)
            return;
    }
    _ins.append(in);
}

void SyncPairHub::unregisterIn(SyncInDataModel *in)
{
    for (int i = _ins.size() - 1; i >= 0; --i) {
        if (!_ins.at(i) || _ins.at(i).data() == in)
            _ins.removeAt(i);
    }
}

void SyncPairHub::syncPortCountFromOut(SyncOutDataModel *out)
{
    if (!out || !out->active())
        return;
    const int port = out->port();
    const unsigned n = out->nPorts(PortType::In);
    for (auto const &in : _ins) {
        if (!in || in->port() != port)
            continue;
        in->applyPortCountFromPair(n);
    }
}

void SyncPairHub::syncPortCountForListenPort(int listenPort)
{
    if (listenPort <= 0)
        return;
    SyncOutDataModel *source = nullptr;
    for (auto const &out : _outs) {
        if (out && out->active() && out->port() == listenPort) {
            source = out.data();
            break;
        }
    }
    if (!source)
        return;
    const unsigned n = source->nPorts(PortType::In);
    for (auto const &in : _ins) {
        if (!in || in->port() != listenPort)
            continue;
        in->applyPortCountFromPair(n);
    }
}

} // namespace Nodes
