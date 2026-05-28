# Fmod Node 节点

## 1. 节点说明

加载 FMOD Bank 并播放其中的事件，最多 12 路音频输出。适合游戏音效、互动装置等由 FMOD Studio 导出的内容。

## 2. 端口说明

### 输入

- **Event**（VariableData）：事件路径字符串，用于触发播放。
- **Index**（VariableData）：按索引选择事件。

### 输出

- **Out 1**～**Out 12**（AudioData）：FMOD 各总线/声道输出。

## 3. 界面说明

- Bank 目录路径输入框。
- 事件列表（加载 Bank 后刷新），选择后播放。

## 4. 使用说明

1. 填写 Bank 文件夹路径（`/file` 或界面）。
2. 等待事件列表加载，选择事件或通过 `/event`、Event 端口触发。
3. 将 Out n 接到混音或声卡。

## 5. 示例

OSC → Event 端口 → Fmod Node → Audio Matrix，现场按曲目触发 FMOD 事件。
