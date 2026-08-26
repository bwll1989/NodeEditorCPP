import { css, html, LitElement, nothing } from "lit";
import { customElement, property, state } from "lit/decorators.js";
import { classMap } from "lit/directives/class-map.js";
import type { ClockSize } from "../../common/datetime/clock-format";
import {
  formatClockDate,
  resolveLocale,
  resolveTimeZone,
  shouldShowDate,
  useAmPm,
} from "../../common/datetime/clock-format";

const INTERVAL = 1000;

/** Port of HA `hui-clock-card-digital` */
@customElement("ha-clock-digital")
export class HaClockDigital extends LitElement {
  @property({ type: String }) public clockSize: ClockSize = "small";

  @property({ type: Boolean }) public showSeconds = false;

  @property({ type: String }) public timeFormat?: string;

  @property({ type: String }) public timeZone?: string;

  @state() private _timeHour?: string;

  @state() private _timeMinute?: string;

  @state() private _timeSecond?: string;

  @state() private _timeAmPm?: string;

  @state() private _date = "";

  private _dateTimeFormat?: Intl.DateTimeFormat;

  private _tickInterval?: number;

  private _lastDateMinute?: string;

  connectedCallback(): void {
    super.connectedCallback();
    this._initFormatter();
    this._startTick();
  }

  disconnectedCallback(): void {
    this._stopTick();
    super.disconnectedCallback();
  }

  protected updated(changed: import("lit").PropertyValues): void {
    if (
      changed.has("timeFormat") ||
      changed.has("timeZone") ||
      changed.has("showSeconds") ||
      changed.has("clockSize")
    ) {
      this._initFormatter();
      this._tick();
    }
  }

  private _initFormatter(): void {
    const h12 = useAmPm(this.timeFormat);
    this._dateTimeFormat = new Intl.DateTimeFormat(resolveLocale(), {
      hour: h12 ? "numeric" : "2-digit",
      minute: "2-digit",
      second: "2-digit",
      hourCycle: h12 ? "h12" : "h23",
      timeZone: resolveTimeZone(this.timeZone),
    });
    this._lastDateMinute = undefined;
  }

  private _startTick(): void {
    this._stopTick();
    this._tick();
    this._tickInterval = window.setInterval(() => this._tick(), INTERVAL);
  }

  private _stopTick(): void {
    if (this._tickInterval) {
      clearInterval(this._tickInterval);
      this._tickInterval = undefined;
    }
  }

  private _tick(): void {
    if (!this._dateTimeFormat) return;

    const date = new Date();
    const parts = this._dateTimeFormat.formatToParts(date);

    this._timeHour = parts.find((part) => part.type === "hour")?.value;
    this._timeMinute = parts.find((part) => part.type === "minute")?.value;
    this._timeSecond = this.showSeconds
      ? parts.find((part) => part.type === "second")?.value
      : undefined;
    this._timeAmPm = parts.find((part) => part.type === "dayPeriod")?.value;

    this._updateDate(date);
  }

  private _updateDate(date: Date): void {
    if (!shouldShowDate(this.clockSize)) {
      this._date = "";
      this._lastDateMinute = undefined;
      return;
    }

    if (
      this._timeMinute !== undefined &&
      this._timeMinute === this._lastDateMinute &&
      this._date
    ) {
      return;
    }

    this._date = formatClockDate(date, this.clockSize, this.timeZone, resolveLocale());
    this._lastDateMinute = this._timeMinute;
  }

  protected render() {
    const sizeClass = this.clockSize ? `size-${this.clockSize}` : "";
    const showDate = Boolean(this._date);

    return html`
      <div class="clock-container">
        <div class=${classMap({ "time-parts": true, [sizeClass]: Boolean(sizeClass) })}>
          <span class="time-part hour">${this._timeHour ?? "--"}</span>
          <span class="time-part minute">${this._timeMinute ?? "--"}</span>
          ${this._timeSecond !== undefined
            ? html`<span class="time-part second">${this._timeSecond}</span>`
            : nothing}
          ${this._timeAmPm !== undefined
            ? html`<span class="time-part am-pm">${this._timeAmPm}</span>`
            : nothing}
        </div>
      </div>
      ${showDate
        ? html`
            <div class="date-container">
              <div class=${classMap({ date: true, [sizeClass]: Boolean(sizeClass) })}>
                ${this._date}
              </div>
            </div>
          `
        : nothing}
    `;
  }

  static styles = css`
    :host {
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      width: 100%;
      height: 100%;
      min-height: 0;
      box-sizing: border-box;
    }
    .clock-container {
      width: 100%;
      display: flex;
      justify-content: center;
    }
    .date-container {
      width: 100%;
      margin-top: var(--ha-space-1, 4px);
    }
    .time-parts {
      align-items: center;
      display: inline-grid;
      grid-template-areas:
        "hour minute second"
        "hour minute am-pm";
      font-size: 1.5rem;
      font-weight: var(--ha-font-weight-medium, 500);
      line-height: 0.8;
      direction: ltr;
      color: var(--primary-text-color);
      font-variant-numeric: tabular-nums;
    }
    .time-parts.size-medium {
      font-size: 3rem;
    }
    .time-parts.size-large {
      font-size: 4rem;
    }
    .time-parts.size-medium .time-part.second,
    .time-parts.size-medium .time-part.am-pm {
      font-size: var(--ha-font-size-l, 18px);
      margin-left: 6px;
    }
    .time-parts.size-large .time-part.second,
    .time-parts.size-large .time-part.am-pm {
      font-size: var(--ha-font-size-2xl, 28px);
      margin-left: 8px;
    }
    .time-part.hour {
      grid-area: hour;
    }
    .time-part.minute {
      grid-area: minute;
    }
    .time-part.second {
      grid-area: second;
      line-height: 0.9;
      opacity: 0.4;
      font-size: var(--ha-font-size-xs, 11px);
      margin-left: 4px;
    }
    .time-part.am-pm {
      grid-area: am-pm;
      line-height: 0.9;
      opacity: 0.6;
      font-size: var(--ha-font-size-xs, 11px);
      margin-left: 4px;
      text-transform: uppercase;
    }
    .time-part.hour::after {
      content: ":";
      margin: 0 2px;
    }
    .date {
      margin-inline: auto;
      text-align: center;
      opacity: 0.8;
      font-size: var(--ha-font-size-s, 12px);
      line-height: 1.1;
      overflow: hidden;
      white-space: nowrap;
      width: 100%;
      color: var(--primary-text-color);
    }
    .date.size-medium {
      font-size: var(--ha-font-size-l, 18px);
    }
    .date.size-large {
      font-size: var(--ha-font-size-2xl, 28px);
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "ha-clock-digital": HaClockDigital;
  }
}
