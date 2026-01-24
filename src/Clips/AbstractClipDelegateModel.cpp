//
// Created by WuBin on 2025/12/30.
//

#include "AbstractClipDelegateModel.h"
AbstractClipDelegateModel::AbstractClipDelegateModel(int start, const QString &filePath, QObject *parent): AbstractClipModel(start, filePath, parent) {
    // 函数级注释：构造剪辑委托模型，初始化 OSC 连接代理对象
    _oscConnectionProxy = new QObject(this);
}

AbstractClipDelegateModel::~AbstractClipDelegateModel() {
    // 函数级注释：析构时自动销毁连接代理，并由事件总线负责清理订阅
}
