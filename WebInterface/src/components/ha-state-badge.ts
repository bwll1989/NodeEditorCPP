import { css, html, LitElement, nothing } from "lit";
import { customElement, property } from "lit/decorators.js";
import { classMap } from "lit/directives/class-map.js";
import { styleMap } from "lit/directives/style-map.js";
import { computeCssColor } from "../common/color/theme-colors";
import { DEFAULT_BADGE_VISUAL_SIZE } from "../panels/lovelace/heading-badges/badge-flags";
import "./ha-icon";

/** 与标题行状态徽章一致的横向胶囊。 */
@customElement("ha-state-badge")
export class HaStateBadge extends LitElement {
  @property() public value = "—";

  @property() public unit = "";

  @property() public label = "";

  @property() public icon = "";

  @property() public color = "";

  @property({ type: Boolean }) public showValue = true;

  @property({ type: Boolean }) public showIcon = false;

  /** 胶囊高度（px），与标题徽章一致默认 26。 */
  @property({ type: Number }) public size = DEFAULT_BADGE_VISUAL_SIZE;

  @property({ type: Boolean }) public showBackground = true;

  protected render() {
    const color = this.color
      ? computeCssColor(this.color)
      : "var(--state-icon-color, var(--primary-color))";
    const height = Math.min(80, Math.max(18, this.size || DEFAULT_BADGE_VISUAL_SIZE));
    const iconSize = Math.max(12, Math.round((height * 16) / 26));
    const fontSize = Math.max(11, Math.round((height * 12) / 26));

    return html`
      <div
        class=${classMap({
          badge: true,
          transparent: !this.showBackground,
          "with-text": this.showValue || Boolean(this.unit),
        })}
        style=${styleMap({
          "--badge-color": color,
          "--badge-height": `${height}px`,
          "--badge-icon-size": `${iconSize}px`,
          "--badge-font-size": `${fontSize}px`,
        })}
      >
        ${this.showIcon && this.icon ? html`<ha-icon .icon=${this.icon}></ha-icon>` : nothing}
        ${this.showValue ? html`<span class="value">${this.value}</span>` : nothing}
        ${this.showValue && this.unit ? html`<span class="unit">${this.unit}</span>` : nothing}
      </div>
      ${this.label ? html`<div class="label">${this.label}</div>` : nothing}
    `;
  }

  static styles = css`
    :host {
      display: inline-flex;
      flex-direction: column;
      align-items: center;
      gap: 2px;
    }
    .badge {
      display: inline-flex;
      flex-direction: row;
      align-items: center;
      justify-content: center;
      gap: 2px;
      height: var(--badge-height, 26px);
      min-width: var(--badge-height, 26px);
      padding: 0;
      border: none;
      border-radius: 999px;
      box-sizing: border-box;
      background: color-mix(in srgb, var(--primary-text-color) 8%, transparent);
      color: var(--badge-color, var(--secondary-text-color));
      font-size: var(--badge-font-size, 12px);
      font-weight: 500;
      line-height: 1;
      white-space: nowrap;
      user-select: none;
    }
    .badge.with-text {
      padding: 0 8px;
    }
    .badge.transparent {
      background: transparent;
    }
    .value {
      padding: 0 2px;
      max-width: 96px;
      overflow: hidden;
      text-overflow: ellipsis;
    }
    .unit {
      padding: 0 2px 0 0;
      opacity: 0.85;
      font-weight: 600;
    }
    ha-icon {
      --mdc-icon-size: var(--badge-icon-size, 16px);
      color: var(--badge-color, inherit);
    }
    .label {
      font-size: 11px;
      font-weight: 500;
      color: var(--primary-text-color);
      max-width: 100px;
      overflow: hidden;
      text-overflow: ellipsis;
      white-space: nowrap;
      text-align: center;
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "ha-state-badge": HaStateBadge;
  }
}
