## 概述
- 默认端口：`8992`
- 协议类型：HTTP + WebSocket
- 用途：
  - WebSocket：实时状态反馈与控制（双向）
  - HTTP：静态文件服务与布局保存/加载、简单命令接口

## WebSocket 协议
- 连接地址：`ws://<host>:8992/`（任意路径均可升级为 WebSocket，推荐使用 `/ws`）
- 服务端会将状态变更实时广播给所有已连接的客户端

### 状态反馈（服务端推送）
服务端在状态更新时向所有客户端推送一条 JSON：
```json
{"address":"/dataflow/Dataflow/3/int","value":"60"}
```
说明：
- `address`：状态地址（字符串）
- `value`：状态值，类型可能为 `bool` / `number` / `string`

### 控制指令（客户端发送）
客户端向服务端发送控制 JSON，服务端将其转交到内部 `StatusContainer::parseOSC` 进行处理：
```json
{"address":"/dataflow/Dataflow/3/int","value":"60"}
```
`value` 支持：`bool`、`number`、`string`、`null`

### 查询指令（客户端发送）
客户端可按需查询指定地址的最新状态，避免一次性传输所有状态：
```json
{
  "query": [
    "/dataflow/Dataflow/0/bool",
    "/dataflow/Dataflow/1/float"
  ]
}
```
返回方式：服务端按地址逐条返回，每条一条消息：
```json
{"address":"/dataflow/Dataflow/0/bool","value":true}
```
```json
{"address":"/dataflow/Dataflow/1/float","value":"40"}
```

## HTTP 协议
- 基础说明：同一端口 `8992` 提供静态文件与 API
- 静态文件根：若未显式设置文档根，默认当前工作目录下的 `www` 目录
- 首页回退：若文档根不存在 `index.html`，返回内置首页

### 路由与接口
- `POST /api/command`
  - 作用：发送控制指令（与 WebSocket 控制一致）
  - 请求体（JSON）：
    ```json
    {"address":"/dataflow/Dataflow/3/int","value":60}
    ```
  - 响应：
    ```json
    {"ok":true}
    ```

- `GET /api/exec?...`（示例接口）
  - 作用：回显请求路径与查询串，便于联调
  - 响应示例：
    ```json
    {"ok":true,"path":"/api/exec","query":"foo=bar"}
    ```

- `POST /api/layout/save`
  - 作用：保存当前布局到服务端内存（不写磁盘）
  - 请求体（JSON，当前为单页示例）：
    ```json
    {
      "items": [
        {
          "type": "按钮",
          "props": { "commandId": "/cmd/demo" },
          "x": 0, "y": 0, "w": 4, "h": 2
        }
      ]
    }
    ```
  - 响应：
    ```json
    {"ok":true}
    ```

- `GET /api/layout/load`
  - 作用：加载服务端内存中的布局
  - 返回内容：与最近一次保存（或内部设置）的 JSON 完全一致
  - 推荐的新格式（多页布局）：
    ```json
    {
      "tabs": [
        {"id":"page_a","name":"页面A"},
        {"id":"page_b","name":"页面B"}
      ],
      "pages": {
        "page_a": { "items": [
          {"type":"按钮","props":{"commandId":"/cmd/a"},"x":0,"y":0,"w":4,"h":2}
        ]},
        "page_b": { "items": [
          {"type":"滑块","props":{"commandId":"/cmd/b"},"x":1,"y":1,"w":6,"h":3}
        ]}
      },
      "activeTabId": "page_a"
    }
    ```
  - 兼容性提示：前端当前仅在“加载所有分页”流程中使用上述新格式（`tabs` + `pages`）。若返回为单页 `{ "items": [...] }`，前端会按页面内本地逻辑回退，仅在部分流程使用。

## 行为说明与建议
- 前端刷新或重连时，建议按需收集界面控件绑定的地址并发送 `query`，以减少状态传输量
- 服务端对 WebSocket 控制与 HTTP 控制的处理逻辑一致，均通过内部 `StatusContainer::parseOSC` 执行
- 广播机制：任何状态更新都会通过 WebSocket广播到所有连接客户端

## 示例
### 前端建立 WebSocket 并查询
```js
const ws = new WebSocket('ws://' + location.host + '/ws');
ws.onopen = () => {
  ws.send(JSON.stringify({ query: ['/dataflow/Dataflow/0/bool', '/dataflow/x'] }));
};
ws.onmessage = (ev) => {
  const msg = JSON.parse(ev.data);
  // msg: { address, value }
};
```

## 参考实现位置
- WebSocket 处理：`src/Widget/ExternalControl/HttpServer.cpp:40-86`
- 查询处理：`src/Widget/ExternalControl/HttpServer.cpp:53-65`
- 控制指令解析：`src/Widget/ExternalControl/HttpServer.cpp:67-83`
- HTTP 路由：`src/Widget/ExternalControl/HttpServer.cpp:308-336`
- 布局加载/保存：`src/Widget/ExternalControl/HttpServer.cpp:177-219`（保存），`src/Widget/ExternalControl/HttpServer.cpp:219-233`（加载静态/首页与文件）
