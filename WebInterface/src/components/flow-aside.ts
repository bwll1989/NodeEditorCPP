import { css, html, LitElement, nothing } from "lit";
import { customElement, property } from "lit/decorators.js";
import { mdiCheck, mdiClose, mdiCog, mdiPencil } from "@mdi/js";
import type { LovelaceViewConfig } from "../types";
import { viewUrlPath } from "../data/layout-mutations";
import type { FlowPanel } from "./ha-sidebar";
import "./ha-icon";

/**
 * 右侧信息栏：连接状态 / 视图导航 / 概览与快捷操作。
 * 纯展示组件——状态由父级（flow-app）注入，交互通过事件外发。
 */
@customElement("flow-aside")
export class FlowAside extends LitElement {
  @property({ type: Boolean }) public connected = false;

  @property({ attribute: false }) public views: LovelaceViewConfig[] = [];

  @property({ type: Number }) public activeView = 0;

  @property({ attribute: false }) public panel: FlowPanel = "home";

  @property({ type: Boolean }) public editMode = false;

  /** 主面板是否允许进入布局编辑模式。 */
  @property({ type: Boolean }) public canEdit = false;

  /** 窄屏（抽屉模式）时头部显示关闭按钮。 */
  @property({ type: Boolean }) public narrow = false;

  private _selectView(index: number): void {
    const view = this.views[index];
    if (!view) return;
    this.dispatchEvent(
      new CustomEvent("view-selected", {
        bubbles: true,
        composed: true,
        detail: { index, path: viewUrlPath(view, index) },
      }),
    );
  }

  private _selectPanel(panel: FlowPanel): void {
    this.dispatchEvent(
      new CustomEvent("panel-selected", {
        bubbles: true,
        composed: true,
        detail: { panel },
      }),
    );
  }

  private _toggleEdit(): void {
    this.dispatchEvent(new CustomEvent("toggle-edit", { bubbles: true, composed: true }));
  }

  private _close(): void {
    this.dispatchEvent(new CustomEvent("close", { bubbles: true, composed: true }));
  }

  private _stats(): { views: number; sections: number; cards: number } {
    let sections = 0;
    let cards = 0;
    for (const view of this.views) {
      sections += view.sections?.length ?? 0;
      for (const section of view.sections ?? []) {
        cards += section.cards?.length ?? 0;
      }
    }
    return { views: this.views.length, sections, cards };
  }

  protected render() {
    const stats = this._stats();
    const onHome = this.panel === "home";

    return html`
      <div class="aside">
        <header class="aside-header">
          <span class="aside-title">信息栏</span>
          ${this.narrow
            ? html`
                <button
                  type="button"
                  class="icon-btn"
                  aria-label="关闭信息栏"
                  title="关闭信息栏"
                  @click=${this._close}
                >
                  <ha-svg-icon .path=${mdiClose}></ha-svg-icon>
                </button>
              `
            : nothing}
        </header>

        <div class="aside-body">
          <section class="card">
            <div class="conn ${this.connected ? "online" : "offline"}">
              <span class="conn-dot" aria-hidden="true"></span>
              <div class="conn-text">
                <span class="conn-title">
                  ${this.connected ? "已连接 Flow" : "未连接 · 演示模式"}
                </span>
                <span class="conn-hint">
                  ${this.connected
                    ? "实时同步实体状态与控制指令。"
                    : "未连接到 Flow 主程序，界面数据仅为演示。"}
                </span>
              </div>
            </div>
            ${this.connected
              ? nothing
              : html`
                  <button type="button" class="link-btn" @click=${() => this._selectPanel("config")}>
                    <ha-svg-icon class="link-icon" .path=${mdiCog}></ha-svg-icon>
                    前往设置
                  </button>
                `}
          </section>

          <section class="card">
            <h3 class="card-title">视图</h3>
            ${this.views.length
              ? html`
                  <div class="view-list">
                    ${this.views.map((view, index) => {
                      const active = onHome && index === this.activeView;
                      const label = view.title || view.path || `视图 ${index + 1}`;
                      return html`
                        <button
                          type="button"
                          class="view-item ${active ? "active" : ""}"
                          aria-current=${active ? "true" : "false"}
                          @click=${() => this._selectView(index)}
                        >
                          <ha-icon class="view-icon" .icon=${view.icon || "mdi:view-dashboard"}></ha-icon>
                          <span class="view-label">${label}</span>
                        </button>
                      `;
                    })}
                  </div>
                `
              : html`<p class="empty">暂无视图</p>`}
          </section>

          <section class="card">
            <h3 class="card-title">概览</h3>
            <div class="stats">
              <div class="stat">
                <span class="stat-value">${stats.views}</span>
                <span class="stat-label">视图</span>
              </div>
              <div class="stat">
                <span class="stat-value">${stats.sections}</span>
                <span class="stat-label">分区</span>
              </div>
              <div class="stat">
                <span class="stat-value">${stats.cards}</span>
                <span class="stat-label">卡片</span>
              </div>
            </div>
            ${this.canEdit && onHome
              ? html`
                  <button
                    type="button"
                    class="edit-btn ${this.editMode ? "active" : ""}"
                    @click=${this._toggleEdit}
                  >
                    <ha-svg-icon class="edit-icon" .path=${this.editMode ? mdiCheck : mdiPencil}></ha-svg-icon>
                    ${this.editMode ? "完成编辑" : "编辑仪表盘"}
                  </button>
                `
              : nothing}
          </section>
        </div>
      </div>
    `;
  }

  static styles = css`
    :host {
      display: block;
      height: 100%;
      background: var(--sidebar-background-color, #fff);
      border-left: 1px solid var(--divider-color, rgba(0, 0, 0, 0.12));
      color: var(--primary-text-color, #1a1a1a);
      font-family: var(--ha-font-family, system-ui, sans-serif);
    }

    .aside {
      display: flex;
      flex-direction: column;
      height: 100%;
      min-height: 0;
    }

    .aside-header {
      display: flex;
      align-items: center;
      justify-content: space-between;
      flex-shrink: 0;
      height: calc(var(--header-height, 56px) + var(--safe-area-inset-top, 0px));
      padding: var(--safe-area-inset-top, 0px) 8px 0 20px;
      border-bottom: 1px solid var(--divider-color, rgba(0, 0, 0, 0.12));
      box-sizing: border-box;
    }

    .aside-title {
      font-size: 15px;
      font-weight: 600;
      letter-spacing: 0.02em;
    }

    .icon-btn {
      display: inline-flex;
      align-items: center;
      justify-content: center;
      width: 44px;
      height: 44px;
      border: none;
      border-radius: 50%;
      background: transparent;
      color: var(--sidebar-icon-color, var(--secondary-text-color));
      cursor: pointer;
    }

    .icon-btn:hover {
      background: var(--hover-background-color, rgba(0, 0, 0, 0.05));
    }

    .icon-btn ha-svg-icon {
      width: 22px;
      height: 22px;
    }

    .aside-body {
      flex: 1;
      min-height: 0;
      overflow-y: auto;
      overscroll-behavior: contain;
      padding: 16px 16px calc(20px + var(--safe-area-inset-bottom, 0px));
      display: flex;
      flex-direction: column;
      gap: 12px;
    }

    .card {
      background: var(--card-background-color, #fff);
      border: 1px solid var(--ha-card-border-color, rgba(0, 0, 0, 0.06));
      border-radius: var(--ha-border-radius-lg, 12px);
      box-shadow: var(--ha-card-box-shadow, none);
      padding: 14px;
    }

    .card-title {
      margin: 0 0 10px;
      font-size: 12px;
      font-weight: 600;
      letter-spacing: 0.06em;
      text-transform: uppercase;
      color: var(--secondary-text-color, #6f7364);
    }

    /* ---- 连接状态 ---- */
    .conn {
      display: flex;
      align-items: flex-start;
      gap: 10px;
    }

    .conn-dot {
      flex-shrink: 0;
      width: 10px;
      height: 10px;
      margin-top: 5px;
      border-radius: 50%;
      background: var(--disabled-color, rgba(0, 0, 0, 0.38));
    }

    .conn.online .conn-dot {
      background: var(--success-color, #3f6212);
      box-shadow: 0 0 0 4px color-mix(in srgb, var(--success-color, #3f6212) 16%, transparent);
    }

    .conn.offline .conn-dot {
      background: var(--error-state-color, #c62828);
      box-shadow: 0 0 0 4px color-mix(in srgb, var(--error-state-color, #c62828) 14%, transparent);
    }

    .conn-text {
      display: flex;
      flex-direction: column;
      gap: 2px;
      min-width: 0;
    }

    .conn-title {
      font-size: 14px;
      font-weight: 600;
    }

    .conn.online .conn-title {
      color: var(--success-color, #3f6212);
    }

    .conn.offline .conn-title {
      color: var(--error-state-color, #c62828);
    }

    .conn-hint {
      font-size: 12px;
      line-height: 1.5;
      color: var(--secondary-text-color, #6f7364);
    }

    .link-btn {
      display: inline-flex;
      align-items: center;
      gap: 6px;
      margin-top: 10px;
      padding: 7px 12px;
      border: none;
      border-radius: var(--ha-border-radius-md, 8px);
      background: rgba(var(--rgb-primary-color), 0.1);
      color: var(--primary-color, #79ae31);
      font: inherit;
      font-size: 13px;
      font-weight: 500;
      cursor: pointer;
    }

    .link-btn:hover {
      background: rgba(var(--rgb-primary-color), 0.16);
    }

    .link-icon {
      width: 16px;
      height: 16px;
    }

    /* ---- 视图导航 ---- */
    .view-list {
      display: flex;
      flex-direction: column;
      gap: 2px;
    }

    .view-item {
      display: flex;
      align-items: center;
      gap: 10px;
      min-height: 40px;
      padding: 0 10px;
      border: none;
      border-radius: var(--ha-border-radius-md, 8px);
      background: transparent;
      color: var(--primary-text-color, #1a1a1a);
      font: inherit;
      font-size: 14px;
      cursor: pointer;
      text-align: left;
      transition: background 0.15s ease;
    }

    .view-item:hover {
      background: var(--hover-background-color, rgba(0, 0, 0, 0.05));
    }

    .view-item.active {
      background: var(--sidebar-selected-background-color, rgba(var(--rgb-primary-color), 0.12));
      color: var(--sidebar-selected-icon-color, var(--primary-color, #79ae31));
      font-weight: 600;
    }

    .view-icon {
      --mdc-icon-size: 20px;
      width: 20px;
      height: 20px;
      flex-shrink: 0;
      color: var(--sidebar-icon-color, var(--secondary-text-color));
    }

    .view-item.active .view-icon {
      color: var(--sidebar-selected-icon-color, var(--primary-color, #79ae31));
    }

    .view-label {
      overflow: hidden;
      text-overflow: ellipsis;
      white-space: nowrap;
    }

    .empty {
      margin: 0;
      padding: 8px 2px;
      font-size: 13px;
      color: var(--secondary-text-color, #6f7364);
    }

    /* ---- 概览 ---- */
    .stats {
      display: grid;
      grid-template-columns: repeat(3, 1fr);
      gap: 8px;
    }

    .stat {
      display: flex;
      flex-direction: column;
      align-items: center;
      gap: 2px;
      padding: 10px 4px;
      border-radius: var(--ha-border-radius-md, 8px);
      background: color-mix(in srgb, var(--primary-text-color) 4%, transparent);
    }

    .stat-value {
      font-size: 18px;
      font-weight: 700;
      line-height: 1.2;
      font-variant-numeric: tabular-nums;
    }

    .stat-label {
      font-size: 11px;
      color: var(--secondary-text-color, #6f7364);
    }

    .edit-btn {
      display: inline-flex;
      align-items: center;
      justify-content: center;
      gap: 8px;
      width: 100%;
      min-height: 40px;
      margin-top: 10px;
      padding: 0 12px;
      border: 1px solid var(--primary-color, #79ae31);
      border-radius: var(--ha-border-radius-md, 8px);
      background: transparent;
      color: var(--primary-color, #79ae31);
      font: inherit;
      font-size: 13px;
      font-weight: 500;
      cursor: pointer;
      transition: background 0.15s ease, color 0.15s ease;
    }

    .edit-btn:hover {
      background: rgba(var(--rgb-primary-color), 0.1);
    }

    .edit-btn.active {
      background: var(--primary-color, #79ae31);
      color: var(--text-primary-color, #fff);
    }

    .edit-icon {
      width: 16px;
      height: 16px;
    }

    button:focus-visible {
      outline: 2px solid var(--primary-color, #79ae31);
      outline-offset: 2px;
    }

    @media (prefers-reduced-motion: reduce) {
      .view-item,
      .edit-btn {
        transition: none;
      }
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "flow-aside": FlowAside;
  }
}
