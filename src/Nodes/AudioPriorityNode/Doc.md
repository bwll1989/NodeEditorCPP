# Audio Priority 节点

## 1. 节点说明

优先级混音：High priority 有信号时压低或切换 Low priority，适合「解说优先于 BGM」类场景。

## 2. 端口说明

### 输入

- **Low priority**（AudioData）：低优先级音源（如背景音乐）。
- **High priority**（AudioData）：高优先级音源（如解说、报警）。

### 输出

- **Out**（AudioData）：按优先级混合后的音频。

## 3. 界面说明

优先级相关参数（阈值、保持时间等，以节点界面为准）。

## 4. 使用说明

1. BGM 接 Low priority，话筒/提示音接 High priority。
2. 调节参数使高优先级出现时自动让路。
3. Out 接主输出。

## 5. 示例

展厅循环 BGM + 讲解触发音 → Audio Priority → 声卡，讲解时 BGM 自动减弱。
