import { css, html, LitElement } from "lit";
import { customElement, property } from "lit/decorators.js";

@customElement("ha-tile-info")
export class HaTileInfo extends LitElement {
  @property() public primary = "";

  @property() public secondary = "";

  protected render() {
    return html`
      <div class="info">
        <div class="primary">
          <span>${this.primary}</span>
        </div>
        ${this.secondary
          ? html`
              <div class="secondary">
                <span>${this.secondary}</span>
              </div>
            `
          : ""}
      </div>
    `;
  }

  static styles = css`
    :host {
      display: block;
      width: 100%;
      min-width: 0;
    }
    .info {
      width: 100%;
      display: flex;
      flex-direction: column;
      align-items: var(--ha-tile-info-align-items, flex-start);
      justify-content: center;
      gap: var(--ha-tile-info-gap, 0);
      min-width: 0;
      min-height: var(--ha-tile-info-min-height, auto);
      text-align: var(--ha-tile-info-text-align, start);
    }
    .primary span,
    .secondary span {
      text-overflow: ellipsis;
      overflow: hidden;
      white-space: nowrap;
      width: 100%;
      display: block;
    }
    .primary {
      width: 100%;
      font-size: var(--ha-font-size-m, 14px);
      font-weight: var(--ha-font-weight-medium, 500);
      line-height: var(--ha-tile-info-primary-line-height, var(--ha-line-height-normal, 1.4));
      min-height: var(--ha-tile-info-primary-min-height, auto);
      letter-spacing: 0.1px;
      color: var(--primary-text-color);
    }
    .secondary {
      width: 100%;
      font-size: var(--ha-font-size-s, 12px);
      font-weight: var(--ha-font-weight-normal, 400);
      line-height: var(--ha-line-height-condensed, 1.25);
      letter-spacing: 0.4px;
      color: var(--ha-tile-info-secondary-color, var(--primary-text-color));
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "ha-tile-info": HaTileInfo;
  }
}
