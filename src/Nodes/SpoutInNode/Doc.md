# SpoutInNode 使用说明

## 用途
从其他支持 Spout 的软件接收实时画面，并输出为 ImageData。

## 端口
### 输入（VariableData）
- SOURCE：发送源名称（string）
- ENABLE：启用/停止接收（bool）

### 输出（ImageData）
- IMAGE 0：接收到的图像

## 参数/界面
- Sender：发送源下拉选择
- Start/Stop：开始/停止接收

## 外部控制（可选）
- /source（string）：设置发送源名称
- /enable（bool）：启用/停止接收

## 使用步骤
1. 确认发送端软件已开启 Spout 输出。
2. 在 SpoutInNode 里选择 Sender（或通过 /source 设置）。
3. 将 ENABLE 置为 true（或通过 /enable 启用）。
4. 把 IMAGE 0 连接到下游图像处理/显示节点。 
