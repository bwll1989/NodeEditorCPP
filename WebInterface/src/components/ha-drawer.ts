import { css, html, LitElement } from "lit";
import { customElement, property } from "lit/decorators.js";
import { fireEvent } from "../common/dom/fire_event";

/**
 * Simplified port of HA `ha-drawer`.
 * - default: persistent sidebar (desktop)
 * - type="modal": overlay drawer (narrow / mobile)
 *
 * 左侧导航（slot="sidebar"，open 控制）+ 右侧信息栏（slot="aside"，asideOpen 控制）。
 * 打开动作只发事件由父级改状态（单向数据流）；关闭动作本地置位 + 发事件同步父级。
 */
@customElement("ha-drawer")
export class HaDrawer extends LitElement {
  @property({ reflect: true }) public type: "" | "modal" = "";

  @property({ type: Boolean, reflect: true }) public open = false;

  @property({ type: Boolean, reflect: true }) public asideOpen = false;

  private _touchId: number | null = null;
  private _touchStartX = 0;
  private _touchStartY = 0;
  /** Content-area gesture origin: "left" / "right" edge, or null when not near an edge. */
  private _contentEdge: "left" | "right" | null = null;

  private get _modal(): boolean {
    return this.type === "modal";
  }

  private _closeLeft(): void {
    if (!this.open) return;
    this.open = false;
    fireEvent(this, "hass-drawer-closed");
  }

  private _closeAside(): void {
    if (!this.asideOpen) return;
    this.asideOpen = false;
    fireEvent(this, "flow-aside-closed");
  }

  private _onScrimClick = (): void => {
    if (this.asideOpen) this._closeAside();
    else this._closeLeft();
  };

  // ---- modal drawer close gestures (swipe toward the edge) ----

  private _onLeftTouchStart = (ev: TouchEvent): void => {
    const touch = ev.touches[0];
    if (!touch) return;
    this._touchId = touch.identifier;
    this._touchStartX = touch.clientX;
    this._touchStartY = touch.clientY;
  };

  private _onLeftTouchEnd = (ev: TouchEvent): void => {
    if (this._touchId === null) return;
    const touch = this._endTouch(ev);
    if (!touch) return;
    const dx = touch.clientX - this._touchStartX;
    const dy = touch.clientY - this._touchStartY;
    if (dx < -72 && Math.abs(dy) < 80) this._closeLeft();
  };

  private _onAsideTouchEnd = (ev: TouchEvent): void => {
    if (this._touchId === null) return;
    const touch = this._endTouch(ev);
    if (!touch) return;
    const dx = touch.clientX - this._touchStartX;
    const dy = touch.clientY - this._touchStartY;
    if (dx > 72 && Math.abs(dy) < 80) this._closeAside();
  };

  // ---- content-area edge gestures (open drawers from screen edges) ----

  private _onContentTouchStart = (ev: TouchEvent): void => {
    if (!this._modal || this.open || this.asideOpen) return;
    const touch = ev.touches[0];
    if (!touch) return;
    const width = this.clientWidth || window.innerWidth;
    this._touchId = touch.identifier;
    this._touchStartX = touch.clientX;
    this._touchStartY = touch.clientY;
    this._contentEdge =
      touch.clientX <= 24 ? "left" : touch.clientX >= width - 24 ? "right" : null;
  };

  private _onContentTouchEnd = (ev: TouchEvent): void => {
    if (this._contentEdge === null) return;
    const touch = this._endTouch(ev);
    if (!touch) return;
    const dx = touch.clientX - this._touchStartX;
    const dy = touch.clientY - this._touchStartY;
    if (Math.abs(dy) < 80) {
      if (this._contentEdge === "left" && dx > 72) {
        fireEvent(this, "hass-toggle-menu", { open: true });
      } else if (this._contentEdge === "right" && dx < -72) {
        fireEvent(this, "flow-toggle-aside", { open: true });
      }
    }
    this._contentEdge = null;
  };

  /** Consume the tracked touch; returns the matching changedTouch or null. */
  private _endTouch(ev: TouchEvent): Touch | null {
    const touch = ev.changedTouches
      ? Array.from(ev.changedTouches).find((t) => t.identifier === this._touchId) ?? null
      : null;
    this._touchId = null;
    return touch;
  }

  protected updated(): void {
    document.body.style.overflow =
      this._modal && (this.open || this.asideOpen) ? "hidden" : "";
  }

  disconnectedCallback(): void {
    document.body.style.overflow = "";
    super.disconnectedCallback();
  }

  protected render() {
    if (this._modal) {
      return html`
        <div class="layout modal">
          <div
            class="scrim ${this.open || this.asideOpen ? "visible" : ""}"
            @click=${this._onScrimClick}
          ></div>
          <aside
            class="modal-drawer ${this.open ? "open" : ""}"
            @touchstart=${this._onLeftTouchStart}
            @touchend=${this._onLeftTouchEnd}
          >
            <slot name="sidebar"></slot>
          </aside>
          <aside
            class="modal-aside ${this.asideOpen ? "open" : ""}"
            @touchstart=${this._onLeftTouchStart}
            @touchend=${this._onAsideTouchEnd}
          >
            <slot name="aside"></slot>
          </aside>
          <div
            class="app-content"
            @touchstart=${this._onContentTouchStart}
            @touchend=${this._onContentTouchEnd}
          >
            <slot></slot>
          </div>
        </div>
      `;
    }

    return html`
      <div class="layout">
        <aside class="sidebar-shell">
          <slot name="sidebar"></slot>
        </aside>
        <div class="app-content">
          <slot></slot>
        </div>
        <aside class="aside-shell">
          <slot name="aside"></slot>
        </aside>
      </div>
    `;
  }

  static styles = css`
    :host {
      display: block;
      height: 100%;
    }
    .layout {
      display: flex;
      height: 100%;
      min-height: 0;
    }
    .sidebar-shell {
      flex: none;
      width: var(--ha-sidebar-width, 56px);
      height: 100%;
      box-sizing: border-box;
      overflow: hidden;
      z-index: 6;
      transition: width 200ms cubic-bezier(0.2, 0, 0, 1);
    }
    .aside-shell {
      flex: none;
      width: var(--ha-aside-width, 0px);
      height: 100%;
      box-sizing: border-box;
      overflow: hidden;
      z-index: 6;
      transition: width 200ms cubic-bezier(0.2, 0, 0, 1);
    }
    .app-content {
      flex: 1;
      min-width: 0;
      min-height: 0;
      height: 100%;
      display: flex;
      flex-direction: column;
      box-sizing: border-box;
    }
    .app-content ::slotted(*) {
      flex: 1;
      min-height: 0;
      height: 100%;
    }
    .sidebar-shell ::slotted(*),
    .aside-shell ::slotted(*),
    .modal-drawer ::slotted(*),
    .modal-aside ::slotted(*) {
      display: block;
      width: 100%;
      height: 100%;
    }
    .layout.modal .app-content {
      width: 100%;
    }
    .scrim {
      position: fixed;
      inset: 0;
      background: rgba(0, 0, 0, 0.32);
      opacity: 0;
      pointer-events: none;
      transition: opacity 200ms ease;
      z-index: 199;
    }
    .scrim.visible {
      opacity: 1;
      pointer-events: auto;
    }
    .modal-drawer,
    .modal-aside {
      position: fixed;
      top: 0;
      bottom: 0;
      width: calc(256px + var(--safe-area-inset-left, 0px));
      max-width: 86vw;
      background: var(--sidebar-background-color, #fff);
      box-shadow: 0 8px 10px -5px rgba(0, 0, 0, 0.2), 0 16px 24px 2px rgba(0, 0, 0, 0.14),
        0 6px 30px 5px rgba(0, 0, 0, 0.12);
      transform: translateX(-105%);
      transition: transform 240ms cubic-bezier(0.2, 0, 0, 1);
      z-index: 200;
      overflow: hidden;
      overscroll-behavior: contain;
    }
    .modal-drawer {
      left: 0;
    }
    .modal-aside {
      right: 0;
      width: calc(300px + var(--safe-area-inset-right, 0px));
      transform: translateX(105%);
    }
    .modal-drawer.open,
    .modal-aside.open {
      transform: translateX(0);
    }
    @media (prefers-reduced-motion: reduce) {
      .sidebar-shell,
      .aside-shell,
      .scrim,
      .modal-drawer,
      .modal-aside {
        transition: none;
      }
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "ha-drawer": HaDrawer;
  }
}
