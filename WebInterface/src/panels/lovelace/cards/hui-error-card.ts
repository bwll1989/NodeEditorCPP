import { css, html, LitElement } from "lit";
import { customElement, property } from "lit/decorators.js";
import type { Flow, LovelaceCard, LovelaceCardConfig, LovelaceGridOptions } from "../../../types";

@customElement("hui-error-card")
export class HuiErrorCard extends LitElement implements LovelaceCard {
  @property({ attribute: false }) public flow?: Flow;

  private _config?: LovelaceCardConfig;

  setConfig(config: LovelaceCardConfig): void {
    this._config = config;
  }

  getGridOptions(): LovelaceGridOptions {
    return { columns: "full", rows: 2 };
  }

  protected render() {
    const message = String(this._config?.message ?? "Unknown error");
    return html`
      <ha-card>
        <div class="error">${message}</div>
      </ha-card>
    `;
  }

  static styles = css`
    .error {
      padding: 16px;
      color: var(--error-color, #b91c1c);
      font-size: 14px;
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "hui-error-card": HuiErrorCard;
  }
}
