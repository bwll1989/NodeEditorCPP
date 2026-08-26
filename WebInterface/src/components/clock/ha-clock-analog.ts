import { css, html, LitElement, nothing } from "lit";
import { customElement, property, state } from "lit/decorators.js";
import { classMap } from "lit/directives/class-map.js";
import { styleMap } from "lit/directives/style-map.js";
import type { ClockSize, ClockTicks } from "../../common/datetime/clock-format";
import {
  formatClockDate,
  getZonedTimeParts,
  resolveLocale,
} from "../../common/datetime/clock-format";

const DATE_UPDATE_MS = 60_000;
const QUARTER_TICKS = [0, 1, 2, 3];
const HOUR_TICKS = Array.from({ length: 12 }, (_, i) => i);
const MINUTE_TICKS = Array.from({ length: 60 }, (_, i) => i);

export type AnalogFaceStyle = "markers" | "numbers" | "roman";

function romanize12HourClock(num: number): string {
  const numerals = ["", "I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX", "X", "XI", "XII"];
  if (num < 1 || num > 12) return "";
  return numerals[num] ?? "";
}

@customElement("ha-clock-analog")
export class HaClockAnalog extends LitElement {
  @property({ type: String }) public clockSize: ClockSize = "small";

  @property({ type: Boolean }) public showSeconds = false;

  @property({ type: String }) public timeZone?: string;

  @property({ type: Boolean }) public border = true;

  @property({ type: String }) public ticks: ClockTicks = "hour";

  @property({ type: String }) public faceStyle: AnalogFaceStyle = "markers";

  @state() private _hourOffsetSec = 0;

  @state() private _minuteOffsetSec = 0;

  @state() private _secondOffsetSec = 0;

  @state() private _date = "";

  private _dateTimer?: number;

  connectedCallback(): void {
    super.connectedCallback();
    document.addEventListener("visibilitychange", this._handleVisibilityChange);
    this._computeOffsets();
    this._updateDate();
    this._startDateTick();
  }

  disconnectedCallback(): void {
    document.removeEventListener("visibilitychange", this._handleVisibilityChange);
    this._stopDateTick();
    super.disconnectedCallback();
  }

  protected updated(changed: import("lit").PropertyValues): void {
    if (changed.has("timeZone") || changed.has("clockSize")) {
      this._computeOffsets();
      this._updateDate();
    }
  }

  private _handleVisibilityChange = (): void => {
    if (!document.hidden) {
      this._computeOffsets();
      this._updateDate();
    }
  };

  private _startDateTick(): void {
    this._stopDateTick();
    this._dateTimer = window.setInterval(() => this._updateDate(), DATE_UPDATE_MS);
  }

  private _stopDateTick(): void {
    if (this._dateTimer) {
      clearInterval(this._dateTimer);
      this._dateTimer = undefined;
    }
  }

  private _computeOffsets(): void {
    const now = new Date();
    const parts = getZonedTimeParts(now, this.timeZone, true);
    const ms = now.getMilliseconds();
    const secondsWithMs = parts.second + ms / 1000;
    const hour12 = parts.hour % 12;

    this._secondOffsetSec = secondsWithMs;
    this._minuteOffsetSec = parts.minute * 60 + secondsWithMs;
    this._hourOffsetSec = hour12 * 3600 + parts.minute * 60 + secondsWithMs;
  }

  private _updateDate(): void {
    this._date = formatClockDate(new Date(), this.clockSize, this.timeZone, resolveLocale());
  }

  private _renderIndicator(number?: number) {
    if (!number || this.faceStyle === "markers") return nothing;

    const label =
      this.faceStyle === "roman" ? romanize12HourClock(number) : String(number);

    return html`<div class="number">${label}</div>`;
  }

  private _renderTick(index: number, tickMode: ClockTicks) {
    let rotation = 0;
    let number: number | undefined;
    let tickClass = "tick";

    if (tickMode === "quarter") {
      rotation = index * 90;
      number = [12, 3, 6, 9][index];
      tickClass = "tick hour";
    } else if (tickMode === "hour") {
      rotation = index * 30;
      number = ((index + 11) % 12) + 1;
      tickClass = "tick hour";
    } else if (tickMode === "minute") {
      rotation = index * 6;
      if (index % 5 === 0) {
        number = ((index / 5 + 11) % 12) + 1;
        tickClass = "tick hour";
      } else {
        tickClass = "tick minute";
      }
    }

    return html`
      <div class=${tickClass} style=${`--tick-rotation: ${rotation}deg`}>
        <div
          class=${classMap({
            line: true,
            numbers: this.faceStyle === "numbers",
            roman: this.faceStyle === "roman",
          })}
        ></div>
        ${this._renderIndicator(number)}
      </div>
    `;
  }

  protected render() {
    const sizeClass = this.clockSize ? `size-${this.clockSize}` : "";
    const ticks = this.ticks ?? "hour";
    const showDate = Boolean(this._date);
    const handStyle = (offset: number) =>
      styleMap({
        animationDelay: `-${offset}s`,
      });

    return html`
      <div class=${classMap({ "analog-clock": true, [sizeClass]: Boolean(sizeClass) })}>
        <div class=${classMap({ dial: true, "dial-border": this.border })}>
          ${ticks === "quarter"
            ? QUARTER_TICKS.map((i) => this._renderTick(i, "quarter"))
            : ticks === "minute"
              ? MINUTE_TICKS.map((i) => this._renderTick(i, "minute"))
              : ticks === "none"
                ? nothing
                : HOUR_TICKS.map((i) => this._renderTick(i, "hour"))}
          ${showDate
            ? html`<div class=${classMap({ date: true, [sizeClass]: Boolean(sizeClass) })}>
                ${this._date}
              </div>`
            : nothing}
          <div class="center-dot"></div>
          <div class="hand hour" style=${handStyle(this._hourOffsetSec)}></div>
          <div class="hand minute" style=${handStyle(this._minuteOffsetSec)}></div>
          ${this.showSeconds
            ? html`<div class="hand second" style=${handStyle(this._secondOffsetSec)}></div>`
            : nothing}
        </div>
      </div>
    `;
  }

  static styles = css`
    :host {
      display: flex;
      align-items: center;
      justify-content: center;
      width: 100%;
      height: 100%;
      min-height: 0;
      box-sizing: border-box;
    }
    .analog-clock {
      --clock-size: 100px;
      display: inline-flex;
      align-items: center;
      justify-content: center;
      width: var(--clock-size);
      height: var(--clock-size);
    }
    .analog-clock.size-medium {
      --clock-size: 160px;
    }
    .analog-clock.size-large {
      --clock-size: 220px;
    }
    .dial {
      position: relative;
      width: 100%;
      height: 100%;
      box-sizing: border-box;
    }
    .dial-border {
      border: 2px solid var(--divider-color);
      border-radius: var(--ha-border-radius-circle, 50%);
    }
    .tick {
      position: absolute;
      top: 0;
      left: 0;
      width: 100%;
      height: 100%;
      transform: rotate(var(--tick-rotation));
      pointer-events: none;
      z-index: 0;
    }
    .tick .line {
      position: absolute;
      top: 0;
      left: 50%;
      transform: translateX(-50%);
      width: 1px;
      height: calc(var(--clock-size) * 0.04);
      background: var(--primary-text-color);
      opacity: 0.5;
      border-radius: 1px;
    }
    .tick.hour .line {
      width: 2px;
      height: calc(var(--clock-size) * 0.07);
      opacity: 0.8;
    }
    .tick.hour .line.numbers,
    .tick.hour .line.roman {
      height: calc(var(--clock-size) * 0.03);
    }
    .tick.minute .line {
      height: calc(var(--clock-size) * 0.015);
      opacity: 0.35;
    }
    .tick .number {
      position: absolute;
      top: 0;
      left: 50%;
      transform: translate(-50%, 35%);
      color: var(--primary-text-color);
      font-weight: var(--ha-font-weight-medium, 500);
      font-size: var(--ha-font-size-s, 12px);
      line-height: var(--ha-line-height-condensed, 1.25);
    }
    .center-dot {
      position: absolute;
      top: 50%;
      left: 50%;
      width: 8px;
      height: 8px;
      border-radius: var(--ha-border-radius-circle, 50%);
      background: var(--primary-text-color);
      transform: translate(-50%, -50%);
      z-index: 3;
    }
    .hand {
      position: absolute;
      left: 50%;
      bottom: 50%;
      transform-origin: 50% 100%;
      transform: translate(-50%, 0) rotate(0deg);
      background: var(--primary-text-color);
      border-radius: 2px;
      will-change: transform;
      animation-name: ha-clock-rotate;
      animation-timing-function: linear;
      animation-iteration-count: infinite;
    }
    .hand.hour {
      width: 4px;
      height: calc(var(--clock-size) * 0.25);
      box-shadow: 0 0 8px rgba(0, 0, 0, 0.2);
      z-index: 1;
      animation-duration: 43200s;
    }
    .hand.minute {
      width: 3px;
      height: calc(var(--clock-size) * 0.35);
      box-shadow: 0 0 6px rgba(0, 0, 0, 0.2);
      opacity: 0.9;
      z-index: 3;
      animation-duration: 3600s;
    }
    .hand.second {
      width: 2px;
      height: calc(var(--clock-size) * 0.42);
      background: var(--ha-color-border-danger-normal, var(--error-color, #db4437));
      box-shadow: 0 0 4px rgba(0, 0, 0, 0.2);
      opacity: 0.8;
      z-index: 2;
      animation-duration: 60s;
    }
    @keyframes ha-clock-rotate {
      from {
        transform: translate(-50%, 0) rotate(0deg);
      }
      to {
        transform: translate(-50%, 0) rotate(360deg);
      }
    }
    .date {
      position: absolute;
      top: 68%;
      left: 50%;
      transform: translate(-50%, -50%);
      display: block;
      color: var(--primary-text-color);
      font-size: var(--ha-font-size-s, 12px);
      font-weight: var(--ha-font-weight-medium, 500);
      line-height: var(--ha-line-height-condensed, 1.25);
      text-align: center;
      opacity: 0.8;
      overflow: hidden;
      white-space: nowrap;
      width: 100%;
    }
    .date.size-medium {
      font-size: var(--ha-font-size-l, 18px);
    }
    .date.size-large {
      font-size: var(--ha-font-size-xl, 22px);
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "ha-clock-analog": HaClockAnalog;
  }
}
