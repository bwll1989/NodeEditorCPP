# HTTP Client

## 1. 节点说明

HTTP Client 向指定 URL 发送 HTTP 请求（GET / POST / PUT / DELETE / PATCH），并将响应解析为 `VariableData` 输出。适合调用 Web API、Webhook、设备 HTTP 接口，或与 Mpv/VLC 等 HTTP 服务配合做通用请求。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 5 | 否 | 5 | 5 | — | 末端口为 HEADERS（覆盖请求头） |
| 输出 | 2 | 否 | 2 | 2 | — | 末端口为 DONE（成功布尔） |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。



### 输入

| 端口 | 名称 | 数据类型 | 说明 |
|------|------|----------|------|
| 0 | TRIGGER | VariableData | 收到数据时发起请求（布尔 `true` 仅作触发，不会当作 path） |
| 1 | PATH | VariableData | 覆盖 Path；`path` 键或形如 `/api/...` 的 `default` 字符串 |
| 2 | BODY | VariableData | 覆盖 Body（POST/PUT/PATCH） |
| 3 | BASE_URL | VariableData | 覆盖 Base URL；`baseUrl` 键或 `http(s)://...` 的 `default` 字符串 |
| 4 | HEADERS | VariableData | 覆盖 Headers；`headers` 键（JSON 字符串或 map）、`default` 为 JSON 对象字符串，或整个 map 作为请求头 |

**任意输入端口数据变化时都会自动发送请求**，无需再手动点 Send 或额外接 TRIGGER。若请求进行中又有新输入，会排队并在当前请求完成后自动重发。

TRIGGER 输入也可通过字段覆盖：
- `path`：请求路径（须以 `/` 或 `http` 开头）
- `body`：请求体（字符串或对象）
- `baseUrl`：基础地址
- `headers`：请求头 JSON 对象或字符串

**注意：** TRIGGER 上单纯的 `true`/`false` 脉冲只用于触发，不会误当成路径。

### 输出

| 端口 | 名称 | 数据类型 | 说明 |
|------|------|----------|------|
| 0 | RESPONSE | VariableData | 响应内容，含 `statusCode`、`success`、`url`；JSON 字段会展开到顶层 |
| 1 | DONE | VariableData | 请求完成时输出 `success` 布尔值，可接 Inject / Delay TRIGGER |

## 3. 界面与属性

| 控件 | 说明 |
|------|------|
| Base URL | 基础地址，如 `http://127.0.0.1:8080` |
| Method | GET / POST / PUT / DELETE / PATCH |
| Path | 请求路径，如 `/api/users`；也可写完整 URL |
| Headers | 可选 JSON 请求头，如 `{"Authorization":"Bearer token"}` |
| Body | POST/PUT/PATCH 请求体（JSON 文本） |
| Send | 手动发送请求（界面编辑后也会自动发送） |

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`

| 地址 | 方向 | 说明 |
|------|------|------|
| `/baseUrl` | 读/写 | 设置基础 URL，**写入后立即自动发一次请求** |
| `/method` | 读/写 | 设置方法（payload 转 int，0=GET 1=POST 2=PUT 3=DELETE 4=PATCH），写入后自动请求 |
| `/path` | 读/写 | 设置路径，写入后自动请求 |
| `/body` | 读/写 | 设置请求体，写入后自动请求 |
| `/headers` | 读/写 | 设置请求头 JSON，写入后自动请求 |
| `/send` | 写 | payload 为 true 时触发请求 |

## 5. 行为细节

- **自动发送**：任意输入端口数据变化、界面参数编辑、OSC 参数写入都会触发请求；请求进行中又有新触发时进入队列，当前请求完成后自动重发（不丢最后一次参数）。
- **参数覆盖优先级**：端口数据 > 属性值；发送前先从界面控件同步一次参数（`syncParametersFromWidget`）。
- **路径/URL 识别**：`path` 键或以 `/`、`http(s)://` 开头的 `default` 字符串才被视为路径；纯布尔 true/false 只作触发。
- **响应解析**：HTTP 状态码与网络错误记入 `statusCode`/`success`/`error`；响应为 JSON 对象时顶层键展开到输出 map，JSON 数组存入 `data`，非 JSON 文本存入 `body`。
- **DONE 端口**：每次请求完成输出一次 `success` 布尔（可做脉冲链）。
- **持久化**：`values.baseUrl` / `values.method` / `values.path` / `values.body` / `values.headers`。

## 6. 使用说明与典型接法

1. 填写 Base URL、Method、Path；POST/PUT/PATCH 时填写 Body。
2. 任意输入端口或界面参数变化即自动发请求；也可点 Send，或将 TRIGGER 接 Condition DATA、Edge Trigger PULSE 等。
3. RESPONSE 接 Extract、Condition、Data Info 解析返回字段。
4. DONE 接仅需「请求完成」脉冲的下游。

**响应字段：**

| 字段 | 说明 |
|------|------|
| `statusCode` | HTTP 状态码 |
| `success` | 是否无网络错误 |
| `error` | 错误信息（失败时） |
| `url` | 实际请求 URL |
| `body` | 非 JSON 响应的原始文本 |
| `data` | JSON 数组响应 |
| 其他字段 | JSON 对象响应的顶层键 |

配置保存在 `values.baseUrl`、`values.method`、`values.path`、`values.body`、`values.headers`。

## 7. 示例

**查询状态：** GET，`/api/status` → RESPONSE 接 Extract `$input.online`。

**Webhook 触发：** Edge Trigger PULSE → TRIGGER；POST Body `{"event":"start"}`。

**动态路径：** PATH 端口接 Variable，`default` 为 `/api/scene/3`；路径变化即自动请求。

**VLC 控制：** Base URL `http://127.0.0.1:8080`，PATH 接动态命令路径，HEADERS 端口接 `{"headers":"{\"Authorization\":\"Basic OjEyMzQ=\"}"}` 或 map 形式。

**请求完成后续：** DONE → Inject TRIGGER，RESPONSE → Merge 合并到流程数据。

