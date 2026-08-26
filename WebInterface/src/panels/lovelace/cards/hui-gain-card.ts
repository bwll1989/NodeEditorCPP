import { css, html, LitElement, nothing } from "lit";
import { customElement, property, state } from "lit/decorators.js";
import { classMap } from "lit/directives/class-map.js";
import { styleMap } from "lit/directives/style-map.js";
import type { Flow, LovelaceCard, LovelaceCardConfig, LovelaceGridOptions } from "../../../types";
import {
  digitsForStep,
  formatGainSecondary,
  getGainMax,
  getGainMin,
  getGainStep,
  getGainValue,
} from "../../../common/entity/gain";
import { computeFeatureColor } from "../../../common/entity/tile-color";
import { tileCardHostStyle, tileCardStyle } from "../../../styles/tile-card-style";
import "../../../components/ha-tile-container";
import "../../../components/ha-tile-icon";
import "../../../components/ha-tile-info";
import "../../../components/ha-control-number-buttons";
import "../../../components/ha-icon";
import "../../../components/ha-card";

function configNumber(value: unknown, fallback: number): number {
  if (value === undefined || value === null || value === "") return fallback;
  const parsed = Number(value);
  return Number.isNaN(parsed) ? fallback : parsed;
}

@customElement("hui-gain-card")
export class HuiGainCard extends LitElement implements LovelaceCard {
  public static getStubConfig(): LovelaceCardConfig {
    return {
      type: "gain",
      entity: "/demo/gain",
      name: "输入增益",
      icon: "mdi:volume-high",
      min: -60,
      max: 12,
      step: 1,
    };
  }

  @property({ attribute: false }) public flow?: Flow;

  @state() private _config?: LovelaceCardConfig;

  /** Latched mute: click → min until clicked again to restore. */
  @state() private _muted = false;

  private _savedGain?: number;

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

  private _gainState() {
    const entity = this._config?.entity;
    if (!entity || !this.flow) return undefined;
    return this.flow.states[entity];
  }

  private _range() {
    const gainState = this._gainState();
    return {
      min: getGainMin(gainState, configNumber(this._config?.min, -60)),
      max: getGainMax(gainState, configNumber(this._config?.max, 12)),
      step: getGainStep(gainState, configNumber(this._config?.step, 1)),
    };
  }

  private async _onGainChange(ev: CustomEvent<{ value: number }>): Promise<void> {
    ev.stopPropagation();
    if (this._muted) return;
    const entity = this._config?.entity;
    if (!entity || !this.flow) return;
    await this.flow.callService(entity, ev.detail.value);
  }

  private async _toggleMute(ev: Event): Promise<void> {
    ev.stopPropagation();
    const entity = this._config?.entity;
    if (!entity || !this.flow) return;

    const { min } = this._range();

    if (this._muted) {
      const restore = this._savedGain ?? min;
      this._muted = false;
      this._savedGain = undefined;
      await this.flow.callService(entity, restore);
      return;
    }

    const current = getGainValue(this._gainState(), min);
    this._savedGain = current;
    this._muted = true;
    await this.flow.callService(entity, min);
  }

  protected render() {
    if (!this._config) return nothing;

    const entity = this._config.entity;
    const gainState = this._gainState();
    const muted = this._muted;
    const active = !muted;
    const vertical = Boolean(this._config.vertical);
    const name = String(
      this._config.name ?? gainState?.attributes?.friendly_name ?? entity ?? "增益",
    );
    const icon = String(this._config.icon ?? gainState?.attributes?.icon ?? "mdi:volume-high");
    const { min, max, step } = this._range();
    const remoteValue = getGainValue(gainState, min);
    const value = muted ? min : remoteValue;
    const digits = digitsForStep(step);
    const muteIcon = muted ? "mdi:volume-off" : "mdi:volume-high";
    const tileColor = computeFeatureColor(active, this._config.color, "primary");
    const colorStyle = styleMap({ "--tile-color": tileColor });
    const featureStyle = styleMap({
      "--feature-color": tileColor ?? (active ? "var(--primary-color)" : "var(--state-inactive-color)"),
    });

    return html`
      <ha-card class=${classMap({ active, muted })} style=${colorStyle}>
        <ha-tile-container ?vertical=${vertical} ?fixed-info-height=${vertical}>
          <ha-tile-icon slot="icon" .icon=${icon} .active=${active}></ha-tile-icon>
          <ha-tile-info
            slot="info"
            .primary=${name}
            .secondary=${formatGainSecondary(muted)}
          ></ha-tile-info>
          <div slot="features" class="feature-row" style=${featureStyle}>
            <ha-control-number-buttons
              class="gain-stepper"
              .value=${value}
              .min=${min}
              .max=${max}
              .step=${step}
              .disabled=${muted}
              unit="dB"
              .digits=${digits}
              @value-changed=${this._onGainChange}
              @click=${(ev: Event) => ev.stopPropagation()}
            ></ha-control-number-buttons>
            <button
              type="button"
              class="mute-btn"
              aria-pressed=${muted ? "true" : "false"}
              aria-label=${muted ? "取消静音" : "静音"}
              title=${muted ? "取消静音" : "静音"}
              @click=${this._toggleMute}
            >
              <ha-icon .icon=${muteIcon}></ha-icon>
            </button>
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
      .feature-row {
        display: flex;
        align-items: stretch;
        gap: 8px;
        box-sizing: border-box;
      }
      .gain-stepper {
        flex: 1;
        min-width: 0;
      }
      .mute-btn {
        flex: 0 0 var(--feature-height);
        width: var(--feature-height);
        height: var(--feature-height);
        appearance: none;
        border: none;
        border-radius: var(--feature-border-radius);
        background: color-mix(
          in srgb,
          var(--feature-color, var(--primary-color)) 20%,
          transparent
        );
        color: var(--feature-color, var(--primary-color));
        display: flex;
        align-items: center;
        justify-content: center;
        cursor: pointer;
        padding: 0;
        --mdc-icon-size: 22px;
      }
      .mute-btn ha-icon {
        display: flex;
        align-items: center;
        justify-content: center;
        width: var(--mdc-icon-size);
        height: var(--mdc-icon-size);
        color: inherit;
      }
      .mute-btn:hover {
        background: color-mix(
          in srgb,
          var(--feature-color, var(--primary-color)) 32%,
          transparent
        );
      }
      ha-card.muted .mute-btn {
        color: var(--state-inactive-color);
        --feature-color: var(--state-inactive-color);
      }
    `,
  ];
}

declare global {
  interface HTMLElementTagNameMap {
    "hui-gain-card": HuiGainCard;
  }
}
