import { css, html, LitElement, nothing } from "lit";
import { property, state } from "lit/decorators.js";
import { classMap } from "lit/directives/class-map.js";
import { styleMap } from "lit/directives/style-map.js";
import type { Flow, LovelaceCard, LovelaceCardConfig, LovelaceGridOptions } from "../../../types";
import {
  contrastingIconColor,
  DEFAULT_RGBA,
  formatHsvaDisplay,
  formatRgbaDisplay,
  hsvaToFlowValue,
  hsvaToRgba,
  parseHsvaValue,
  parseRgbaValue,
  rgbaToCss,
  rgbaToFlowValue,
  rgbaToHsva,
  type HsvaColor,
  type RgbaColor,
} from "../../../common/entity/color";
import { tileCardHostStyle, tileCardStyle } from "../../../styles/tile-card-style";
import "../../../components/ha-tile-container";
import "../../../components/ha-tile-icon";
import "../../../components/ha-tile-info";
import "../../../components/ha-control-color-pad";
import "../../../components/ha-control-slider";
import "../../../components/ha-card";

export type ColorCardMode = "rgba" | "hsv";

/** Shared tile + HS pad UI for RGBA / HSV color cards. */
export abstract class HuiColorCardBase extends LitElement implements LovelaceCard {
  abstract readonly colorMode: ColorCardMode;

  abstract readonly stubEntity: string;

  abstract readonly stubName: string;

  abstract readonly stubIcon: string;

  @property({ attribute: false }) public flow?: Flow;

  @state() protected _config?: LovelaceCardConfig;

  @state() private _localHsva?: HsvaColor;

  setConfig(config: LovelaceCardConfig): void {
    this._config = config;
    this._localHsva = undefined;
    this.requestUpdate();
  }

  getGridOptions(): LovelaceGridOptions {
    const vertical = Boolean(this._config?.vertical);
    return {
      columns: 6,
      rows: 5,
      min_columns: vertical ? 3 : 6,
      min_rows: 5,
    };
  }

  private _entityState() {
    const entity = this._config?.entity;
    if (!entity || !this.flow) return undefined;
    return this.flow.states[entity];
  }

  private _remoteHsva(): HsvaColor {
    const state = this._entityState()?.state;
    if (this.colorMode === "hsv") {
      return parseHsvaValue(state) ?? rgbaToHsva(DEFAULT_RGBA);
    }
    return rgbaToHsva(parseRgbaValue(state) ?? DEFAULT_RGBA);
  }

  private _hsva(): HsvaColor {
    return this._localHsva ?? this._remoteHsva();
  }

  private _rgba(): RgbaColor {
    return hsvaToRgba(this._hsva());
  }

  private async _send(hsva: HsvaColor): Promise<void> {
    const entity = this._config?.entity;
    if (!entity || !this.flow) return;
    if (this.colorMode === "hsv") {
      await this.flow.callService(entity, hsvaToFlowValue(hsva));
      return;
    }
    await this.flow.callService(entity, rgbaToFlowValue(hsvaToRgba(hsva)));
  }

  private async _onPadChanged(
    ev: CustomEvent<{ rgba: RgbaColor; hsva: HsvaColor }>,
  ): Promise<void> {
    ev.stopPropagation();
    this._localHsva = ev.detail.hsva;
    await this._send(ev.detail.hsva);
  }

  private _onPadPointerUp(): void {
    window.setTimeout(() => {
      this._localHsva = undefined;
    }, 250);
  }

  private async _onValueChange(ev: CustomEvent<{ value: number }>): Promise<void> {
    ev.stopPropagation();
    const next = { ...this._hsva(), v: ev.detail.value };
    this._localHsva = next;
    await this._send(next);
  }

  private async _onAlphaChange(ev: CustomEvent<{ value: number }>): Promise<void> {
    ev.stopPropagation();
    const next = { ...this._hsva(), a: ev.detail.value };
    this._localHsva = next;
    await this._send(next);
  }

  protected render() {
    if (!this._config) return nothing;

    const entity = this._config.entity;
    const state = this._entityState();
    const hsva = this._hsva();
    const rgba = this._rgba();
    const vertical = Boolean(this._config.vertical);
    const isRgba = this.colorMode === "rgba";
    const name = String(
      this._config.name ?? state?.attributes?.friendly_name ?? entity ?? this.stubName,
    );
    const icon = String(this._config.icon ?? state?.attributes?.icon ?? this.stubIcon);
    const cssColor = rgbaToCss(rgba);
    const secondary =
      this.colorMode === "hsv" ? formatHsvaDisplay(hsva) : formatRgbaDisplay(rgba);
    const iconOnColor = contrastingIconColor(rgba);
    const cardStyle = styleMap({ "--tile-color": cssColor });
    const iconStyle = styleMap({
      "--tile-icon-color": cssColor,
      "--tile-icon-foreground": iconOnColor,
      "--tile-icon-opacity": "1",
    });
    const containerStyle = vertical
      ? styleMap({
          "--ha-tile-info-gap": "4px",
          "--ha-tile-info-min-height": "auto",
          "--ha-tile-info-primary-min-height": "auto",
          "--ha-tile-info-primary-line-height": "1.3",
        })
      : nothing;

    return html`
      <ha-card class=${classMap({ active: true })} style=${cardStyle}>
        <ha-tile-container ?vertical=${vertical} expand-features style=${containerStyle}>
          <ha-tile-icon
            slot="icon"
            style=${iconStyle}
            .icon=${icon}
            .active=${true}
          ></ha-tile-icon>
          <ha-tile-info slot="info" .primary=${name} .secondary=${secondary}></ha-tile-info>
          <div slot="features" class="features">
            <ha-control-color-pad
              .hue=${hsva.h}
              .saturation=${hsva.s}
              .value=${hsva.v}
              .alpha=${hsva.a}
              @value-changed=${this._onPadChanged}
              @pointerup=${this._onPadPointerUp}
              @click=${(ev: Event) => ev.stopPropagation()}
            ></ha-control-color-pad>
            <div class="sliders">
              ${isRgba
                ? html`
                    <div class="slider-row">
                      <span class="slider-label">透明度</span>
                      <ha-control-slider
                        .value=${hsva.a}
                        .min=${0}
                        .max=${1}
                        .step=${0.01}
                        show-handle
                        @value-changed=${this._onAlphaChange}
                        @click=${(ev: Event) => ev.stopPropagation()}
                      ></ha-control-slider>
                    </div>
                  `
                : html`
                    <div class="slider-row">
                      <span class="slider-label">明度</span>
                      <ha-control-slider
                        .value=${hsva.v}
                        .min=${0}
                        .max=${1}
                        .step=${0.01}
                        show-handle
                        @value-changed=${this._onValueChange}
                        @click=${(ev: Event) => ev.stopPropagation()}
                      ></ha-control-slider>
                    </div>
                  `}
            </div>
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
      .features {
        display: flex;
        flex-direction: column;
        gap: 8px;
        flex: 1 1 auto;
        min-height: 0;
        height: 100%;
        overflow: hidden;
        box-sizing: border-box;
      }
      ha-control-color-pad {
        flex: 1 1 120px;
        width: 100%;
        min-height: 120px;
        align-self: stretch;
      }
      .sliders {
        display: flex;
        flex-direction: column;
        gap: 4px;
        flex: 0 0 auto;
        --feature-height: 24px;
        --feature-border-radius: 8px;
      }
      .slider-row {
        display: grid;
        grid-template-columns: 3em 1fr;
        align-items: center;
        gap: 8px;
        min-height: var(--feature-height);
      }
      .slider-label {
        font-size: 11px;
        line-height: 1.2;
        color: var(--secondary-text-color);
        white-space: nowrap;
      }
    `,
  ];
}
