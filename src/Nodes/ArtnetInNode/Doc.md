# ArtnetInNode

## 1. 节点说明

从局域网接收 Art-Net / DMX 数据，并输出为可在流程中继续使用的变量数据。可指定 Universe、按通道过滤，也支持通过外部命令或输入端口动态修改这些参数。

## 2. 端口说明

### 输入

| 端口 | 类型 | 说明 |
|------|------|------|
| UNIVERSE | VariableData | Universe 编号（整数） |
| CHANNELS | VariableData | 通道过滤列表，如 `1,2,5-10`；留空表示不过滤具体通道 |
| FILTER | VariableData | 是否启用通道过滤（布尔） |

### 输出

| 端口 | 类型 | 说明 |
|------|------|------|
| OUTPUT | VariableData | 接收到的数据。未过滤时含 `universe`、`host`、`default` 等；过滤后含 `universe`、`host` 及指定通道键值 |

## 3. 界面说明

本节点无可嵌入界面，参数通过输入端口或外部控制设置。

## 4. 使用说明

1. 将节点放入流程，默认监听全部 Art-Net 数据（FILTER 为 false）。
2. 若只关心某个 Universe：向 UNIVERSE 写入编号，或将 FILTER 设为 true 并填写 CHANNELS。
3. 通道号为 0～512，与 DMX 字节下标一致；支持逗号与范围，如 `1,2,5-10`。
4. 外部控制：`/universe`、`/channels`、`/filter`。

## 5. 示例

只取 Universe 0 的通道 1、2、10：

- UNIVERSE = `0`
- CHANNELS = `1,2,10`
- FILTER = `true`

将 OUTPUT 接到需要 DMX 数值的下游节点即可。
