import { css, html, LitElement, nothing } from "lit";
import { customElement, property, state } from "lit/decorators.js";
import { classMap } from "lit/directives/class-map.js";
import type { Flow, LovelaceCard, LovelaceCardConfig, LovelaceGridOptions } from "../../../types";
import type { ClockSize, ClockStyle, ClockTicks } from "../../../common/datetime/clock-format";
import "../../../components/ha-card";
import "../../../components/clock/ha-clock-digital";
import "../../../components/clock/ha-clock-analog";

@customElement("hui-clock-card")
export class HuiClockCard extends LitElement implements LovelaceCard {
  public static getStubConfig(): LovelaceCardConfig {
    return {
      type: "clock",
      clock_style: "digital",
      clock_size: "small",
      time_format: "24",
    };
  }

  @property({ attribute: false }) public flow?: Flow;

  @state() private _config?: LovelaceCardConfig;

  setConfig(config: LovelaceCardConfig): void {
    this._config = {
      clock_style: "digital",
      clock_size: "small",
      ...config,
    };
  }

  getGridOptions(): LovelaceGridOptions {
    const style = String(this._config?.clock_style ?? "digital") as ClockStyle;
    const size = String(this._config?.clock_size ?? "small") as ClockSize;
    const hasTitle = this._config?.title !== undefined && this._config?.title !== "";

    if (style === "analog") {
      switch (size) {
        case "medium":
          return {
            columns: 6,
            rows: hasTitle ? 4 : 3,
            min_columns: 5,
            min_rows: hasTitle ? 4 : 3,
          };
        case "large":
          return {
            columns: 6,
            rows: hasTitle ? 5 : 4,
            min_columns: 6,
            min_rows: hasTitle ? 5 : 4,
          };
        default:
          return {
            columns: 6,
            rows: hasTitle ? 3 : 2,
            min_columns: 2,
            min_rows: hasTitle ? 3 : 2,
          };
      }
    }

    switch (size) {
      case "medium":
        return {
          columns: 6,
          rows: hasTitle ? 2 : 1,
          min_columns: 4,
          min_rows: hasTitle ? 2 : 1,
          max_rows: 4,
        };
      case "large":
        return {
          columns: 6,
          rows: 2,
          min_columns: 6,
          min_rows: 2,
          max_rows: 4,
        };
      default:
        return {
          columns: 6,
          rows: hasTitle ? 2 : 1,
          min_columns: 3,
          min_rows: 1,
          max_rows: 4,
        };
    }
  }

  protected render() {
    if (!this._config) return nothing;

    const style = String(this._config.clock_style ?? "digital") as ClockStyle;
    const size = String(this._config.clock_size ?? "small") as ClockSize;
    const title = this._config.title !== undefined ? String(this._config.title) : undefined;
    const noBackground = Boolean(this._config.no_background);
    const showSeconds = Boolean(this._config.show_seconds);
    const timeFormat = this._config.time_format ? String(this._config.time_format) : undefined;
    const timeZone = this._config.time_zone ? String(this._config.time_zone) : undefined;
    const analogOptions = (this._config.analog_options ?? {}) as Record<string, unknown>;
    const ticks = String(analogOptions.ticks ?? this._config.analog_ticks ?? "hour") as ClockTicks;
    const border = analogOptions.border !== undefined
      ? Boolean(analogOptions.border)
      : this._config.analog_border !== undefined
        ? Boolean(this._config.analog_border)
        : true;
    const faceStyle = String(
      analogOptions.face_style ?? this._config.analog_face_style ?? "markers",
    ) as "markers" | "numbers" | "roman";

    return html`
      <ha-card class=${classMap({ "no-background": noBackground })}>
        <div class=${classMap({ "time-wrapper": true, [`size-${size}`]: true })}>
          ${title !== undefined
            ? html`<div class="time-title">${title}</div>`
            : nothing}
          ${style === "analog"
            ? html`
                <ha-clock-analog
                  .clockSize=${size}
                  .showSeconds=${showSeconds}
                  .timeZone=${timeZone}
                  .border=${border}
                  .ticks=${ticks}
                  .faceStyle=${faceStyle}
                ></ha-clock-analog>
              `
            : html`
                <ha-clock-digital
                  .clockSize=${size}
                  .showSeconds=${showSeconds}
                  .timeFormat=${timeFormat}
                  .timeZone=${timeZone}
                ></ha-clock-digital>
              `}
        </div>
      </ha-card>
    `;
  }

  static styles = css`
    :host {
      display: block;
      height: 100%;
      min-height: 0;
    }
    ha-card {
      height: 100%;
      min-height: 0;
      display: flex;
      flex-direction: column;
      cursor: default;
      overflow: hidden;
    }
    ha-card.no-background {
      background: none;
      box-shadow: none;
      border: none;
    }
    .time-wrapper {
      flex: 1;
      min-height: 0;
      width: 100%;
      display: flex;
      align-items: center;
      justify-content: center;
      flex-direction: column;
      padding: 6px 8px;
      row-gap: 6px;
      box-sizing: border-box;
    }
    .time-wrapper.size-medium,
    .time-wrapper.size-large {
      padding: 16px;
      row-gap: var(--ha-space-3, 12px);
    }
    .time-title {
      color: var(--primary-text-color);
      font-size: var(--ha-font-size-m, 14px);
      font-weight: var(--ha-font-weight-normal, 400);
      line-height: var(--ha-line-height-condensed, 1.25);
      overflow: hidden;
      text-align: center;
      text-overflow: ellipsis;
      white-space: nowrap;
      width: 100%;
      flex: none;
    }
    .time-wrapper.size-medium .time-title {
      font-size: var(--ha-font-size-l, 18px);
    }
    .time-wrapper.size-large .time-title {
      font-size: var(--ha-font-size-2xl, 28px);
    }
    ha-clock-analog,
    ha-clock-digital {
      flex: 1;
      min-height: 0;
      width: 100%;
      display: flex;
      align-items: center;
      justify-content: center;
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "hui-clock-card": HuiClockCard;
  }
}
