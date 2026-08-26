import { css, html, LitElement, nothing } from "lit";
import { customElement, property, state } from "lit/decorators.js";
import { classMap } from "lit/directives/class-map.js";
import { styleMap } from "lit/directives/style-map.js";
import type { Flow, LovelaceCard, LovelaceCardConfig, LovelaceGridOptions } from "../../../types";
import {
  climateActive,
  formatClimateSecondary,
  getClimateMax,
  getClimateMin,
  getClimateStep,
  getClimateUnit,
  getTargetTemperature,
} from "../../../common/entity/climate";
import { computeFeatureColor } from "../../../common/entity/tile-color";
import { tileCardHostStyle, tileCardStyle } from "../../../styles/tile-card-style";
import "../../../components/ha-tile-container";
import "../../../components/ha-tile-icon";
import "../../../components/ha-tile-info";
import "../../../components/ha-control-number-buttons";
import "../../../components/ha-card";

function configNumber(value: unknown, fallback: number): number {
  if (value === undefined || value === null || value === "") return fallback;
  const parsed = Number(value);
  return Number.isNaN(parsed) ? fallback : parsed;
}

@customElement("hui-climate-card")
export class HuiClimateCard extends LitElement implements LovelaceCard {
  public static getStubConfig(): LovelaceCardConfig {
    return {
      type: "climate",
      entity: "/demo/climate",
      name: "Upstairs",
      icon: "mdi:home-thermometer",
      min: 0,
      max: 100,
      step: 1,
      unit: "°C",
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
    const rows = vertical ? 3 : 2;
    return {
      columns: 6,
      rows,
      min_columns: vertical ? 3 : 6,
      min_rows: rows,
    };
  }

  private _entityState() {
    const entity = this._config?.entity;
    if (!entity || !this.flow) return undefined;
    return this.flow.states[entity];
  }

  private async _onTargetChange(ev: CustomEvent<{ value: number }>): Promise<void> {
    ev.stopPropagation();
    const entity = this._config?.entity;
    if (!entity || !this.flow) return;
    await this.flow.callService(entity, ev.detail.value);
  }

  protected render() {
    if (!this._config) return nothing;

    const entity = this._config.entity;
    const state = this._entityState();
    const active = climateActive(state);
    const vertical = Boolean(this._config.vertical);
    const name = String(
      this._config.name ?? state?.attributes?.friendly_name ?? entity ?? "Climate",
    );
    const icon = String(this._config.icon ?? state?.attributes?.icon ?? "mdi:home-thermometer");
    const min = getClimateMin(state, configNumber(this._config.min, 0));
    const max = getClimateMax(state, configNumber(this._config.max, 100));
    const step = getClimateStep(state, configNumber(this._config.step, 1));
    const unit = getClimateUnit(state, String(this._config.unit ?? ""));
    const target = getTargetTemperature(state, min);
    const digits = step >= 1 ? 0 : 1;
    const tileColor = computeFeatureColor(active, this._config.color, "orange");
    const colorStyle = styleMap({ "--tile-color": tileColor });
    const featureStyle = styleMap({
      "--feature-color": tileColor ?? (active ? "var(--orange-color, #ff9800)" : "var(--state-inactive-color)"),
    });

    return html`
      <ha-card class=${classMap({ active })} style=${colorStyle}>
        <ha-tile-container ?vertical=${vertical} ?fixed-info-height=${vertical}>
          <ha-tile-icon slot="icon" .icon=${icon} .active=${active}></ha-tile-icon>
          <ha-tile-info
            slot="info"
            .primary=${name}
            .secondary=${formatClimateSecondary(state, unit)}
          ></ha-tile-info>
          <div slot="features" class="feature" style=${featureStyle}>
            <ha-control-number-buttons
              .value=${target}
              .min=${min}
              .max=${max}
              .step=${step}
              .unit=${unit}
              .digits=${digits}
              @value-changed=${this._onTargetChange}
              @click=${(ev: Event) => ev.stopPropagation()}
            ></ha-control-number-buttons>
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
      }
    `,
  ];
}

declare global {
  interface HTMLElementTagNameMap {
    "hui-climate-card": HuiClimateCard;
  }
}
