# Audio Ducking 节点

## 1. 节点说明

侧链压限：当 Sidechain（如话筒、解说）有信号时，自动降低 Music（背景音乐）电平，常用于自动闪避。

## 2. 端口说明

### 输入

- **Music**（AudioData）：被压低的主音乐/背景。
- **Sidechain**（AudioData）：触发压限的参考信号（如人声）。

### 输出

- **Out**（AudioData）：处理后的音乐。

## 3. 界面说明

阈值、比率、启动/释放时间等压限参数（依界面控件为准）。

## 4. 使用说明

1. 背景音乐接 Music，话筒接 Sidechain。
2. 调节阈值等，使人声出现时 BGM 自动变小。
3. Out 接 Audio Device Out 或录像链路。

## 5. 示例

BGM 播放器 → Music；主持麦 → Sidechain → Ducking → 主混音 LR。
