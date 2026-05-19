# RectNode 使用说明

## 用途
生成/拆分矩形数据：输入 x/y/width/height，输出 Rect、Size、Center 等信息，供图像裁切、布局、区域计算等下游节点使用。

## 端口
### 输入（VariableData）
- POS_X：x
- POS_Y：y
- WIDTH：宽
- HEIGHT：高

### 输出（VariableData）
- Rect：QRectF
- SIZE：QSizeF
- CENTER：QPointF
- BOTTOM_RIGHT：QPointF

## 外部控制（可选）
- /x（double）
- /y（double）
- /width（double）
- /height（double）

## 使用步骤
1. 连接数值到 POS_X/POS_Y/WIDTH/HEIGHT（或用外部控制设置 /x /y /width /height）。
2. 按需连接输出：
   - Rect：用于需要 QRect 的节点
   - SIZE：用于需要尺寸的节点
   - CENTER/BOTTOM_RIGHT：用于锚点/对齐计算

## 注意事项
- 输出为 VariableData 承载的 Qt 几何类型；下游若只接受数值，需再做拆分/提取。 

