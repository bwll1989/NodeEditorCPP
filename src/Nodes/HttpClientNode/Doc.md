# HTTP Client

## 1. 节点说明

HTTP Client 向指定 URL 发送 HTTP 请求（GET / POST / PUT / DELETE / PATCH），并将响应解析为 `VariableData` 输出。适合调用 Web API、Webhook、设备 HTTP 接口，或与 Mpv/VLC 等 HTTP 服务配合做通用请求。

## 2. 端口说明

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

## 3. 界面说明

| 控件 | 说明 |
|------|------|
| Base URL | 基础地址，如 `http://127.0.0.1:8080` |
| Method | GET / POST / PUT / DELETE / PATCH |
| Path | 请求路径，如 `/api/users`；也可写完整 URL |
| Headers | 可选 JSON 请求头，如 `{"Authorization":"Bearer token"}` |
| Body | POST/PUT/PATCH 请求体（JSON 文本） |
| Send | 手动发送请求（界面编辑后也会自动发送） |

**外部控制地址：**

| 地址 | 说明 |
|------|------|
| `/baseUrl` | 设置基础 URL |
| `/method` | 设置方法（0~4） |
| `/path` | 设置路径 |
| `/body` | 设置请求体 |
| `/headers` | 设置请求头 JSON |
| `/send` | 发送 true 时触发请求 |

## 4. 使用说明

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

## 5. 示例

**查询状态：** GET，`/api/status` → RESPONSE 接 Extract `$input.online`。

**Webhook 触发：** Edge Trigger PULSE → TRIGGER；POST Body `{"event":"start"}`。

**动态路径：** PATH 端口接 Variable，`default` 为 `/api/scene/3`；路径变化即自动请求。

**VLC 控制：** Base URL `http://127.0.0.1:8080`，PATH 接动态命令路径，HEADERS 端口接 `{"headers":"{\"Authorization\":\"Basic OjEyMzQ=\"}"}` 或 map 形式。

**请求完成后续：** DONE → Inject TRIGGER，RESPONSE → Merge 合并到流程数据。
