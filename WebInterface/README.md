# Flow WebInterface

仿 [Home Assistant Sections Demo](https://demo.home-assistant.io/?demo=sections#/lovelace/home) 重构的 Flow 远程控台前端。

## 技术栈（对齐 HA Frontend）

| 能力 | 实现 |
|------|------|
| UI 框架 | **Lit 3** Web Components |
| 语言 | **TypeScript strict** |
| 图标 | **@mdi/js**（Material Design Icons） |
| 状态 | **FlowStore**（对标 `hass.states`） |
| 通信 | **WebSocket**（Flow 现有 `/ws` 协议） |
| 布局 | **View → Section → Card** + CSS Grid |
| 卡片 | **注册表 + `createCardElement`**（对标 Lovelace） |
| 上下文 | **@lit/context**（对标 `hass` context） |

## 目录结构

```
WebInterface/
  src/
    entrypoints/bootstrap.ts    # 启动：WS + Store + App
    data/
      flow-store.ts             # 实体状态表
      connection.ts             # WebSocket 连接
      flow.ts                   # Flow 对象（≈ hass）
      layout.ts                 # 布局 load/save
    registry/card-registry.ts   # 卡片注册表
    panels/lovelace/
      flow-app.ts               # 应用壳
      flow-view.ts              # 视图（Tab）
      flow-section.ts           # 分区（大卡片）
      hui-card-container.ts     # 卡片容器
      cards/                    # hui-* 卡片
      create-element/             # createCardElement
    components/                 # ha-icon, ha-card
```

## 开发

```bash
cd WebInterface
npm install
npm run dev
```

Vite 开发服务器会将 `/api` 和 `/ws` 代理到 `127.0.0.1:8992`。

## 部署到 Flow.exe（生产）

1. 构建前端：

```bash
cd WebInterface
npm run build
```

2. 将 **`WebInterface/dist/` 里的全部内容**（不是 dist 文件夹本身）复制到：

```
build/bin/www/
├── index.html
└── assets/
    ├── main-xxxxx.js
    └── main-xxxxx.css
```

3. **重新编译并启动 Flow.exe**（C++ 已改为从 exe 同目录的 `www` 加载，不依赖当前工作目录）。

4. 在 Flow 菜单选择 **「打开网页控制台」**，或浏览器访问：

```
http://127.0.0.1:8992/
```

> 不要双击 `index.html` 用 `file://` 打开，必须通过 HTTP 服务访问。

5. 启动后可在 Flow 日志中看到：`HTTP static docRoot: D:/.../build/bin/www`

### 常见问题

| 现象 | 原因 | 处理 |
|------|------|------|
| 显示「FLOW HTTP Server」内置页 | 未找到 `www/index.html` | 确认文件在 `exe同目录/www/` |
| 空白页 | JS 未加载 | F12 看 Network，确认 `/assets/*.js` 为 200 |
| 未连接 | Flow 未启动或端口不对 | 确认 8992 端口、Flow 已运行 |
| `npm run dev` 报 8992 拒绝 | Flow 未开 | 先开 Flow，或用 dev 模式只看 demo 布局 |

## 布局格式 v2

```json
{
  "version": 2,
  "views": [
    {
      "title": "Home",
      "path": "home",
      "sections": [
        {
          "title": "Living Room",
          "icon": "mdi:sofa",
          "badges": [{ "type": "entity", "entity": "/demo/temp", "name": "10°C" }],
          "cards": [
            { "type": "tile", "entity": "/demo/lamp", "name": "Floor lamp", "icon": "mdi:floor-lamp" }
          ]
        }
      ]
    }
  ]
}
```

- `entity` 字段对应 Flow 后端的 **address**（如 `/dataflow/...`）
- 卡片通过 `entity` 订阅 `FlowStore`，写入走 `flow.callService(address, value)`

## 新增卡片

完整步骤见 **[第三方卡片开发指南](docs/third-party-cards.md)**。

简要流程：

1. 在 `src/panels/lovelace/cards/` 新建 `hui-xxx-card.ts`
2. 实现 `LovelaceCard` 接口：`setConfig`, `getGridOptions`, `static getStubConfig`
3. 在 `register-cards.ts` 中 `registerCard({ type, constructor, ... })`
4. 在 `card-config-schemas.ts` 中注册编辑器字段（可选但推荐）

无需修改控件库、属性面板、工厂 if-else。

## 与 Home Assistant 的对应

| Home Assistant | Flow WebInterface |
|----------------|-------------------|
| `hass.states` | `FlowStore` / `flow.states` |
| `entity_id` | `entity`（address 路径） |
| `callService` | `flow.callService` |
| `hui-tile-card` | `hui-tile-card` |
| Section 视图 | `flow-section` |
| `createCardElement` | `createCardElement` |
| `customCards` 注册 | `registerCard` |
