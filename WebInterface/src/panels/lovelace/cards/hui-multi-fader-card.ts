import { css, html, LitElement, nothing } from "lit";
import { customElement, property, state } from "lit/decorators.js";
import { classMap } from "lit/directives/class-map.js";
import { styleMap } from "lit/directives/style-map.js";
import type { Flow, LovelaceCard, LovelaceCardConfig, LovelaceGridOptions } from "../../../types";
import {
  clampFaderCount,
  formatVectorDisplay,
  MULTI_FADER_DEFAULTS,
  parseFaderLabels,
  parseVectorValue,
  stepFaderValue,
  vectorToFlowValue,
} from "../../../common/entity/multi-fader";
import { computeFeatureColor } from "../../../common/entity/tile-color";
import { tileCardHostStyle, tileCardStyle } from "../../../styles/tile-card-style";
import "../../../components/ha-tile-container";
import "../../../components/ha-tile-icon";
import "../../../components/ha-tile-info";
import "../../../components/ha-control-slider";
import "../../../components/ha-card";

function configNumber(value: unknown, fallback: number): number {
  if (value === undefined || value === null || value === "") return fallback;
  const parsed = Number(value);
  return Number.isNaN(parsed) ? fallback : parsed;
}

@customElement("hui-multi-fader-card")
export class HuiMultiFaderCard extends LitElement implements LovelaceCard {
  public static getStubConfig(): LovelaceCardConfig {
    return {
      type: "multi-fader",
      entity: "/demo/faders",
      name: "多维推杆",
      icon: "mdi:tune-vertical-variant",
      count: MULTI_FADER_DEFAULTS.count,
      min: MULTI_FADER_DEFAULTS.min,
      max: MULTI_FADER_DEFAULTS.max,
      step: MULTI_FADER_DEFAULTS.step,
      labels: "0,1,2,3",
      orientation: "vertical",
    };
  }

  @property({ attribute: false }) public flow?: Flow;

  @state() private _config?: LovelaceCardConfig;

  @state() private _local?: number[];

  setConfig(config: LovelaceCardConfig): void {
    this._config = config;
    this._local = undefined;
    this.requestUpdate();
  }

  getGridOptions(): LovelaceGridOptions {
    const verticalLayout = Boolean(this._config?.vertical);
    const count = clampFaderCount(this._config?.count);
    const orientation = this._orientation();
    if (orientation === "vertical") {
      return {
        columns: Math.min(12, Math.max(6, count * 2)),
        rows: 4,
        min_columns: verticalLayout ? 3 : 6,
        min_rows: 3,
      };
    }
    const rows = Math.max(2, Math.min(8, 1 + Math.ceil(count / 2)));
    return {
      columns: 6,
      rows,
      min_columns: verticalLayout ? 3 : 6,
      min_rows: rows,
    };
  }

  private _orientation(): "vertical" | "horizontal" {
    return this._config?.orientation === "horizontal" ? "horizontal" : "vertical";
  }

  private _options() {
    return {
      count: clampFaderCount(this._config?.count),
      min: configNumber(this._config?.min, MULTI_FADER_DEFAULTS.min),
      max: configNumber(this._config?.max, MULTI_FADER_DEFAULTS.max),
      step: configNumber(this._config?.step, MULTI_FADER_DEFAULTS.step),
    };
  }

  private _values(): number[] {
    const { count, min, max, step } = this._options();
    if (this._local && this._local.length === count) return this._local;
    const entity = this._config?.entity;
    const state = entity && this.flow ? this.flow.states[entity]?.state : undefined;
    return parseVectorValue(state, count, { min, max, step });
  }

  private async _send(values: number[]): Promise<void> {
    const entity = this._config?.entity;
    if (!entity || !this.flow) return;
    await this.flow.callService(entity, vectorToFlowValue(values));
  }

  private async _onFaderChange(index: number, ev: CustomEvent<{ value: number }>): Promise<void> {
    ev.stopPropagation();
    const { count, min, max, step } = this._options();
    const next = [...this._values()];
    while (next.length < count) next.push(min);
    next[index] = stepFaderValue(ev.detail.value, step, min, max);
    this._local = next.slice(0, count);
    await this._send(this._local);
  }

  private _onPointerUp(): void {
    window.setTimeout(() => {
      this._local = undefined;
    }, 250);
  }

  protected render() {
    if (!this._config) return nothing;

    const entity = this._config.entity;
    const state = entity && this.flow ? this.flow.states[entity] : undefined;
    const { count, min, max, step } = this._options();
    const values = this._values();
    const labels = parseFaderLabels(this._config.labels, count);
    const orientation = this._orientation();
    const vertical = Boolean(this._config.vertical);
    const name = String(
      this._config.name ?? state?.attributes?.friendly_name ?? entity ?? "多维推杆",
    );
    const icon = String(
      this._config.icon ?? state?.attributes?.icon ?? "mdi:tune-vertical-variant",
    );
    const active = values.some((v) => v > min);
    const tileColor = computeFeatureColor(active, this._config.color, "primary");
    const colorStyle = styleMap({ "--tile-color": tileColor });
    const featureStyle = styleMap({
      "--feature-color": tileColor ?? "var(--primary-color)",
    });
    const digits = step < 1 ? 2 : 0;
    const hideState = Boolean(this._config.hide_state);
    const secondary = hideState ? "" : formatVectorDisplay(values, digits);

    return html`
      <ha-card class=${classMap({ active })} style=${colorStyle}>
        <ha-tile-container ?vertical=${vertical} expand-features>
          <ha-tile-icon slot="icon" .icon=${icon} .active=${active}></ha-tile-icon>
          <ha-tile-info
            slot="info"
            .primary=${name}
            .secondary=${secondary}
          ></ha-tile-info>
          <div
            slot="features"
            class=${classMap({
              faders: true,
              vertical: orientation === "vertical",
              horizontal: orientation === "horizontal",
            })}
            style=${featureStyle}
          >
            ${values.map(
              (value, index) => html`
                <div class="fader">
                  ${orientation === "horizontal"
                    ? html`<span class="fader-label">${labels[index]}</span>`
                    : nothing}
                  <ha-control-slider
                    .value=${value}
                    .min=${min}
                    .max=${max}
                    .step=${step}
                    ?vertical=${orientation === "vertical"}
                    show-handle
                    .roundValue=${step >= 1}
                    @value-changed=${(ev: CustomEvent<{ value: number }>) =>
                      this._onFaderChange(index, ev)}
                    @pointerup=${this._onPointerUp}
                    @click=${(ev: Event) => ev.stopPropagation()}
                  ></ha-control-slider>
                  ${orientation === "vertical"
                    ? html`<span class="fader-label">${labels[index]}</span>`
                    : nothing}
                </div>
              `,
            )}
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
      .faders {
        display: flex;
        box-sizing: border-box;
        min-height: 0;
        height: 100%;
        --feature-height: 28px;
        --feature-border-radius: 10px;
      }
      .faders.vertical {
        flex-direction: row;
        align-items: stretch;
        justify-content: space-evenly;
        gap: 6px;
        --feature-height: 28px;
      }
      .faders.horizontal {
        flex-direction: column;
        gap: 6px;
        --feature-height: 24px;
      }
      .fader {
        display: flex;
        min-width: 0;
        min-height: 0;
      }
      .faders.vertical .fader {
        flex: 1 1 0;
        flex-direction: column;
        align-items: center;
        gap: 4px;
      }
      .faders.vertical ha-control-slider {
        flex: 1 1 auto;
        min-height: 0;
        width: var(--feature-height);
      }
      .faders.horizontal .fader {
        flex-direction: row;
        align-items: center;
        gap: 8px;
      }
      .faders.horizontal ha-control-slider {
        flex: 1 1 auto;
        min-width: 0;
      }
      .fader-label {
        flex: 0 0 auto;
        font-size: 11px;
        line-height: 1.2;
        color: var(--secondary-text-color);
        text-align: center;
        white-space: nowrap;
        max-width: 100%;
        overflow: hidden;
        text-overflow: ellipsis;
      }
      .faders.horizontal .fader-label {
        width: 2.5em;
        text-align: left;
      }
    `,
  ];
}

declare global {
  interface HTMLElementTagNameMap {
    "hui-multi-fader-card": HuiMultiFaderCard;
  }
}
