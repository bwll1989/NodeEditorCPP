# Audio CrossFader 节点

## 1. 节点说明

在两路音频 A、B 之间做交叉淡入淡出（Crossfade），用 mix 参数控制比例。

## 2. 端口说明

### 输入

- **A**（AudioData）：音源 A。
- **B**（AudioData）：音源 B。

### 输出

- **Out**（AudioData）：混合结果。

## 3. 界面说明

Mix 滑块或旋钮：0 为全 A，1 为全 B，中间为过渡。

## 4. 使用说明

1. 两路音源分别接 A、B。
2. 拖动 mix 或外部控制实现平滑切换。
3. Out 接主输出。

## 5. 示例

曲目 A / 曲目 B → CrossFader；TimeLine 驱动 mix → 自动淡入下一曲。
