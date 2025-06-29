#include "QtNodes/ConnectionStyle"
#include "QtNodes/NodeStyle"
#include <QtNodes/GraphicsViewStyle>
using QtNodes::GraphicsViewStyle;
using QtNodes::ConnectionStyle;
using QtNodes::NodeStyle;
using QtNodes::GroupStyle;
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

      "LineWidth": 3.0,
      "ConstructionLineWidth": 2.0,
      "PointDiameter": 8.0,

      "UseDataDefinedColors": true,
      "InArrow" : true,
       "OutArrow" : false
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
    "FontColor" : [53, 53, 53],
    "FontColorFaded" : "gray",
    "ConnectionPointColor": [169, 169, 169],
    "FilledConnectionPointColor": "cyan",
    "ErrorColor": "red",
    "WarningColor": [128, 128, 0],

    "PenWidth": 1.0,
    "HoveredPenWidth": 2,

    "ConnectionPointDiameter": 10.0,

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
  GroupStyle::setGroupStyle(
            R"(
  {
    "NormalBoundaryColor": [255, 255, 255],
    "FontColor" : [53, 53, 53],
    "NormalColor": [255, 255, 255],
    "SelectedColor": [255, 165, 0],
    "SelectedHaloColor": "orange",
    "CaptionColor": "darkcyan",
    "Opacity": 0.8,
    "PenWidth": 1.0,
    "HoveredPenWidth": 1.5,
    "GradientColor0": "gray",
    "CaptionHeight": 30.0,
    "GradientColor1": [80, 80, 80],
    "GradientColor2": [64, 64, 64],
    "GradientColor3": [58, 58, 58],
    "UseDataDefinedColors": false
  })");
}
