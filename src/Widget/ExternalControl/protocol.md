# External Control HTTP / WebSocket 协议

## 概述

| 项 | 说明 |
|---|---|
| 默认端口 | `8992`（`AppConstants::HTTP_SERVER_PORT`） |
| 协议 | HTTP + WebSocket（同一端口） |
| 静态根目录 | 未设置时默认为进程当前目录下的 `www` |
| 缓存策略 | API / 动态响应：`Cache-Control: no-cache` |

用途：

- **WebSocket**：状态实时推送；客户端可下发控制指令或按地址查询
- **HTTP**：静态页面、控制 API、布局存取、媒体 / Flow 上传下载、设置页鉴权

---

## WebSocket

- 地址：`ws://<host>:8992/`（任意路径均可升级；推荐 `/ws`）
- 升级条件：请求头 `Upgrade: websocket`
- 服务端关闭默认收发超时（长连接）；支持 `PING` → `PONG`
- 状态变更时，服务端向**所有**已连接客户端广播

### 服务端推送（状态反馈）

由 `StatusItem::toJsonObject()` 生成：

```json
{"address":"/dataflow/DataFlow/3/int","value":60}
```

| 字段 | 类型 | 说明 |
|---|---|---|
| `address` | string | 状态地址 |
| `value` | bool / number / string / null | 当前值 |

### 客户端 → 服务端：控制指令

```json
{"address":"/dataflow/DataFlow/3/int","value":60}
```

- 地址字段兼容：`address` 或 `addr`（优先 `address`，WebSocket 路径下 `addr` 为备选）
- `value`：bool / number / string / null
- 处理：构建 `OSCMessage`，经 `StatusContainer::parseOSC` 异步执行

### 客户端 → 服务端：按需查询

```json
{
  "query": [
    "/dataflow/DataFlow/0/bool",
    "/dataflow/DataFlow/1/float"
  ]
}
```

- 仅对容器中**已存在**的地址回包
- 每个地址单独一条消息（格式同状态反馈）
- 同一帧 JSON 可同时包含 `query` 与控制字段（先处理 query，再处理控制）

### 前端示例

```js
const ws = new WebSocket('ws://' + location.host + '/ws');
ws.onopen = () => {
  ws.send(JSON.stringify({
    query: ['/dataflow/DataFlow/0/bool', '/dataflow/x']
  }));
};
ws.onmessage = (ev) => {
  const msg = JSON.parse(ev.data); // { address, value }
};
```

---

## HTTP API

所有 JSON API 响应均为 `Content-Type: application/json; charset=utf-8`。

### 路由一览

| 方法 | 路径 | 说明 |
|---|---|---|
| `POST` | `/api/auth/setting` | 设置页访问密码校验 |
| `POST` | `/api/command` | 发送控制指令（等同 WebSocket 控制） |
| `GET` | `/api/exec?...` | 联调回显（路径 + query） |
| `POST` | `/api/layout/save` | 保存布局到服务端内存 |
| `GET` | `/api/layout/load` | 加载服务端内存中的布局 |
| `POST` | `/api/upload/media?filename=` | 上传媒体文件 |
| `POST` | `/api/upload/flow?filename=` | 上传 `.flow` 并触发加载 |
| `GET` | `/api/download/current_flow` | 下载当前 Flow |
| `GET` | `/api/info/current_flow` | 查询当前 Flow 信息 |
| `GET` | `/api/info/app` | 查询软件名称与版本 |
| `*` | 其它路径 | 静态文件服务 |

---

### `POST /api/auth/setting`

校验 Web 设置页访问密码。

请求体：

```json
{"password":"..."}
```

| 结果 | HTTP | 响应 |
|---|---|---|
| 密码为空或匹配 | 200 | `{"ok":true}` |
| 密码错误 | 200 | `{"ok":false,"error":"unauthorized"}` |
| JSON 无效 | 400 | `{"ok":false,"error":"invalid_json"}` |
| 非 POST | 405 | `{"ok":false,"error":"method_not_allowed"}` |

说明：密码错误故意返回 200，避免浏览器控制台刷 401。期望密码来自 `ConfigManager::getWebAccessPassword()`。

---

### `POST /api/command`

发送控制指令，语义与 WebSocket 控制一致。

请求体：

```json
{"address":"/dataflow/DataFlow/3/int","value":60}
```

- 地址字段：`addr` 优先，否则取 `address`
- `value`：bool / number / string / null；其它类型拒绝

| 结果 | HTTP | 响应 |
|---|---|---|
| 成功 | 200 | `{"ok":true}` |
| JSON 无效 | 400 | `{"ok":false,"error":"invalid_json"}` |
| 缺少地址 | 400 | `{"ok":false,"error":"missing_addr"}` |
| value 类型不支持 | 400 | `{"ok":false,"error":"unsupported_value_type"}` |

---

### `GET /api/exec?...`

联调用：回显请求路径与查询串。前缀匹配 `/api/exec`。

```json
{"ok":true,"path":"/api/exec","query":"foo=bar"}
```

---

### `POST /api/layout/save`

将布局 JSON 保存到服务端**内存**（不写磁盘）。请求体需为 JSON Object；解析成功则覆盖内存布局。

```json
{"ok":true}
```

推荐布局格式（多页）：

```json
{
  "tabs": [
    {"id":"page_a","name":"页面A"},
    {"id":"page_b","name":"页面B"}
  ],
  "pages": {
    "page_a": {
      "items": [
        {"type":"Trigger 按钮","props":{"commandId":"/cmd/a"},"x":0,"y":0,"w":4,"h":2}
      ]
    },
    "page_b": {
      "items": [
        {"type":"滑块","props":{"commandId":"/cmd/b"},"x":1,"y":1,"w":6,"h":3}
      ]
    }
  },
  "activeTabId": "page_a"
}
```

兼容旧单页格式：`{ "items": [ ... ] }`。

服务端停止时会清空内存布局，避免新项目继承旧布局。

---

### `GET /api/layout/load`

返回内存中的布局 JSON（与上次 save / 内部 `load` 一致）。

- 内存非空：直接返回该 Object（不一定带 `ok` 字段）
- 内存为空：`{"ok":true,"items":[]}`

---

### `POST /api/upload/media?filename=<name>`

上传媒体二进制流。

| 项 | 说明 |
|---|---|
| Content-Type | `application/octet-stream`（约定） |
| 请求体 | 文件二进制 |
| 保存目录 | `AppConstants::MEDIA_LIBRARY_STORAGE_DIR` |
| 文件名 | query `filename`；会做路径/非法字符过滤 |

成功：

```json
{"ok":true,"path":"D:\\...\\yourfile.ext"}
```

| 错误 | HTTP | `error` |
|---|---|---|
| 非 POST | 405 | `method_not_allowed` |
| 文件名无效 | 400 | `missing_or_invalid_filename` |

---

### `POST /api/upload/flow?filename=<name>.flow`

上传项目文件；扩展名必须为 `.flow`（大小写不敏感）。

| 项 | 说明 |
|---|---|
| Content-Type | `application/octet-stream`（约定） |
| 保存目录 | `AppConstants::MEDIA_LIBRARY_FLOW_DIR` |
| 成功后 | 发射 `flowFileUploaded(path)`，由主窗口加载该项目 |

成功：

```json
{"ok":true,"path":"D:\\...\\project.flow"}
```

| 错误 | HTTP | `error` |
|---|---|---|
| 非 POST | 405 | `method_not_allowed` |
| 文件名无效 | 400 | `missing_or_invalid_filename` |
| 扩展名非 flow | 400 | `invalid_extension` |

---

### `GET /api/download/current_flow`

下载当前正在运行的 `.flow`（来自 `ConfigManager::getCurrentFlowPath()`）。

成功：

- `Content-Type: application/octet-stream`
- `Content-Disposition: attachment; filename="xxx.flow"`
- 响应体为文件二进制

| 错误 | HTTP | `error` |
|---|---|---|
| 非 GET | 405 | `method_not_allowed` |
| 无当前文件 | 404 | `no_file_running` |
| 磁盘上不存在 | 404 | `file_not_found` |

---

### `GET /api/info/current_flow`

查询当前 Flow 元信息。

成功：

```json
{"ok":true,"filename":"project.flow","path":"D:\\...\\project.flow"}
```

| 结果 | HTTP | 响应 |
|---|---|---|
| 无当前路径 | 200 | `{"ok":false,"error":"no_recent_file"}` |
| 文件不存在 | 200 | `{"ok":false,"error":"file_not_found"}` |
| 非 GET | 405 | `{"ok":false,"error":"no_file_running"}` |

---

### `GET /api/info/app`

查询软件名称与版本（与桌面端「关于」同源，`PRODUCT_VERSION`）。

成功：

```json
{"ok":true,"name":"Flow","version":"1.6.14"}
```

| 结果 | HTTP | 响应 |
|---|---|---|
| 非 GET | 405 | `{"ok":false,"error":"method_not_allowed"}` |

---

## 静态文件服务

- 文档根：`setDocRoot` 设置；默认 `./www`
- `/` 或空路径：优先 `www/index.html`，否则返回内置首页
- 安全：禁止 `..`、绝对路径；解析后必须仍在 docRoot 前缀下
- Content-Type：按扩展名推断（`html/htm/css/js/json/png/jpg/jpeg/gif/svg`），其它为 `application/octet-stream`
- 缓存：
  - `html` / `htm`、`service-worker.js`：`no-cache`
  - 其它静态资源：`public, max-age=31536000, immutable`
  - 支持 `ETag` + `If-None-Match` → `304 Not Modified`
- `service-worker.js` 额外头：`Service-Worker-Allowed: /`

---

## 行为说明

- WebSocket 控制与 `POST /api/command` 均走 `StatusContainer::parseOSC`
- 任意状态更新（`StatusContainer::statusUpdated`）都会经 WebSocket 广播
- 前端重连后建议只 `query` 当前界面绑定的地址，避免全量同步
- HTTP JSON API 统一 `Cache-Control: no-cache`

---

## 实现位置

| 模块 | 文件 |
|---|---|
| 声明 | `HttpServer.hpp` |
| 实现 | `HttpServer.cpp` |
| WebSocket | `PageWebSocketHandler` |
| HTTP 路由 | `StaticRequestHandler::handleRequest` |
| 广播 | `NodeHttpServer::onOscMessageSent` |
| 默认端口 | `ConstantDefines.h` → `HTTP_SERVER_PORT` |
