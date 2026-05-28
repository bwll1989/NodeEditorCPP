# NDI Out 节点

## 1. 节点说明

将图像以 NDI 流形式发送到网络，供 vMix、OBS、其他 NDI 接收端使用。

## 2. 端口说明

### 输入

- **IMAGE**（ImageData）：要发送的视频帧。
- **ENABLE**（VariableData）：是否发送（布尔）。

### 输出

无输出端口。

## 3. 界面说明

- NDI 发送名称。
- 开始/停止发送及状态指示。

## 4. 使用说明

1. 填写发送名称（接收端可见的 NDI 源名）。
2. 连接 IMAGE，ENABLE 为 true 时开始推流。
3. 在接收软件中选择对应 NDI 源。

外部地址：`/enable`、`/senderName`。

## 5. 示例

合成画面 → NDI Out（「Program」）→ vMix 作为 NDI 输入。
