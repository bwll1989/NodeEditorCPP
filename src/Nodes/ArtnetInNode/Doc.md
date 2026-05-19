# ArtnetInNode 使用说明

## 用途
ArtnetInNode 用于从网络接收 Art-Net / DMX 数据，并输出为一份可在流程中继续处理的变量数据。

## 输入端口
- UNIVERSE：要监听的 Universe 编号（整数）。
- CHANNELS：需要过滤的通道列表（字符串，可为空）。示例：`1,2,5-10`。
- FILTER：是否启用通道过滤（布尔）。

## 输出端口
- OUTPUT：接收到的数据（VariableData）。
  - 未开启过滤：输出上游完整数据（包含 `universe`、`host`、`default` 等字段）。
  - 开启过滤：输出一个简化对象，包含 `universe`、`host`，以及你指定通道的键值对。

## 外部控制（OSC）
- `/universe`：设置 Universe（int）。
- `/channels`：设置通道列表（string）。
- `/filter`：开关过滤（bool）。

## 快速上手
1. 将 ArtnetInNode 放入流程。
2. 直接连接 OUTPUT 到下游节点即可查看数据；默认不过滤。
3. 若只关心部分通道：
   - 设置 FILTER 为 true。
   - 在 CHANNELS 填写通道列表，例如 `1,2,5-10`。

## 通道号说明
- 通道列表使用“索引”方式过滤，范围为 `0~512`（与接收到的 DMX 数据字节序列下标一致）。

## 常见问题
- 没有输出：确认网络中确实有 Art-Net 数据；Universe 过滤条件是否匹配。
- 过滤后没有字段：确认 CHANNELS 不为空，且通道号在有效范围内。