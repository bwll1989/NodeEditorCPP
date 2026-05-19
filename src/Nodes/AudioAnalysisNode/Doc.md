# AudioAnalysisNode 使用说明

## 用途
AudioAnalysisNode 用于对输入音频做实时分析，并将分析结果以键值形式输出，便于做可视化、触发条件、自动化控制或录制统计。

## 能得到什么
- 音量相关：RMS、Peak 等
- 频谱相关：频谱重心、滚降点、通量、平坦度等
- 音高：Pitch（Hz）
- 频段能量：10 段能量占比/能量值（dB）与总能量
- 人声概率：VoiceProbability / InstrumentProbability

## 输入端口
- **端口类型**：AudioData
- **端口数量**：1个
- **数据格式**：支持AudioTimestampRingQueue共享缓冲区
- **音频格式**：16位有符号整数音频数据
- **采样率**：支持多种采样率（推荐48kHz）
- **帧大小**：2048采样点/帧（可配置）

## 输出端口
- **端口类型**：AnalysisData
- **端口数量**：1个
- **数据格式**：JSON格式的分析结果
- **更新频率**：实时更新（基于音频帧率）
- **数据内容**：包含所有音频特征的键值对

## 分析特征

### 时域特征
- **RMS (均方根值)**：音频信号的有效值，反映音频的整体能量
- **Peak (峰值能量)**：音频信号的峰值，用于检测瞬态信号
- **ZCR (零交叉率)**：信号穿越零点的频率，用于区分人声和乐器

### 频域特征
- **SpectralCentroid (频谱重心)**：频谱的重心位置，反映音频的"亮度"
- **SpectralRolloff (频谱滚降点)**：包含85%能量的频率点
- **SpectralFlux (频谱通量)**：频谱变化的剧烈程度
- **SpectralCrest (频谱峰值因子)**：频谱的峰值与平均值比率
- **SpectralFlatness (频谱平坦度)**：频谱的平坦程度
- **SpectralKurtosis (频谱峰度)**：频谱分布的尖锐程度

### 倒谱特征
- **MFCC_0 到 MFCC_3**：前4个梅尔频率倒谱系数
- **MelFrequencySpectrum**：梅尔频率谱

### 起始检测
- **EnergyDifference**：能量差分起始检测
- **SpectralDifference**：频谱差分起始检测
- **SpectralDifferenceHWR**：半波整流频谱差分
- **ComplexSpectralDifference**：复数频谱差分
- **HighFrequencyContent**：高频内容检测

### 音高检测
- **Pitch**：基于Yin算法的音高估计（Hz）

### 频段能量分析
- **Band1EnergyRatio 到 Band10EnergyRatio**：10个频段的能量占比
  - Band1: 0-63Hz（超低频）
  - Band2: 63-200Hz（低频）
  - Band3: 200-630Hz（中低频）
  - Band4: 630-2000Hz（中频）
  - Band5: 2000-6300Hz（中高频）
  - Band6: 6300-12500Hz（高频）
  - Band7: 12500-16000Hz（超高频）
  - Band8: 16000-18000Hz（极高频1）
  - Band9: 18000-20000Hz（极高频2）
  - Band10: >20000Hz（剩余频段）
- **Band1EnergyDB 到 Band10EnergyDB**：各频段的绝对能量值（dB）
- **TotalEnergyDB**：总能量值（dB）

### 人声检测
- **VoiceProbability**：人声概率（0-1）
- **InstrumentProbability**：乐器概率（0-1）

## 使用步骤
1. 将音频源连接到输入端口。
2. 从输出端口读取分析结果（键值对/JSON）。
3. 典型用法：将 `Rms/Peak` 等字段接到条件节点做触发，或将频段能量接到参数映射实现“跟随音乐”。

## 输出字段（常用）
- 音量：`Rms`、`Peak`
- 时域：`Zcr`
- 频谱：`SpectralCentroid`、`SpectralRolloff`、`SpectralFlux`、`SpectralFlatness` 等
- 音高：`Pitch`
- 人声：`VoiceProbability`、`InstrumentProbability`
- 频段：`Band1EnergyRatio`..`Band10EnergyRatio`、`Band1EnergyDB`..`Band10EnergyDB`、`TotalEnergyDB`

不同版本/配置下输出字段可能略有差异，以实际输出为准。

## 常见问题
- 无输出：确认输入端口已连接且音频源正在输出。
- 数值异常：确认上游音频格式/采样率正常；可先用 `Rms/Peak` 验证链路。
- 抖动较大：建议在下游做平滑（例如滑动平均）或加阈值滞回。