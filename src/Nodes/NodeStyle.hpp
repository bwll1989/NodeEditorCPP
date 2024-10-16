#include "QtNodes/ConnectionStyle"
#include "QtNodes/NodeStyle"
#include <QtNodes/GraphicsViewStyle>
using QtNodes::GraphicsViewStyle;
using QtNodes::ConnectionStyle;
using QtNodes::NodeStyle;
static void setStyle()
{
    ConnectionStyle::setConnectionStyle(
            R"(
  {
    "ConnectionStyle": {
      "ConstructionColor": "gray",
      "NormalColor": "black",
      "SelectedColor": "gray",
      "SelectedHaloColor": "deepskyblue",
      "HoveredColor": "deepskyblue",

      "LineWidth": 2.0,
      "ConstructionLineWidth": 1.0,
      "PointDiameter": 2.0,

      "UseDataDefinedColors": true
    }
  }
  )");
    NodeStyle::setNodeStyle(
            R"(
  {
    "NodeStyle": {
    "NormalBoundaryColor": [255, 255, 255],
    "SelectedBoundaryColor": [255, 165, 0],

    "GradientColor0": "gray",
    "GradientColor1": [80, 80, 80],
    "GradientColor2": [64, 64, 64],
    "GradientColor3": [58, 58, 58],
    "ShadowColor": [20, 20, 20],
    "ShadowEnabled": false,
    "FontColor" : "white",
    "FontColorFaded" : "gray",
    "ConnectionPointColor": [169, 169, 169],
    "FilledConnectionPointColor": "cyan",
    "ErrorColor": "red",
    "WarningColor": [128, 128, 0],

    "PenWidth": 1.0,
    "HoveredPenWidth": 2,

    "ConnectionPointDiameter": 8.0,

    "Opacity": 0.95
  }
  }
  )");
    GraphicsViewStyle::setStyle(
            R"(
  {
    "GraphicsViewStyle": {
      "BackgroundColor": [53, 53, 53],
    "FineGridColor": [60, 60, 60],
    "CoarseGridColor": [25, 25, 25]
    }
  }
  )");
}
