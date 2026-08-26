import { css, html, LitElement, nothing } from "lit";
import { customElement, property } from "lit/decorators.js";
import { resolveIconPath } from "./ha-icon";
import "./ha-icon";

@customElement("ha-icon-button")
export class HaIconButton extends LitElement {
  @property() public path = "";

  @property() public label = "";

  @property({ type: Boolean }) public disabled = false;

  protected render() {
    const iconPath = this.path || resolveIconPath("mdi:help-circle-outline");
    return html`
      <button
        type="button"
        class="button"
        ?disabled=${this.disabled}
        aria-label=${this.label || nothing}
        title=${this.label || nothing}
      >
        <ha-svg-icon .path=${iconPath}></ha-svg-icon>
      </button>
    `;
  }

  static styles = css`
    :host {
      display: inline-flex;
    }
    .button {
      display: inline-flex;
      align-items: center;
      justify-content: center;
      width: 40px;
      height: 40px;
      border: none;
      border-radius: 50%;
      background: transparent;
      color: inherit;
      cursor: pointer;
      padding: 0;
    }
    .button:hover:not(:disabled) {
      background: color-mix(in srgb, currentColor 8%, transparent);
    }
    .button:disabled {
      opacity: 0.38;
      cursor: default;
    }
    ha-svg-icon {
      width: 24px;
      height: 24px;
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "ha-icon-button": HaIconButton;
  }
}
