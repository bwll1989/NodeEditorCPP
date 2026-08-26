import { css, html, LitElement } from "lit";
import { customElement, property } from "lit/decorators.js";

@customElement("ha-heading-badge")
export class HaHeadingBadge extends LitElement {
  @property() public type: "text" | "button" = "text";

  protected render() {
    return html`
      <div
        class="heading-badge"
        role=${this.type === "button" ? "button" : "group"}
        tabindex=${this.type === "button" ? "0" : "-1"}
      >
        <slot name="icon"></slot>
        <slot></slot>
      </div>
    `;
  }

  static styles = css`
    .heading-badge {
      display: inline-flex;
      flex-direction: row;
      align-items: center;
      gap: 4px;
      white-space: nowrap;
      color: var(--ha-heading-badge-text-color, var(--secondary-text-color));
      font-size: var(--ha-heading-badge-font-size, 14px);
      font-weight: 400;
      line-height: 20px;
      letter-spacing: 0.1px;
    }
    ::slotted([slot="icon"]) {
      display: flex;
      color: var(--icon-color, inherit);
      --mdc-icon-size: 16px;
    }
    :host([type="button"]) .heading-badge,
    .heading-badge[role="button"] {
      cursor: pointer;
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "ha-heading-badge": HaHeadingBadge;
  }
}
