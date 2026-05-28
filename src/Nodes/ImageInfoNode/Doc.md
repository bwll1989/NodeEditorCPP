# Image Info 节点

## 1. 节点说明

查看输入图像的宽度、高度、通道数等信息，并原样透传图像。属性信息以 VariableData 形式从第二路输出，便于做条件判断或调试。

## 2. 端口说明

### 输入

- **INPUT 0**（ImageData）：待分析的图像。

### 输出

- **OUTPUT 0**（ImageData）：与输入相同的图像（透传）。
- **OUTPUT 1**（VariableData）：图像属性，常见字段包括 `width`、`height`、`channels`；输入为空时仅有 `isNull=true`。

## 3. 界面说明

节点内嵌属性树，连接图像后会显示当前图像的各项属性。

## 4. 使用说明

1. 将图像源接到 INPUT 0。
2. 需要继续处理图像时，使用 OUTPUT 0。
3. 需要读取宽高等数值时，将 OUTPUT 1 接到 Extract、Condition、Switch 等节点。

## 5. 示例

摄像头 → Image Info → OUTPUT 1 → Condition（判断 width 是否达到 1920）→ 控制下游逻辑。
