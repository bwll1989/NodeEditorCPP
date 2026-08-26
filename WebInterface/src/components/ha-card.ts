import { css, html, LitElement } from "lit";
import { customElement } from "lit/decorators.js";

@customElement("ha-card")
export class HaCard extends LitElement {
  protected render() {
    return html`<slot></slot>`;
  }

  static styles = css`
    :host {
      display: block;
      background: var(--card-background-color, #fff);
      border-radius: var(--ha-card-border-radius, 12px);
      border: var(--ha-card-border-width, 1px) solid var(--divider-color);
      box-shadow: var(--ha-card-box-shadow);
      overflow: var(--ha-card-overflow, hidden);
      height: 100%;
      box-sizing: border-box;
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "ha-card": HaCard;
  }
}
