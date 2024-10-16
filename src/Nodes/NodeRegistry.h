//
// Created by 吴斌 on 2023/11/22.
//

#ifndef NODEEDITORCPP_NODEREGISTRY_H
#define NODEEDITORCPP_NODEREGISTRY_H
#include "QtNodes/NodeDelegateModelRegistry"
//#include "QtNodes/DataFlowGraphModel"
using QtNodes::NodeDelegateModelRegistry;
//using QtNodes::DataFlowGraphModel;
std::shared_ptr<NodeDelegateModelRegistry> registerDataModels();

#endif //NODEEDITORCPP_NODEREGISTRY_H
