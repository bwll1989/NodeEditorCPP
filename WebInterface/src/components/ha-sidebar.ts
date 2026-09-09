import { css, html, LitElement, nothing } from "lit";
import { customElement, property, state } from "lit/decorators.js";
import { mdiClipboardTextOutline, mdiCog, mdiHome, mdiLightningBolt, mdiMenu, mdiMenuOpen, mdiPlayBoxMultipleOutline } from "@mdi/js";
import { fireEvent } from "../common/dom/fire_event";
import "./ha-icon";

export type FlowPanel = "home" | "actions" | "logs" | "media" | "config";

const SIDEBAR_TITLE = "Flow Dashboard";
const SIDEBAR_EXPANDED_KEY = "flow_sidebar_expanded";

@customElement("ha-sidebar")
export class HaSidebar extends LitElement {
  @property({ attribute: false }) public activePanel: FlowPanel = "home";

  @property({ type: Boolean }) public connected = false;

  /** HA `narrow`: sidebar is shown as a modal drawer. */
  @property({ type: Boolean, reflect: true }) public narrow = false;

  @state() private _expanded = true;

  connectedCallback(): void {
    super.connectedCallback();
    try {
      const stored = localStorage.getItem(SIDEBAR_EXPANDED_KEY);
      if (stored !== null) this._expanded = stored === "true";
    } catch {
      // ignore
    }
  }

  private get _isExpanded(): boolean {
    return this.narrow || this._expanded;
  }

  private _toggleExpanded(): void {
    if (this.narrow) {
      fireEvent(this, "hass-toggle-menu", { open: false });
      return;
    }
    this._expanded = !this._expanded;
    try {
      localStorage.setItem(SIDEBAR_EXPANDED_KEY, String(this._expanded));
    } catch {
      // ignore
    }
    fireEvent(this, "hass-dock-sidebar", {
      dock: this._expanded ? "docked" : "auto",
    });
  }

  private _select(panel: FlowPanel): void {
    this.dispatchEvent(
      new CustomEvent("panel-selected", {
        bubbles: true,
        composed: true,
        detail: { panel },
      }),
    );
    if (this.narrow) {
      fireEvent(this, "hass-toggle-menu", { open: false });
    }
  }

  private _renderNavItem(options: {
    panel: FlowPanel;
    iconPath: string;
    label: string;
    badge?: string;
  }) {
    const selected = this.activePanel === options.panel;
    return html`
      <button
        type="button"
        class="nav-item ${selected ? "selected" : ""}"
        title=${options.label}
        aria-current=${selected ? "page" : "false"}
        @click=${() => this._select(options.panel)}
      >
        <ha-svg-icon class="nav-icon" .path=${options.iconPath}></ha-svg-icon>
        <span class="nav-label">${options.label}</span>
        ${options.badge
          ? html`<span class="nav-badge">${options.badge}</span>`
          : nothing}
      </button>
    `;
  }

  protected render() {
    const expanded = this._isExpanded;
    const menuIcon = expanded ? mdiMenuOpen : mdiMenu;
    const settingsBadge = this.connected ? undefined : "!";

    return html`
      <aside
        class="sidebar ${expanded ? "expanded" : "collapsed"}"
        aria-label="主导航"
      >
        <div class="menu-header">
          <button
            type="button"
            class="menu-toggle"
            aria-label=${this._expanded ? "收起侧边栏" : "展开侧边栏"}
            @click=${this._toggleExpanded}
          >
            <ha-svg-icon .path=${menuIcon}></ha-svg-icon>
          </button>
          <span class="menu-title">${SIDEBAR_TITLE}</span>
        </div>

        <div class="sidebar-body">
          <div class="panel-list top-list">
            ${this._renderNavItem({
              panel: "home",
              iconPath: mdiHome,
              label: "Home",
            })}
            ${this._renderNavItem({
              panel: "actions",
              iconPath: mdiLightningBolt,
              label: "动作库",
            })}
            ${this._renderNavItem({
              panel: "logs",
              iconPath: mdiClipboardTextOutline,
              label: "日志",
            })}
            ${this._renderNavItem({
              panel: "media",
              iconPath: mdiPlayBoxMultipleOutline,
              label: "媒体",
            })}
          </div>

          <div class="spacer"></div>

          <div class="panel-list bottom-list">
            ${this._renderNavItem({
              panel: "config",
              iconPath: mdiCog,
              label: "设置",
              badge: settingsBadge,
            })}
          </div>
        </div>
      </aside>
    `;
  }

  static styles = css`
    :host {
      display: block;
      height: 100%;
      width: 100%;
    }

    .sidebar {
      --sidebar-item-width: 48px;
      --sidebar-expanded-item-width: 248px;
      display: flex;
      flex-direction: column;
      height: 100%;
      width: 100%;
      background: var(--sidebar-background-color, #ffffff);
      border-right: 1px solid var(--divider-color, rgba(0, 0, 0, 0.12));
      box-sizing: border-box;
      overflow: hidden;
    }

    .menu-header {
      display: flex;
      align-items: center;
      flex-shrink: 0;
      height: calc(var(--header-height, 56px) + var(--safe-area-inset-top, 0px));
      padding-top: var(--safe-area-inset-top, 0px);
      padding-left: calc(4px + var(--safe-area-inset-left, 0px));
      padding-right: 8px;
      border-bottom: 1px solid var(--divider-color, rgba(0, 0, 0, 0.12));
      overflow: hidden;
      white-space: nowrap;
      background: inherit;
    }

    .menu-toggle {
      display: inline-flex;
      align-items: center;
      justify-content: center;
      width: 48px;
      height: 48px;
      border: none;
      border-radius: 50%;
      background: transparent;
      color: var(--sidebar-icon-color, rgba(0, 0, 0, 0.54));
      cursor: pointer;
      flex-shrink: 0;
    }

    .menu-toggle:hover {
      background: rgba(0, 0, 0, 0.04);
    }

    .menu-toggle ha-svg-icon {
      width: 24px;
      height: 24px;
    }

    .menu-title {
      font-size: 20px;
      font-weight: 400;
      line-height: 1.2;
      color: var(--sidebar-text-color, rgba(0, 0, 0, 0.87));
      opacity: 0;
      max-width: 0;
      overflow: hidden;
      transition:
        max-width 0.2s ease,
        opacity 0.2s ease;
    }

    .sidebar.expanded .menu-title {
      opacity: 1;
      max-width: 220px;
      transition-delay: 0ms, 80ms;
    }

    .sidebar-body {
      flex: 1;
      min-height: 0;
      display: flex;
      flex-direction: column;
      padding-bottom: calc(8px + var(--safe-area-inset-bottom, 0px));
    }

    .panel-list {
      display: flex;
      flex-direction: column;
      padding: 8px 4px;
      gap: 4px;
      flex-shrink: 0;
    }

    .spacer {
      flex: 1;
      min-height: 8px;
    }

    .nav-item {
      position: relative;
      display: flex;
      align-items: center;
      width: var(--sidebar-item-width);
      min-height: 40px;
      margin: 0 4px;
      padding: 0 12px;
      border: none;
      border-radius: 8px;
      background: transparent;
      color: var(--sidebar-icon-color, rgba(0, 0, 0, 0.54));
      cursor: pointer;
      text-align: left;
      transition: width 0.2s ease;
    }

    .sidebar.expanded .nav-item {
      width: var(--sidebar-expanded-item-width);
    }

    .nav-item:hover {
      background: rgba(0, 0, 0, 0.04);
    }

    .nav-item.selected {
      color: var(--sidebar-selected-icon-color, var(--primary-color, #03a9f4));
    }

    .nav-item.selected::before {
      content: "";
      position: absolute;
      inset: 0;
      border-radius: inherit;
      background: var(--sidebar-selected-icon-color, var(--primary-color, #03a9f4));
      opacity: 0.12;
      pointer-events: none;
    }

    .nav-icon {
      width: 24px;
      height: 24px;
      flex-shrink: 0;
      position: relative;
      z-index: 1;
    }

    .nav-label {
      position: relative;
      z-index: 1;
      margin-left: 12px;
      font-size: 14px;
      font-weight: 500;
      line-height: 1.25;
      color: var(--sidebar-text-color, rgba(0, 0, 0, 0.87));
      white-space: nowrap;
      overflow: hidden;
      text-overflow: ellipsis;
      opacity: 0;
      max-width: 0;
      transition:
        max-width 0.2s ease,
        opacity 0.2s ease;
    }

    .sidebar.expanded .nav-label {
      opacity: 1;
      max-width: 180px;
      transition-delay: 0ms, 80ms;
    }

    .nav-item.selected .nav-label {
      color: var(--sidebar-selected-icon-color, var(--primary-color, #03a9f4));
    }

    .nav-badge {
      margin-left: auto;
      min-width: 18px;
      height: 18px;
      padding: 0 5px;
      border-radius: 9px;
      background: var(--accent-color, #ff9800);
      color: #fff;
      font-size: 11px;
      font-weight: 500;
      line-height: 18px;
      text-align: center;
      position: relative;
      z-index: 1;
      flex-shrink: 0;
      opacity: 0;
      transform: scale(0.8);
      transition: opacity 0.2s ease, transform 0.2s ease;
    }

    .sidebar.expanded .nav-badge {
      opacity: 1;
      transform: scale(1);
    }

    .sidebar.collapsed .menu-header {
      justify-content: center;
      padding-left: var(--safe-area-inset-left, 0px);
      padding-right: 0;
    }

    .sidebar.collapsed .panel-list {
      align-items: center;
      padding-left: 0;
      padding-right: 0;
    }

    .sidebar.collapsed .nav-item {
      width: 48px;
      margin: 0;
      padding: 0;
      justify-content: center;
    }

    .sidebar.collapsed .nav-label {
      position: absolute;
      width: 1px;
      height: 1px;
      margin: 0;
      padding: 0;
      overflow: hidden;
      clip: rect(0, 0, 0, 0);
      white-space: nowrap;
      border: 0;
      opacity: 0;
    }

    .sidebar.collapsed .nav-badge {
      position: absolute;
      top: 2px;
      right: 6px;
      left: auto;
      margin-left: 0;
      min-width: 16px;
      height: 16px;
      line-height: 16px;
      font-size: 10px;
      padding: 0 4px;
      opacity: 1;
      transform: scale(1);
    }

    @media (prefers-reduced-motion: reduce) {
      .sidebar,
      .menu-title,
      .nav-item,
      .nav-label,
      .nav-badge {
        transition: none;
      }
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "ha-sidebar": HaSidebar;
  }
}
