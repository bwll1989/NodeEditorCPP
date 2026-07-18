# Lookup

## 1. 节点说明

Value Lookup 根据输入 VariableData 在**规则表**中查找第一条匹配条件，命中后输出**输入数据的完整副本**，并将 Value 列的结果合并进输出（标量写入 `default`，对象则合并各键）。适合「条件判断 → 输出参数/表达式结果」映射，如状态码转文案、通道号转 DMX 值、按字段计算输出等。

无匹配时不推送输出。每次匹配成功会附带 `_matchedRow`（行号）与 `_pulse`（递增）。

## 2. 端口说明

### 输入

| 端口 | 名称 | 数据类型 | 说明 |
|------|------|----------|------|
| 0 | INPUT 0 | VariableData | 供 `$input` 使用的键值数据 |

### 输出

| 端口 | 名称 | 数据类型 | 说明 |
|------|------|----------|------|
| 0 | OUTPUT 0 | VariableData | 输入副本 + Value 列结果；附带 `_matchedRow`、`_pulse` |

## 3. 界面说明

规则表：

| 列 | 说明 |
|----|------|
| Condition | JS 布尔表达式，通过 `$input` 访问输入；`*` 表示匹配任意输入 |
| Value | JS 表达式或字面量；表达式结果写入 `default`（标量）或合并进输出（对象） |
| Action | 删除该行 |

- 空单元格显示占位提示：`JS Expression (e.g., "input['key']")`。
- **Add Rule**：新增一行。
- 按**从上到下**顺序，**第一条**匹配的规则生效。

## 4. 使用说明

1. 自上而下填写 Condition 与 Value（更具体的规则放上面）。
2. Condition 需返回布尔值，例如 `$input["code"] == 3`、`$input["enabled"]`。
3. Value 可为 JS 表达式，例如 `$input["value"] * 2`、`"sceneC"`；表达式错误时回退为字面量解析（整数/浮点/布尔/字符串）。
4. 将待查 VariableData 接到 INPUT，从 OUTPUT 取映射结果及原输入字段。

规则保存在 `rules`：`[{ "condition": "...", "value": "..." }, ...]`。

## 5. 示例

| Condition | Value | 输入 `{ "code": 3 }` 时输出 |
|-----------|-------|-----------------------------|
| `$input["code"] == 1` | `"sceneA"` | — |
| `$input["code"] == 2` | `"sceneB"` | — |
| `$input["code"] == 3` | `"sceneC"` | `{ "code": 3, "default": "sceneC", "_matchedRow": 2, "_pulse": 1 }` |
| `*` | `"default"` | 兜底 |

控台编号接入 INPUT，输出字符串或整份对象接场景加载、Extract 或 OSC 地址生成。
