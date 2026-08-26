import { css, html, LitElement, nothing } from "lit";
import { customElement, property } from "lit/decorators.js";
import { styleMap } from "lit/directives/style-map.js";
import { computeCssColor } from "../common/color/theme-colors";
import "./ha-icon";

@customElement("ha-state-badge")
export class HaStateBadge extends LitElement {
  @property() public value = "—";

  @property() public unit = "";

  @property() public label = "";

  @property() public icon = "";

  @property() public color = "";

  @property({ type: Boolean }) public showValue = true;

  @property({ type: Boolean }) public showIcon = false;

  protected render() {
    const color = this.color ? computeCssColor(this.color) : "var(--label-badge-red, #df4c1e)";
    return html`
      <div class="wrap" style=${styleMap({ "--badge-color": color })}>
        <div class="badge">
          ${this.showIcon && this.icon ? html`<ha-icon .icon=${this.icon}></ha-icon>` : nothing}
          ${this.showValue
            ? html`
                <span class="value">${this.value}</span>
                ${this.unit ? html`<span class="unit">${this.unit}</span>` : nothing}
              `
            : nothing}
        </div>
        ${this.label ? html`<div class="label">${this.label}</div>` : nothing}
      </div>
    `;
  }

  static styles = css`
    :host {
      display: inline-flex;
    }
    .wrap {
      display: flex;
      flex-direction: column;
      align-items: center;
      gap: 4px;
    }
    .badge {
      width: 56px;
      height: 56px;
      border-radius: 50%;
      box-sizing: border-box;
      border: 2px solid var(--badge-color, var(--label-badge-red, #df4c1e));
      background: var(--card-background-color, #fff);
      color: var(--primary-text-color);
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      gap: 2px;
      box-shadow: 0 1px 2px rgba(0, 0, 0, 0.12);
    }
    .value {
      font-size: 15px;
      font-weight: 600;
      line-height: 1.1;
      max-width: 50px;
      overflow: hidden;
      text-overflow: ellipsis;
      white-space: nowrap;
      text-align: center;
    }
    .unit {
      margin-top: 1px;
      font-size: 10px;
      font-weight: 600;
      line-height: 1;
      color: var(--badge-color, var(--label-badge-red, #df4c1e));
      background: color-mix(in srgb, var(--badge-color, #df4c1e) 16%, transparent);
      border-radius: 999px;
      padding: 1px 5px;
      max-width: 48px;
      overflow: hidden;
      text-overflow: ellipsis;
      white-space: nowrap;
    }
    ha-icon {
      --mdc-icon-size: 20px;
      color: var(--badge-color, var(--label-badge-red, #df4c1e));
    }
    .label {
      font-size: 12px;
      font-weight: 500;
      color: var(--primary-text-color);
      max-width: 88px;
      overflow: hidden;
      text-overflow: ellipsis;
      white-space: nowrap;
      text-align: center;
      text-shadow: 0 0 4px var(--card-background-color, #fff);
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "ha-state-badge": HaStateBadge;
  }
}
