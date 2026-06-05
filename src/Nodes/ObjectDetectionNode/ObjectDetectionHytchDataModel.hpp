#pragma once

#include "ObjectDetectionDataModel.hpp"

namespace Nodes
{
    inline const std::vector<std::string>& hytchClassNames()
    {
        static const std::vector<std::string> names = {
            "xgpl_boat",
            "gdtlj_truck",
        };
        return names;
    }

    class ObjectDetectionHytchDataModel : public ObjectDetectionDataModel
    {
    public:
        ObjectDetectionHytchDataModel()
            : ObjectDetectionDataModel(ObjectDetectionProfile{
                  QStringLiteral("./plugins/Models/yolo26n-dection-hytch.onnx"),
                  QStringLiteral("Object Detection (Hytch)"),
                  &hytchClassNames(),
              })
        {
        }
    };
}
