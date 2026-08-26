import { css, html, LitElement } from "lit";
import { customElement, property } from "lit/decorators.js";
import { fireEvent } from "../common/dom/fire_event";

/**
 * Simplified port of HA `ha-drawer`.
 * - default: persistent sidebar (desktop)
 * - type="modal": overlay drawer (narrow / mobile)
 */
@customElement("ha-drawer")
export class HaDrawer extends LitElement {
  @property({ reflect: true }) public type: "" | "modal" = "";

  @property({ type: Boolean, reflect: true }) public open = false;

  private _touchStartX = 0;

  private get _modal(): boolean {
    return this.type === "modal";
  }

  private _close(): void {
    if (!this.open) return;
    this.open = false;
    fireEvent(this, "hass-drawer-closed");
  }

  private _onTouchStart = (ev: TouchEvent): void => {
    if (!this._modal || !this.open) return;
    this._touchStartX = ev.touches[0]?.clientX ?? 0;
  };

  private _onTouchEnd = (ev: TouchEvent): void => {
    if (!this._modal || !this.open) return;
    const endX = ev.changedTouches[0]?.clientX ?? this._touchStartX;
    if (this._touchStartX - endX > 72) {
      this._close();
    }
  };

  protected updated(): void {
    document.body.style.overflow = this._modal && this.open ? "hidden" : "";
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
            class="scrim ${this.open ? "visible" : ""}"
            @click=${this._close}
          ></div>
          <aside
            class="modal-drawer ${this.open ? "open" : ""}"
            @touchstart=${this._onTouchStart}
            @touchend=${this._onTouchEnd}
          >
            <slot name="sidebar"></slot>
          </aside>
          <div class="app-content">
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
      transition: width 180ms ease;
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
    .modal-drawer ::slotted(*) {
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
      transition: opacity 180ms ease;
      z-index: 199;
    }
    .scrim.visible {
      opacity: 1;
      pointer-events: auto;
    }
    .modal-drawer {
      position: fixed;
      top: 0;
      bottom: 0;
      left: 0;
      width: calc(256px + var(--safe-area-inset-left, 0px));
      max-width: 86vw;
      background: var(--sidebar-background-color, #fff);
      box-shadow: 0 8px 10px -5px rgba(0, 0, 0, 0.2), 0 16px 24px 2px rgba(0, 0, 0, 0.14),
        0 6px 30px 5px rgba(0, 0, 0, 0.12);
      transform: translateX(-105%);
      transition: transform 200ms ease;
      z-index: 200;
      overflow: hidden;
    }
    .modal-drawer.open {
      transform: translateX(0);
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "ha-drawer": HaDrawer;
  }
}
