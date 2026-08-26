import { css, html, LitElement, nothing } from "lit";
import { customElement, property } from "lit/decorators.js";
import { mdiMenu } from "@mdi/js";
import { fireEvent } from "../common/dom/fire_event";
import "./ha-icon-button";

/** Port of HA `ha-menu-button` — only visible in narrow / modal sidebar. */
@customElement("ha-menu-button")
export class HaMenuButton extends LitElement {
  @property({ type: Boolean }) public narrow = false;

  @property({ type: Boolean }) public hasNotifications = false;

  protected render() {
    if (!this.narrow) return nothing;
    return html`
      <ha-icon-button
        .path=${mdiMenu}
        label="菜单"
        @click=${this._toggleMenu}
      ></ha-icon-button>
      ${this.hasNotifications ? html`<span class="dot"></span>` : nothing}
    `;
  }

  private _toggleMenu(): void {
    fireEvent(this, "hass-toggle-menu");
  }

  static styles = css`
    :host {
      position: relative;
      display: inline-flex;
    }
    .dot {
      pointer-events: none;
      position: absolute;
      background-color: var(--accent-color);
      width: 12px;
      height: 12px;
      top: 9px;
      right: 7px;
      border-radius: 50%;
      border: 2px solid var(--app-header-background-color, #fff);
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "ha-menu-button": HaMenuButton;
  }
}
