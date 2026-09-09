import { css, html, LitElement, nothing } from "lit";
import { customElement, property, state } from "lit/decorators.js";
import { consume } from "@lit/context";
import type { Flow, FlowValue } from "../../../types";
import { flowContext } from "../../../context/flow-context";
import { toggleValue } from "../../../common/entity/state-active";
import {
  actionAccessLabel,
  actionDisplayName,
  deleteAction,
  fetchActions,
  type FlowAction,
} from "../../../data/actions";
import { showConfirmationDialog } from "../../../dialogs/show-confirm-dialog";

function canWriteAction(action: FlowAction): boolean {
  return action.access !== "read";
}

function resolveTestValue(entity: string, flow?: Flow): FlowValue {
  const lower = entity.toLowerCase();
  if (lower.includes("/trigger") || lower.endsWith("trigger")) {
    return true;
  }
  const current = flow?.states[entity];
  if (current) return toggleValue(current);
  return true;
}

@customElement("flow-action-library")
export class FlowActionLibrary extends LitElement {
  @consume({ context: flowContext, subscribe: true })
  @property({ attribute: false })
  public flow?: Flow;

  @state() private _actions: FlowAction[] = [];

  @state() private _loading = true;

  @state() private _error = "";

  @state() private _busyEntity = "";

  @state() private _testingEntity = "";

  @state() private _testHint = "";

  private readonly _onActionsChanged = (): void => {
    void this._load();
  };

  connectedCallback(): void {
    super.connectedCallback();
    document.addEventListener("flow-actions-changed", this._onActionsChanged);
    void this._load();
  }

  disconnectedCallback(): void {
    document.removeEventListener("flow-actions-changed", this._onActionsChanged);
    super.disconnectedCallback();
  }

  private async _load(): Promise<void> {
    this._loading = true;
    this._error = "";
    try {
      const items = await fetchActions();
      this._actions = [...items].sort((a, b) =>
        actionDisplayName(a).localeCompare(actionDisplayName(b), "zh-CN"),
      );
      const entities = this._actions.map((item) => item.entity).filter(Boolean);
      if (entities.length && this.flow) {
        this.flow.connection.trackAddresses(entities);
      }
    } catch {
      this._error = "加载动作库失败";
      this._actions = [];
    } finally {
      this._loading = false;
    }
  }

  private async _remove(action: FlowAction): Promise<void> {
    const name = actionDisplayName(action);
    const usedHint = action.used
      ? "该动作仍被页面卡片引用，删除后相关控件将失效。"
      : "删除后可从节点重新导出，或手动重新添加。";
    const confirmed = await showConfirmationDialog({
      title: "删除动作",
      text: `确定删除「${name}」（${action.entity}）？ ${usedHint}`,
      confirmText: "删除",
      cancelText: "取消",
      destructive: true,
    });
    if (!confirmed) return;

    this._busyEntity = action.entity;
    this._error = "";
    const ok = await deleteAction(action.entity);
    this._busyEntity = "";
    if (!ok) {
      this._error = `删除失败：${name}`;
      return;
    }
    await this._load();
  }

  private async _test(action: FlowAction): Promise<void> {
    if (!this.flow || !canWriteAction(action)) return;
    const name = actionDisplayName(action);
    const value = resolveTestValue(action.entity, this.flow);
    this._testingEntity = action.entity;
    this._testHint = "";
    this._error = "";
    try {
      await this.flow.callService(action.entity, value);
      const shown = typeof value === "boolean" ? (value ? "true" : "false") : String(value);
      this._testHint = `已测试「${name}」→ ${shown}`;
    } catch {
      this._error = `测试失败：${name}`;
    } finally {
      this._testingEntity = "";
    }
  }

  private _renderRow(action: FlowAction) {
    const busy = this._busyEntity === action.entity;
    const testing = this._testingEntity === action.entity;
    const name = actionDisplayName(action);
    const writable = canWriteAction(action);
    const anyBusy = Boolean(this._busyEntity || this._testingEntity);
    return html`
      <div class="row">
        <div class="meta">
          <span class="name" title=${name}>${name}</span>
          <span class="entity" title=${action.entity}>${action.entity}</span>
        </div>
        <div class="tags">
          <span class="tag">${actionAccessLabel(action.access)}</span>
          ${action.used
            ? html`<span class="tag used">使用中</span>`
            : html`<span class="tag idle">未使用</span>`}
        </div>
        <div class="ops">
          <button
            type="button"
            class="test"
            title=${writable ? "发送测试值" : "只读动作不可测试"}
            aria-label=${`测试 ${name}`}
            ?disabled=${!writable || anyBusy}
            @click=${() => void this._test(action)}
          >
            ${testing ? "…" : "测试"}
          </button>
          <button
            type="button"
            class="remove"
            title="删除"
            aria-label=${`删除 ${name}`}
            ?disabled=${anyBusy}
            @click=${() => void this._remove(action)}
          >
            ${busy ? "…" : "×"}
          </button>
        </div>
      </div>
    `;
  }

  protected render() {
    return html`
      <div class="section-head">
        <span class="section-title">已登记动作</span>
        ${this._loading ? nothing : html`<span class="count">${this._actions.length}</span>`}
      </div>
      ${this._error ? html`<p class="error">${this._error}</p>` : nothing}
      ${this._testHint ? html`<p class="hint">${this._testHint}</p>` : nothing}
      ${this._loading
        ? html`<p class="empty">加载中…</p>`
        : this._actions.length === 0
          ? html`<p class="empty">暂无动作。可通过节点右键导出，或在「手动添加」中登记。</p>`
          : html`<div class="list">${this._actions.map((a) => this._renderRow(a))}</div>`}
    `;
  }

  static styles = css`
    :host {
      display: block;
    }
    .section-head {
      display: flex;
      align-items: center;
      gap: 6px;
      margin-bottom: 8px;
    }
    .section-title {
      font-size: 13px;
      font-weight: 600;
      color: var(--primary-text-color);
    }
    .count {
      font-size: 11px;
      color: var(--secondary-text-color);
      background: color-mix(in srgb, var(--divider-color) 70%, transparent);
      border-radius: 999px;
      padding: 0 6px;
      line-height: 18px;
    }
    .empty {
      margin: 0;
      font-size: 12px;
      line-height: 1.4;
      color: var(--secondary-text-color);
    }
    .error {
      margin: 0 0 6px;
      font-size: 12px;
      color: var(--error-color, #db4437);
    }
    .hint {
      margin: 0 0 6px;
      font-size: 12px;
      color: var(--success-color, #0f9d58);
    }
    .list {
      max-height: min(52vh, 420px);
      overflow-y: auto;
      border: 1px solid var(--divider-color);
      border-radius: 8px;
      background: #fafbfc;
    }
    .row {
      display: grid;
      grid-template-columns: minmax(0, 1fr) auto auto;
      align-items: center;
      gap: 8px;
      min-height: 36px;
      padding: 4px 8px;
      border-bottom: 1px solid var(--divider-color);
    }
    .row:last-child {
      border-bottom: none;
    }
    .meta {
      min-width: 0;
      display: flex;
      align-items: baseline;
      gap: 8px;
    }
    .name {
      flex: 0 1 auto;
      max-width: 42%;
      font-size: 13px;
      font-weight: 600;
      color: var(--primary-text-color);
      white-space: nowrap;
      overflow: hidden;
      text-overflow: ellipsis;
    }
    .entity {
      flex: 1 1 auto;
      min-width: 0;
      font-size: 11px;
      color: var(--secondary-text-color);
      font-family: ui-monospace, SFMono-Regular, Menlo, Consolas, monospace;
      white-space: nowrap;
      overflow: hidden;
      text-overflow: ellipsis;
    }
    .tags {
      display: flex;
      align-items: center;
      gap: 4px;
      flex-shrink: 0;
    }
    .tag {
      font-size: 10px;
      line-height: 16px;
      padding: 0 6px;
      border-radius: 999px;
      background: color-mix(in srgb, var(--divider-color) 65%, transparent);
      color: var(--secondary-text-color);
      white-space: nowrap;
    }
    .tag.used {
      background: color-mix(in srgb, var(--primary-color, #03a9f4) 18%, transparent);
      color: var(--primary-color, #03a9f4);
    }
    .tag.idle {
      background: color-mix(in srgb, #94a3b8 20%, transparent);
      color: #64748b;
    }
    .ops {
      display: flex;
      align-items: center;
      gap: 4px;
      flex-shrink: 0;
    }
    .test,
    .remove {
      flex: 0 0 auto;
      height: 26px;
      border: 1px solid var(--divider-color);
      border-radius: 6px;
      background: #fff;
      font: inherit;
      line-height: 1;
      cursor: pointer;
      padding: 0;
    }
    .test {
      min-width: 40px;
      padding: 0 8px;
      font-size: 12px;
      font-weight: 600;
      color: var(--primary-color, #03a9f4);
    }
    .test:hover:not(:disabled) {
      background: color-mix(in srgb, var(--primary-color, #03a9f4) 10%, #fff);
    }
    .remove {
      width: 26px;
      color: var(--error-color, #db4437);
      font-size: 16px;
    }
    .remove:hover:not(:disabled) {
      background: color-mix(in srgb, var(--error-color, #db4437) 10%, #fff);
    }
    .test:disabled,
    .remove:disabled {
      opacity: 0.5;
      cursor: not-allowed;
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "flow-action-library": FlowActionLibrary;
  }
}
