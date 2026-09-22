# Audio List Decoder

## 1. 节点说明

独立插件：按播放列表顺序解码。每行可改文件与增益；右上角「添加」新增一条。Index 仅表示 PLAY 目标序号。

## 2. 端口

**输入（4）：** PLAY / INDEX / LOOP / LIST_LOOP  
**输出（可编辑）：** CH 0、CH 1…

| 输入 | 行为 |
|------|------|
| PLAY | **bool**：`true` 按 Index 播放，`false` 停止 |
| INDEX | 只设定 PLAY 目标序号 |
| LOOP | 单曲循环 |
| LIST_LOOP | 列表循环 |

## 3. 界面（QListView）

| 区域 | 说明 |
|------|------|
| Index / Play·Stop / 循环 | 单一勾选按钮：勾选=播放，取消=停止 |
| 列表标题行 | 左侧「播放列表」，右侧「添加」 |
| 曲目行 | `↑` `↓` + 文件选择（…）+ 增益 dB + `×`；双击播放（不改 Index） |

## 4. OSC

| 地址 | 说明 |
|------|------|
| `/play` | bool，true 播放 / false 停止 |
| `/index` | PLAY 目标序号 |
| `/loop` `/listLoop` | 循环模式 |
