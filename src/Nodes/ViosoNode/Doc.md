# Vioso

## 1. 节点说明

使用 [VIOSO WarpBlend API](https://github.com/vioso/VIOSO_API) 加载 `.vwf`，对输入图像做 Warp + Blend。按文件内通道数自动展开多路输出（单机即为 1 路）。

依赖：`3rdParty/VIOSO_API`（构建时拷贝 `VIOSOWarpBlend64.dll` 到 `bin/`）。

## 2. 端口说明

### 输入

- **IMAGE**：待变形源图像。

### 输出

- **IMAGE** / **IMAGE 0…N-1**：N = `.vwf` 通道数（上限 64）；每路对应一个投影机。

## 3. 界面说明

| 控件 | 说明 |
|------|------|
| 文件选择 | 媒体库 Vioso 分类中的 `.vwf` |
| Passkey | 加密 `.vwf` 的 AES-128 密钥：32 位十六进制，或 16 字节明文；空=不加密 |
| Reload | 重新加载校准并按文件重算输出口数 |

## 4. 实现要点

```
IMAGE → 输入翻 V → 各通道 VWB_render → 输出翻 V → IMAGE[i]
```

- 翻转在 `runGl` 外侧；失败路径勿在回调内析构带 RAII 的纹理。
- 勿传 `VWB_STATEMASK_VIEWPORT`。
- 动态口：`portsAboutToBeInserted/Deleted`；`load` 时 `notifyPorts=false`。

## 5. 使用说明
![{438AC022-B4D2-4676-9202-E6692C88DB92}.png](%7B438AC022-B4D2-4676-9202-E6692C88DB92%7D.png)
1. 导入 `.vwf` 到媒体库（Vioso 分类），节点上选择。
2. 若 VIOSO 日志报 `Passkey missing`，向 VIOSO 索取 16 字节密钥并填入 Passkey（常用 32 位 hex），再点 Reload。
3. IMAGE 接源；各输出口分别接 Display / Spout 等。
4. 校准更新后点 Reload。
