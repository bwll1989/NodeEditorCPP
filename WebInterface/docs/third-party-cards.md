# 第三方卡片开发指南

本文说明如何为 Flow WebInterface 创建并集成自定义卡片（第三方卡片）。架构对标 Home Assistant Lovelace：`registerCard` 注册表 + `createCardElement` 工厂，无需修改核心 if-else。

---

## 1. 架构概览

```
布局 JSON (type: "my-card")
        ↓
createCardElement()  →  card-registry 查找 type
        ↓
new MyCardConstructor()
        ↓
hui-card-container 注入 flow、订阅 entity、计算 grid 尺寸
        ↓
你的 hui-*-card Web Component 渲染 UI
```

| 概念 | Home Assistant | Flow WebInterface |
|------|----------------|-------------------|
| 实体 ID | `entity_id` | `entity`（OSC / 数据流 **地址**，如 `/demo/light`） |
| 状态表 | `hass.states` | `flow.states` |
| 写入控制 | `callService` | `flow.callService(address, value)` |
| 注册卡片 | `customCards` | `registerCard()` |
| 卡片接口 | `LovelaceCard` | 同名 `LovelaceCard` |

---

## 2. 前置条件

- Node.js 18+
- 熟悉 **Lit 3** Web Components 与 TypeScript
- 在 `WebInterface/` 目录执行过 `npm install`

当前版本**不支持**在已部署的 `dist` 中动态加载外部 JS 插件；第三方卡片需加入源码树、注册后重新 `npm run build` 并部署。

---

## 3. 最小卡片示例

### 3.1 创建组件

路径：`src/panels/lovelace/cards/hui-hello-card.ts`

```typescript
import { css, html, LitElement, nothing } from "lit";
import { customElement, property, state } from "lit/decorators.js";
import type { Flow, LovelaceCard, LovelaceCardConfig, LovelaceGridOptions } from "../../../types";
import "../../../components/ha-card";

@customElement("hui-hello-card")
export class HuiHelloCard extends LitElement implements LovelaceCard {
  /** 新建卡片时的默认配置（卡片选择器预览用） */
  public static getStubConfig(): LovelaceCardConfig {
    return {
      type: "hello",
      entity: "/demo/trigger",
      name: "Hello",
      message: "你好，Flow！",
    };
  }

  @property({ attribute: false }) public flow?: Flow;

  @property({ type: Boolean }) public preview = false;

  @state() private _config?: LovelaceCardConfig;

  setConfig(config: LovelaceCardConfig): void {
    this._config = config;
    this.requestUpdate();
  }

  getGridOptions(): LovelaceGridOptions {
    return { columns: 6, rows: 1, min_columns: 3, min_rows: 1 };
  }

  protected render() {
    if (!this._config) return nothing;

    const entity = this._config.entity;
    const state = entity && this.flow ? this.flow.states[entity] : undefined;
    const name = String(this._config.name ?? "Hello");
    const message = String(this._config.message ?? "");

    return html`
      <ha-card>
        <div class="body">
          <div class="title">${name}</div>
          <div class="message">${message}</div>
          <div class="state">${state ? String(state.state) : "—"}</div>
        </div>
      </ha-card>
    `;
  }

  static styles = css`
    :host {
      display: block;
      height: 100%;
    }
    .body {
      padding: 12px;
    }
    .title {
      font-weight: 600;
    }
    .state {
      color: var(--secondary-text-color);
      font-size: 13px;
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "hui-hello-card": HuiHelloCard;
  }
}
```

### 3.2 必须实现的接口

`LovelaceCard`（见 `src/types/index.ts`）：

| 成员 | 必填 | 说明 |
|------|------|------|
| `setConfig(config)` | 是 | 保存配置并触发重绘 |
| `getGridOptions()` | 推荐 | 默认网格占位（列/行） |
| `flow` | 自动注入 | 由 `hui-card-container` 赋值，含状态与控制 API |
| `preview` | 可选 | 编辑器预览模式 |
| `static getStubConfig()` | 推荐 | 卡片选择器与「添加卡片」默认项 |

`LovelaceCardConstructor` 可选：

- `static getConfigForm()` — 高级：完全自定义配置表单（多数卡片用下方 schema 即可）

---

## 4. 注册卡片

在 `src/panels/lovelace/cards/register-cards.ts`（或你自己的 `register-my-cards.ts`）中：

```typescript
import { registerCard } from "../../../registry/card-registry";
import { HuiHelloCard } from "./hui-hello-card";

registerCard({
  type: "hello",                    // 布局 JSON 中的 type，全局唯一
  name: "Hello",                    // 卡片选择器显示名
  description: "示例第三方卡片",
  icon: "mdi:hand-wave",            // MDI 图标名
  category: "container",            // tile | control | sensor | container
  constructor: HuiHelloCard,
});
```

`category` 仅影响卡片选择器分组：

- `tile` — 磁贴类
- `control` — 控制类（滑块、开关、增益等）
- `sensor` — 数值/图表类
- `container` — 标题、Markdown、图片等

若拆成独立文件，需在 `src/entrypoints/bootstrap.ts` 中 import，保证在 `flow-app` 启动前执行注册：

```typescript
import "../panels/lovelace/cards/register-cards";
import "../panels/lovelace/cards/register-my-cards"; // 你的注册
```

---

## 5. 配置编辑器（Schema）

编辑器通过 `getCardConfigSchema(type)` 读取字段定义。在 `src/panels/lovelace/editor/card-config-schemas.ts` 的 `CARD_CONFIG_SCHEMAS` 中增加：

```typescript
hello: [
  ...entityCardSchema(),   // 含 entity、name、icon、color
  { name: "message", label: "消息", type: "text", optional: true },
],
```

常用字段类型：`text`、`number`、`boolean`、`select`、`select_box`、`icon`、`color`、`entity`、`textarea`、`section`、`grid`。

复用工具：

- `entityCardSchema([...extras])` — 带地址 + 内容区的标准实体卡片
- `CONTENT_LAYOUT_FIELD` — 水平/垂直布局（若卡片支持 `vertical`）

在 `flow-card-editor.ts` 的 `_cardTypeName()` 中补充显示名（可选）：

```typescript
hello: "Hello",
```

若配置需在保存/加载时转换，可扩展 `normalizeCardConfig` / `denormalizeCardConfig`（参见 `gain`、`clock` 等卡片）。

---

## 6. 订阅状态与控制写入

### 6.1 读取状态

```typescript
const state = this.flow?.states[this._config.entity!];
const value = state?.state;
const attrs = state?.attributes;
```

容器层已订阅 `config.entity` 并在变化时刷新卡片。若卡片使用**额外地址**（如增益的 `mute_entity`），需：

1. 在 `collectCardEntityAddresses()`（`src/data/flow.ts`）和 `collectCardEntities()`（`src/data/section-config.ts`）中收集这些字段；
2. 或在卡片内自行 `flow.subscribeEntity(address, callback)`（记得在 `disconnectedCallback` 取消）。

### 6.2 写入控制

```typescript
await this.flow?.callService("/your/address", true);
// value 类型: boolean | number | string | null
```

离线 Demo 模式下，`callService` 会走 `demo-service.ts` 的本地模拟逻辑。

### 6.3 在 render 中依赖状态版本

容器订阅更新后可能只调用子卡片的 `requestUpdate`。在 `render()` 开头读取一次：

```typescript
void this.flow?.statesRevision;
```

以确保 Lit 在任意 entity 变化时重新渲染。

---

## 7. 网格尺寸 `getGridOptions`

Section 布局为 12 列 CSS Grid。返回示例：

```typescript
getGridOptions(): LovelaceGridOptions {
  return {
    columns: 6,       // 占 6 列（半宽）
    rows: 2,            // 占 2 行
    min_columns: 6,
    min_rows: 2,
    max_columns: 12,
  };
}
```

用户可在「布局」编辑器中覆盖 `grid_options`。若卡片支持横/竖布局，在 `syncLayoutGridOptions` 中为你的 `type` 添加规则（参见 `tile`、`media`）。

---

## 8. 懒加载（体积较大的卡片）

图表等重组件建议懒加载，参考 `register-cards.ts` 中的 `registerLazyChartCard`：

```typescript
registerCard({
  type: "hello-heavy",
  name: "Hello Heavy",
  icon: "mdi:package",
  category: "container",
  lazy: true,
  loader: async () => {
    const mod = await import("./hui-hello-heavy-card");
    entry.constructor = mod.HuiHelloHeavyCard;
  },
} as CardRegistryEntry);
```

`createCardElement` 会在首次使用时 `await ensureCardLoaded(type)`。

---

## 9. 图标

- 配置与注册表中使用 **`mdi:icon-name`** 字符串。
- 卡片选择器、首屏常用图标应加入 `src/common/icons/mdi-registry.ts` 的 `STATIC_PATHS`，否则在 MDI 全库懒加载完成前可能显示占位图标。
- 组件内使用 `<ha-icon .icon=${"mdi:play"}></ha-icon>`（注意 property 绑定 `.icon=`）。

---

## 10. 复用 UI 组件

内置可复用组件（`src/components/`）：

| 组件 | 用途 |
|------|------|
| `ha-card` | 卡片外框 |
| `ha-tile-container` / `ha-tile-icon` / `ha-tile-info` | 磁贴布局 |
| `ha-control-slider` | 滑块 |
| `ha-control-switch` | 开关 |
| `ha-control-number-buttons` | ± 步进 |
| `ha-control-cover-position` | 分段单选 |
| `ha-control-media-playback` | 播放/停止 |

样式可引入 `tileCardStyle`、`tileCardHostStyle`（`src/styles/tile-card-style.ts`）。

推荐参考的完整卡片：

- 简单只读：`hui-sensor-card.ts`
- 实体控制：`hui-tile-card.ts`、`hui-switch-card.ts`
- 双行 feature：`hui-gain-card.ts`、`hui-media-card.ts`

---

## 11. 布局 JSON 示例

```json
{
  "type": "hello",
  "entity": "/dataflow/volume",
  "name": "音量",
  "icon": "mdi:volume-high",
  "message": "自定义说明",
  "grid_options": {
    "columns": 6,
    "rows": 1
  }
}
```

保存后通过 `POST /api/layout/save` 持久化；Flow 启动时 `GET /api/layout/load` 加载。

---

## 12. 构建与部署

```bash
cd WebInterface
npm run build
```

将 `dist/` **内所有文件**复制到 Flow 可执行文件旁的 `www/`：

```
build/bin/www/
├── index.html
└── assets/
```

重新编译/启动 Flow 后访问 `http://127.0.0.1:8992/`。勿用 `file://` 打开。

本地开发：

```bash
npm run dev
```

Vite 将 `/api`、`/ws` 代理到 `127.0.0.1:8992`。

---

## 13. 开发检查清单

- [ ] `@customElement("hui-xxx-card")` 标签名与类名一致
- [ ] 实现 `LovelaceCard`：`setConfig`、`getGridOptions`
- [ ] 提供 `static getStubConfig()`
- [ ] `registerCard({ type, name, icon, category, constructor })`
- [ ] `CARD_CONFIG_SCHEMAS` 中注册编辑器字段
- [ ] 使用的 `entity` 已被 `collectCardEntityAddresses` 收集（多地址卡片）
- [ ] 新图标加入 `mdi-registry.ts` STATIC_PATHS（若首屏需要）
- [ ] `npm run build` 无 TypeScript 错误
- [ ] 编辑模式下拖拽、配置、撤销/保存正常

---

## 14. 常见问题

**Q: 卡片显示 “Unknown card type”**  
A: `type` 未注册，或 `register-cards` 未被 bootstrap import。

**Q: 状态不刷新**  
A: 确认 `entity` 路径正确；render 中读取 `flow.statesRevision`；额外地址是否已订阅。

**Q: 图标空白**  
A: 将对应 MDI 名加入 `STATIC_PATHS`，或使用已收录的图标。

**Q: 能否不写 TypeScript 直接写 JS？**  
A: 可以，但项目为 strict TS，建议保持一致；JS 需自行保证与 `LovelaceCard` 结构兼容。

**Q: 能否打包成独立 npm 包？**  
A: 可以，但最终仍需在你的分支中 import 并执行 `registerCard`，然后重新构建 WebInterface 并部署 `www`。

---

## 15. 相关源码索引

| 文件 | 说明 |
|------|------|
| `src/types/index.ts` | `LovelaceCard`、`CardRegistryEntry`、`ConfigFieldSchema` |
| `src/registry/card-registry.ts` | 注册表 API |
| `src/panels/lovelace/create-element/create-card-element.ts` | 卡片工厂 |
| `src/panels/lovelace/hui-card-container.ts` | 容器：flow 注入、entity 订阅、grid |
| `src/panels/lovelace/cards/register-cards.ts` | 内置卡片注册 |
| `src/panels/lovelace/editor/card-config-schemas.ts` | 配置编辑器 schema |
| `src/data/flow.ts` | `Flow` 对象与 `callService` |
| `src/entrypoints/bootstrap.ts` | 应用入口 |

---

如有新卡片类型需要新的编辑器字段控件（如自定义列表编辑器），需在 `flow-card-config-editor.ts` 中扩展对应 `type` 的渲染分支。
