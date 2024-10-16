//
// Created by 吴斌 on 2023/11/22.
//
//#ifndef NODEEDITORCPP_NODEREGISTRY_H
//#define NODEEDITORCPP_NODEREGISTRY_H
//
//
//#include "QtNodes/NodeDelegateModelRegistry"
//#include "Nodes/CaculatorNode/AdditionModel.hpp"
//#include "Nodes/CaculatorNode/DivisionModel.hpp"
//#include "Nodes/CaculatorNode/MultiplicationModel.hpp"
//#include "Nodes/CaculatorNode/NumberDisplayDataModel.hpp"
//#include "Nodes/CaculatorNode/NumberSourceDataModel.hpp"
//#include "Nodes/ProjectorNode/ProjectorDataModel.hpp"
//#include "Nodes/CaculatorNode/SubtractionModel.hpp"
//#include "Nodes/ClientNode/ClientDataModel.hpp"
//#include "Nodes/ProjectNode/ProjectDataModel.hpp"
//#include "Nodes/ScreenNode/ScreenDataModel.hpp"
//#include "Nodes/TextDisplayNode/TextDisplayDataModel.hpp"
//#include "Nodes/TextSourceNode/TextSourceDataModel.hpp"
//#include "Nodes/ImageLoadNode/ImageLoaderModel.hpp"
//#include "Nodes/ImageShowNode/ImageShowModel.hpp"
////#include "Nodes/PythonNode/PythonNodeModel.hpp"
//#include "Nodes/AudioDeviceOutNode/AudioDeviceOutDataModel.hpp"
//#include "Nodes/AudioPlayNode/AudioPlayDataModel.hpp"
//#include "Nodes/AudioDeviceInNode/AudioDeviceInDataModel.hpp"
//
//using QtNodes::NodeDelegateModelRegistry;
//
//std::shared_ptr<NodeDelegateModelRegistry>  registerDataModels()
//{
//    auto ret = std::make_shared<NodeDelegateModelRegistry>();
//    ret->registerModel<NumberSourceDataModel>("输入");
//    ret->registerModel<TextDisplayDataModel>("输出");
//    ret->registerModel<TextSourceDataModel>("输入");
//    ret->registerModel<ImageLoaderModel>("输入");
//    ret->registerModel<ImageShowModel>("输出");
//    ret->registerModel<NumberDisplayDataModel>("输出");
//
//    ret->registerModel<AdditionModel>("操作");
//
//    ret->registerModel<SubtractionModel>("操作");
//
//    ret->registerModel<MultiplicationModel>("操作");
//
//    ret->registerModel<DivisionModel>("操作");
//
//    ret->registerModel<ProjectorDataModel>("Other");
//
//    ret->registerModel<ClientDataModel>("Other");
//
//    ret->registerModel<ProjectDataModel>("Other");
//
//    ret->registerModel<ScreenDataModel>("Other");
////    ret->registerModel<PythonDataModel>("逻辑运算");
//
//    ret->registerModel<AudioDeviceOutDataModel>("Audio");
//
//    ret->registerModel<AudioPlayDataModel>("Audio");
//
//    ret->registerModel<AudioDeviceInDataModel>("Audio");
//
////
//     return ret;
//}

//#endif //NODEEDITORCPP_NODEREGISTRY_H