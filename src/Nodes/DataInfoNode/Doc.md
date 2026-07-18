# Data Info

## 1. 节点说明

Data Info 用于**查看**流经节点的数据结构：在独立窗口中以树形方式展示 VariableData 或 ImageData 的字段，支持增量更新，便于调试数据流、确认 OSC/脚本输出格式或图像元信息。

- **VariableData**：输出 0 透传原数据，并在 View 窗口中展示全部字段。
- **ImageData**：输出 0 提供宽高等元数据（VariableData），输出 1 透传原图像；View 窗口展示 `width`、`height`、`channels`、`isNull` 等字段。

（原 Image Info 节点功能已合并至本节点。）

## 2. 端口说明

### 输入

| 端口 | 名称 | 数据类型 | 说明 |
|------|------|----------|------|
| 0 | DATA | VariableData | 可接 VariableData；也可接 ImageData（输入端口兼容任意类型） |

### 输出

| 端口 | 名称 | 数据类型 | 说明 |
|------|------|----------|------|
| 0 | DATA | VariableData | VariableData 透传；或 ImageData 对应的元数据 |
| 1 | IMAGE | ImageData | 输入为 ImageData 时透传图像，否则为空 |

## 3. 界面说明

- **View** 按钮：点击后打开置顶浮动窗口「Data Info」（约 800×400），显示当前数据的属性树。  
  窗口可关闭；关闭后再次点击 View 重新打开。  
  数据在输入更新时会**增量刷新**浏览器内容（首次为全量构建）。

节点本体仅显示按钮，不内嵌完整树视图。

## 4. 使用说明

1. 将待观察的数据流或图像源串在链路中。
2. 运行后点击 **View** 查看实时字段与值。
3. VariableData 从输出 0 继续接下游；ImageData 可从输出 1 继续处理图像，从输出 0 读取宽高等数值。

适合开发阶段；正式演出可移除或保留在旁路。

## 5. 示例

**OSC 调试：** Osc In → Data Info → Extract。在 View 中确认 `address`、`args` 等键名后再写 Extract 表达式。  
**图像尺寸判断：** 摄像头 → Data Info → 输出 0 → Condition（判断 `width` 是否达到 1920）。
