import { css, html, LitElement, nothing } from "lit";
import { customElement, property, state } from "lit/decorators.js";
import { classMap } from "lit/directives/class-map.js";
import { styleMap } from "lit/directives/style-map.js";
import type { Flow, LovelaceCard, LovelaceCardConfig, LovelaceGridOptions } from "../../../types";
import { stateActive } from "../../../common/entity/state-active";
import { computeBrightnessTileColor } from "../../../common/entity/tile-color";
import { tileCardHostStyle, tileCardStyle } from "../../../styles/tile-card-style";
import "../../../components/ha-tile-container";
import "../../../components/ha-tile-icon";
import "../../../components/ha-tile-info";
import "../../../components/ha-control-slider";
import "../../../components/ha-card";

@customElement("hui-slider-card")
export class HuiSliderCard extends LitElement implements LovelaceCard {
  public static getStubConfig(): LovelaceCardConfig {
    return {
      type: "slider",
      entity: "/demo/spotlights",
      name: "Spotlights",
      icon: "mdi:spotlight-beam",
      min: 0,
      max: 100,
      step: 1,
    };
  }

  @property({ attribute: false }) public flow?: Flow;

  @state() private _config?: LovelaceCardConfig;

  setConfig(config: LovelaceCardConfig): void {
    this._config = config;
    this.requestUpdate();
  }

  getGridOptions(): LovelaceGridOptions {
    const vertical = Boolean(this._config?.vertical);
    // HA: 1 content row + 1 feature row; vertical layout adds another row.
    const rows = vertical ? 3 : 2;
    return {
      columns: 6,
      rows,
      min_columns: vertical ? 3 : 6,
      min_rows: rows,
    };
  }

  private _value(): number {
    const entity = this._config?.entity;
    if (!entity || !this.flow) return 0;
    const v = this.flow.states[entity]?.state;
    return typeof v === "number" ? v : Number(v) || 0;
  }

  private _formatValue(value: number): string {
    const max = Number(this._config?.max ?? 100);
    const min = Number(this._config?.min ?? 0);
    const rounded = Math.round(value);
    if (max === 100 && min === 0) {
      return `${rounded}%`;
    }
    return String(rounded);
  }

  private async _onSliderChange(ev: CustomEvent<{ value: number }>): Promise<void> {
    ev.stopPropagation();
    const entity = this._config?.entity;
    if (!entity || !this.flow) return;
    await this.flow.callService(entity, ev.detail.value);
  }

  protected render() {
    if (!this._config) return nothing;

    const entity = this._config.entity;
    const state = entity && this.flow ? this.flow.states[entity] : undefined;
    const min = Number(this._config.min ?? 0);
    const max = Number(this._config.max ?? 100);
    const step = Number(this._config.step ?? 1);
    const value = this._value();
    const active = stateActive(state) || value > min;
    const vertical = Boolean(this._config.vertical);
    const name = String(
      this._config.name ?? state?.attributes?.friendly_name ?? entity ?? "Slider",
    );
    const icon = String(this._config.icon ?? state?.attributes?.icon ?? "mdi:spotlight-beam");
    const stateText = this._formatValue(value);

    const tileColor = computeBrightnessTileColor(active, this._config.color);
    const colorStyle = styleMap({
      "--tile-color": tileColor,
    });
    const featureStyle = styleMap({
      "--feature-color": tileColor ?? (active ? "var(--state-icon-color)" : "var(--state-inactive-color)"),
    });

    return html`
      <ha-card class=${classMap({ active })} style=${colorStyle}>
        <ha-tile-container ?vertical=${vertical} ?fixed-info-height=${vertical}>
          <ha-tile-icon slot="icon" .icon=${icon} .active=${active}></ha-tile-icon>
          <ha-tile-info slot="info" .primary=${name} .secondary=${stateText}></ha-tile-info>
          <div slot="features" class="feature" style=${featureStyle}>
            <ha-control-slider
              .value=${value}
              .min=${min}
              .max=${max}
              .step=${step}
              show-handle
              round-value
              @value-changed=${this._onSliderChange}
              @click=${(ev: Event) => ev.stopPropagation()}
            ></ha-control-slider>
          </div>
        </ha-tile-container>
      </ha-card>
    `;
  }

  static styles = [
    tileCardStyle,
    tileCardHostStyle,
    css`
      :host {
        display: block;
        height: 100%;
        min-height: 0;
        --feature-height: 40px;
        --feature-border-radius: var(--ha-card-border-radius, 12px);
      }
      ha-card {
        --feature-color: var(--tile-color);
        height: 100%;
        min-height: 0;
        display: flex;
        flex-direction: column;
        cursor: default;
        overflow: hidden;
      }
      ha-tile-container {
        flex: 1;
        min-height: 0;
      }
      ha-card:hover {
        box-shadow: var(--ha-card-box-shadow);
      }
      .feature {
        box-sizing: border-box;
        --feature-color: var(--tile-color);
      }
    `,
  ];
}

declare global {
  interface HTMLElementTagNameMap {
    "hui-slider-card": HuiSliderCard;
  }
}
