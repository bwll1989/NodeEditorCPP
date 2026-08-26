import { css, html, LitElement, nothing } from "lit";
import type { PropertyValues } from "lit";
import { customElement, property, state } from "lit/decorators.js";
import { styleMap } from "lit/directives/style-map.js";
import { computeAnchoredMenuPosition } from "../common/dom/fixed-dropdown";
import { notifyEditMenuOpen, onEditMenuOpen } from "../common/dom/edit-menu-bus";
import "./ha-icon";

export interface ActionMenuItem {
  value: string;
  label: string;
  icon?: string;
  destructive?: boolean;
  divider?: boolean;
}

const MENU_MIN_WIDTH = 160;

@customElement("ha-action-menu")
export class HaActionMenu extends LitElement {
  @property({ attribute: false }) public items: ActionMenuItem[] = [];

  @state() private _open = false;

  @state() private _menuStyle: Record<string, string> = {};

  private _scrollParent?: HTMLElement | null;

  private _unsubMenuOpen?: () => void;

  connectedCallback(): void {
    super.connectedCallback();
    document.addEventListener("click", this._onDocumentClick);
    window.addEventListener("resize", this._repositionMenu);
    this._unsubMenuOpen = onEditMenuOpen((source) => {
      if (source !== this && this._open) {
        this._close();
      }
    });
  }

  disconnectedCallback(): void {
    document.removeEventListener("click", this._onDocumentClick);
    window.removeEventListener("resize", this._repositionMenu);
    this._detachScrollListener();
    this._unsubMenuOpen?.();
    super.disconnectedCallback();
  }

  private _onDocumentClick = (ev: MouseEvent): void => {
    if (!this._open) return;
    if (!ev.composedPath().includes(this)) {
      this._close();
    }
  };

  private _repositionMenu = (): void => {
    if (!this._open) return;
    this._updateMenuPosition();
  };

  private _attachScrollListener(): void {
    this._detachScrollListener();
    let node: HTMLElement | null = this.parentElement;
    while (node) {
      const style = getComputedStyle(node);
      if (/(auto|scroll)/.test(style.overflowY)) {
        this._scrollParent = node;
        node.addEventListener("scroll", this._repositionMenu, { passive: true });
        return;
      }
      node = node.parentElement;
    }
  }

  private _detachScrollListener(): void {
    this._scrollParent?.removeEventListener("scroll", this._repositionMenu);
    this._scrollParent = undefined;
  }

  private _measureMenu(menu: HTMLElement): { width: number; height: number } {
    const width = Math.max(menu.offsetWidth, MENU_MIN_WIDTH);
    const height = Math.max(menu.scrollHeight, menu.offsetHeight);
    return { width, height };
  }

  private _updateMenuPosition(): boolean {
    const trigger = this.shadowRoot?.querySelector(".trigger");
    const menu = this.shadowRoot?.querySelector(".menu");
    if (!trigger || !menu) return false;

    const triggerRect = trigger.getBoundingClientRect();
    if (triggerRect.width === 0 && triggerRect.height === 0) return false;

    const menuSize = this._measureMenu(menu as HTMLElement);
    const pos = computeAnchoredMenuPosition(triggerRect, menuSize, {
      preferredMaxHeight: 360,
      minMenuWidth: MENU_MIN_WIDTH,
    });

    this._menuStyle = {
      position: "fixed",
      top: `${pos.top}px`,
      left: `${pos.left}px`,
      width: `${pos.width}px`,
      maxHeight: `${pos.maxHeight}px`,
      zIndex: "200",
      visibility: "visible",
    };
    return true;
  }

  private async _scheduleMenuPosition(): Promise<void> {
    await this.updateComplete;
    if (!this._open) return;
    if (!this._updateMenuPosition()) {
      await new Promise<void>((resolve) => requestAnimationFrame(() => resolve()));
      if (!this._open) return;
      this._updateMenuPosition();
    }
    await new Promise<void>((resolve) => requestAnimationFrame(() => resolve()));
    if (!this._open) return;
    this._updateMenuPosition();
  }

  protected updated(changed: PropertyValues): void {
    super.updated(changed);
    if (changed.has("_open") && this._open) {
      void this._scheduleMenuPosition();
    }
  }

  private _openMenu(ev: Event): void {
    ev.stopPropagation();
    if (this._open) {
      this._close();
      return;
    }
    notifyEditMenuOpen(this);
    this._menuStyle = {
      position: "fixed",
      visibility: "hidden",
      width: `${MENU_MIN_WIDTH}px`,
    };
    this._open = true;
    this.setAttribute("open", "");
    this._attachScrollListener();
    this.dispatchEvent(new CustomEvent("menu-opened", { bubbles: true, composed: true }));
  }

  private _close(): void {
    if (!this._open) return;
    this._open = false;
    this._menuStyle = {};
    this.removeAttribute("open");
    this._detachScrollListener();
    this.dispatchEvent(new CustomEvent("menu-closed", { bubbles: true, composed: true }));
  }

  private _select(value: string, ev: Event): void {
    ev.stopPropagation();
    this._close();
    this.dispatchEvent(
      new CustomEvent("action", {
        bubbles: true,
        composed: true,
        detail: { value },
      }),
    );
  }

  protected render() {
    return html`
      <div class="wrap">
        <button
          type="button"
          class="trigger"
          @click=${this._openMenu}
          aria-label="更多操作"
          aria-expanded=${this._open ? "true" : "false"}
        >
          <slot name="trigger">⋮</slot>
        </button>
        ${this._open
          ? html`
              <div class="menu" style=${styleMap(this._menuStyle)} role="menu" @click=${(ev: Event) => ev.stopPropagation()}>
                ${this.items.map((item) =>
                  item.divider
                    ? html`<div class="divider"></div>`
                    : html`
                        <button
                          type="button"
                          class="item ${item.destructive ? "destructive" : ""}"
                          @click=${(ev: Event) => this._select(item.value, ev)}
                        >
                          ${item.icon
                            ? item.icon.startsWith("mdi:")
                              ? html`<ha-icon class="icon" .icon=${item.icon}></ha-icon>`
                              : html`<span class="icon">${item.icon}</span>`
                            : nothing}
                          <span>${item.label}</span>
                        </button>
                      `,
                )}
              </div>
            `
          : nothing}
      </div>
    `;
  }

  static styles = css`
    :host {
      display: inline-flex;
    }
    :host([open]) {
      z-index: 200;
    }
    .wrap {
      position: relative;
      display: inline-flex;
    }
    .trigger {
      border: none;
      background: var(--ha-action-menu-trigger-background, transparent);
      cursor: pointer;
      color: inherit;
      font-size: 18px;
      line-height: 1;
      padding: 0;
      width: var(--ha-action-menu-trigger-size, auto);
      height: var(--ha-action-menu-trigger-size, auto);
      min-width: var(--ha-action-menu-trigger-size, auto);
      min-height: var(--ha-action-menu-trigger-size, auto);
      border-radius: var(--ha-action-menu-trigger-radius, 50%);
      display: inline-flex;
      align-items: center;
      justify-content: center;
    }
    .trigger:hover {
      background: var(
        --ha-action-menu-trigger-hover-background,
        rgba(0, 0, 0, 0.04)
      );
    }
    .menu {
      background: var(--card-background-color, #fff);
      border-radius: 12px;
      box-shadow: 0 8px 24px rgba(0, 0, 0, 0.16);
      border: 1px solid var(--divider-color);
      padding: 6px 0;
      overflow: auto;
      box-sizing: border-box;
    }
    .item {
      width: 100%;
      border: none;
      background: transparent;
      display: flex;
      align-items: center;
      gap: 10px;
      padding: 10px 14px;
      font-size: 14px;
      color: var(--primary-text-color);
      cursor: pointer;
      text-align: left;
    }
    .item:hover {
      background: rgba(0, 0, 0, 0.04);
    }
    .item.destructive {
      color: var(--error-color, #b91c1c);
    }
    .divider {
      height: 1px;
      background: var(--divider-color);
      margin: 4px 0;
    }
    .icon {
      width: 18px;
      display: inline-flex;
      align-items: center;
      justify-content: center;
      opacity: 0.8;
      --mdc-icon-size: 18px;
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "ha-action-menu": HaActionMenu;
  }
}
