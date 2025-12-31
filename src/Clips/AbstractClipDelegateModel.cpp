//
// Created by WuBin on 2025/12/30.
//

#include "AbstractClipDelegateModel.h"
AbstractClipDelegateModel::AbstractClipDelegateModel(int start, const QString &filePath, QObject *parent): AbstractClipModel(start, filePath, parent) {
    _oscConnectionProxy = new QObject(this);
}

AbstractClipDelegateModel::~AbstractClipDelegateModel() {

}
