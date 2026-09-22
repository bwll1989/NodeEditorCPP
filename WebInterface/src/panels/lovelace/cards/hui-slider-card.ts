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

  /**
   * 根据 step 在百分比空间的精度计算需要保留的小数位数。
   * 例：range=1  step=0.001 → pctStep=0.1 → 1 位小数
   *     range=100 step=1    → pctStep=1   → 0 位小数
   */
  private _percentFractionDigits(min: number, max: number, step: number): number {
    if (max - min === 0 || step <= 0) return 2;
    const pctStep = (Math.abs(step) / Math.abs(max - min)) * 100;
    if (pctStep <= 0) return 2;
    const digits = Math.max(0, Math.ceil(-Math.log10(pctStep)));
    return Math.min(digits, 6);
  }

  /**
   * 将 value 按 min/max 范围映射为 0-100 的百分比文本。
   * 任何 min/max 范围（包括 0-1、10-90、0-100）都统一显示百分比，
   * 小数精度匹配 step 在百分比空间的粒度。
   */
  private _formatValue(value: number): string {
    const max = Number(this._config?.max ?? 100);
    const min = Number(this._config?.min ?? 0);
    const step = Number(this._config?.step ?? 1);
    const bounded = Math.min(Math.max(value, min), max);
    const percent = max - min === 0 ? 0 : ((bounded - min) / (max - min)) * 100;
    const fracDigits = this._percentFractionDigits(min, max, step);
    // toFixed 后去掉末尾多余的 0 和孤立的小数点
    const text = percent.toFixed(fracDigits).replace(/\.0+$/, "").replace(/(\.\d*?)0+$/, "$1");
    return `${text}%`;
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
