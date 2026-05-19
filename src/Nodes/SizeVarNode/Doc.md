# SizeVarNode 使用说明

## 用途
把宽高两个数值组合为一个 Size（QSizeF），便于在下游节点中作为“尺寸”参数使用。

## 端口
### 输入（VariableData）
- Width：宽
- Height：高

### 输出（VariableData）
- SIZE：QSizeF

## 外部控制（可选）
- /width（double）
- /height（double）

## 使用步骤
1. 将数值连接到 Width/Height（或用外部控制设置 /width /height）。
2. 将 SIZE 输出连接到需要 QSize/QSizeF 的下游节点。 

