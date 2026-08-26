import { DIRECTION_ALL, Manager, Pan, Tap } from "@egjs/hammerjs";
import type { PropertyValues, TemplateResult } from "lit";
import { LitElement, css, html, nothing } from "lit";
import { customElement, property, query, state } from "lit/decorators.js";
import { classMap } from "lit/directives/class-map.js";
import { styleMap } from "lit/directives/style-map.js";

/** Port of HA `ha-grid-layout-slider` */
@customElement("ha-grid-layout-slider")
export class HaGridLayoutSlider extends LitElement {
  @property({ type: Boolean, reflect: true })
  public disabled = false;

  @property({ type: Boolean, reflect: true })
  public vertical = false;

  @property({ attribute: "touch-action" })
  public touchAction?: string;

  @property({ attribute: "tooltip-mode" })
  public tooltipMode: "never" | "always" | "interaction" = "interaction";

  @property({ type: Number })
  public value?: number;

  @property({ type: Number })
  public step = 1;

  @property({ type: Number })
  public min = 1;

  @property({ type: Number })
  public max = 4;

  @property({ type: Number })
  public range?: number;

  @state()
  public pressed = false;

  @state()
  public tooltipVisible = false;

  private _mc?: HammerManager;

  private get _range(): number {
    return this.range ?? this.max;
  }

  private _valueToPercentage(value: number): number {
    return this._boundedValue(value) / this._range;
  }

  private _percentageToValue(percentage: number): number {
    return this._range * percentage;
  }

  private _steppedValue(value: number): number {
    return Math.round(value / this.step) * this.step;
  }

  private _boundedValue(value: number): number {
    return Math.min(Math.max(value, this.min), this.max);
  }

  protected firstUpdated(changedProperties: PropertyValues): void {
    super.firstUpdated(changedProperties);
    this.setupListeners();
    this.setAttribute("role", "slider");
    if (!this.hasAttribute("tabindex")) {
      this.setAttribute("tabindex", "0");
    }
  }

  protected updated(changedProps: PropertyValues): void {
    super.updated(changedProps);
    if (changedProps.has("value")) {
      const valuenow = this._steppedValue(this.value ?? 0);
      this.setAttribute("aria-valuenow", valuenow.toString());
      this.setAttribute("aria-valuetext", valuenow.toString());
    }
    if (changedProps.has("min")) {
      this.setAttribute("aria-valuemin", this.min.toString());
    }
    if (changedProps.has("max")) {
      this.setAttribute("aria-valuemax", this.max.toString());
    }
    if (changedProps.has("vertical")) {
      this.setAttribute("aria-orientation", this.vertical ? "vertical" : "horizontal");
    }
  }

  connectedCallback(): void {
    super.connectedCallback();
    this.setupListeners();
  }

  disconnectedCallback(): void {
    super.disconnectedCallback();
    this.destroyListeners();
  }

  @query("#slider")
  private slider?: HTMLElement;

  setupListeners(): void {
    if (this.slider && !this._mc) {
      this._mc = new Manager(this.slider, {
        touchAction: this.touchAction ?? (this.vertical ? "pan-x" : "pan-y"),
      });
      this._mc.add(
        new Pan({
          threshold: 10,
          direction: DIRECTION_ALL,
          enable: true,
        }),
      );
      this._mc.add(new Tap({ event: "singletap" }));

      let savedValue: number | undefined;
      this._mc.on("panstart", () => {
        if (this.disabled) return;
        this.pressed = true;
        this._showTooltip();
        savedValue = this.value;
      });
      this._mc.on("pancancel", () => {
        if (this.disabled) return;
        this.pressed = false;
        this._hideTooltip();
        this.value = savedValue;
      });
      this._mc.on("panmove", (e) => {
        if (this.disabled) return;
        const percentage = this._getPercentageFromEvent(e);
        this.value = this._percentageToValue(percentage);
        const value = this._steppedValue(this._boundedValue(this.value));
        this.dispatchEvent(
          new CustomEvent("slider-moved", {
            bubbles: true,
            composed: true,
            detail: { value },
          }),
        );
      });
      this._mc.on("panend", (e) => {
        if (this.disabled) return;
        this.pressed = false;
        this._hideTooltip();
        const percentage = this._getPercentageFromEvent(e);
        const value = this._percentageToValue(percentage);
        this.value = this._steppedValue(this._boundedValue(value));
        this.dispatchEvent(
          new CustomEvent("slider-moved", {
            bubbles: true,
            composed: true,
            detail: { value: undefined },
          }),
        );
        this.dispatchEvent(
          new CustomEvent("value-changed", {
            bubbles: true,
            composed: true,
            detail: { value: this.value },
          }),
        );
      });

      this._mc.on("singletap", (e) => {
        if (this.disabled) return;
        const percentage = this._getPercentageFromEvent(e);
        const value = this._percentageToValue(percentage);
        this.value = this._steppedValue(this._boundedValue(value));
        this.dispatchEvent(
          new CustomEvent("value-changed", {
            bubbles: true,
            composed: true,
            detail: { value: this.value },
          }),
        );
      });

      this.addEventListener("keydown", this._handleKeyDown);
      this.addEventListener("keyup", this._handleKeyUp);
    }
  }

  destroyListeners(): void {
    if (this._mc) {
      this._mc.destroy();
      this._mc = undefined;
    }
    this.removeEventListener("keydown", this._handleKeyDown);
    this.removeEventListener("keyup", this._handleKeyUp);
  }

  private get _tenPercentStep(): number {
    return Math.max(this.step, (this.max - this.min) / 10);
  }

  private _handleKeyDown = (e: KeyboardEvent): void => {
    const codes = new Set([
      "ArrowRight",
      "ArrowUp",
      "ArrowLeft",
      "ArrowDown",
      "PageUp",
      "PageDown",
      "Home",
      "End",
    ]);
    if (!codes.has(e.code)) return;
    e.preventDefault();
    switch (e.code) {
      case "ArrowRight":
      case "ArrowUp":
        this.value = this._boundedValue((this.value ?? 0) + this.step);
        break;
      case "ArrowLeft":
      case "ArrowDown":
        this.value = this._boundedValue((this.value ?? 0) - this.step);
        break;
      case "PageUp":
        this.value = this._steppedValue(
          this._boundedValue((this.value ?? 0) + this._tenPercentStep),
        );
        break;
      case "PageDown":
        this.value = this._steppedValue(
          this._boundedValue((this.value ?? 0) - this._tenPercentStep),
        );
        break;
      case "Home":
        this.value = this.min;
        break;
      case "End":
        this.value = this.max;
        break;
      default:
        break;
    }
    this.dispatchEvent(
      new CustomEvent("slider-moved", {
        bubbles: true,
        composed: true,
        detail: { value: this.value },
      }),
    );
  };

  private _handleKeyUp = (e: KeyboardEvent): void => {
    const codes = new Set([
      "ArrowRight",
      "ArrowUp",
      "ArrowLeft",
      "ArrowDown",
      "PageUp",
      "PageDown",
      "Home",
      "End",
    ]);
    if (!codes.has(e.code)) return;
    e.preventDefault();
    this.dispatchEvent(
      new CustomEvent("value-changed", {
        bubbles: true,
        composed: true,
        detail: { value: this.value },
      }),
    );
  };

  private _tooltipTimeout?: number;

  private _showTooltip(): void {
    if (this._tooltipTimeout != null) window.clearTimeout(this._tooltipTimeout);
    this.tooltipVisible = true;
  }

  private _hideTooltip(delay?: number): void {
    if (!delay) {
      this.tooltipVisible = false;
      return;
    }
    this._tooltipTimeout = window.setTimeout(() => {
      this.tooltipVisible = false;
    }, delay);
  }

  private _getPercentageFromEvent = (e: HammerInput): number => {
    if (this.vertical) {
      const y = e.center.y;
      const offset = e.target.getBoundingClientRect().top;
      const total = e.target.clientHeight;
      return Math.max(Math.min(1, (y - offset) / total), 0);
    }
    const x = e.center.x;
    const offset = e.target.getBoundingClientRect().left;
    const total = e.target.clientWidth;
    return Math.max(Math.min(1, (x - offset) / total), 0);
  };

  private _renderTooltip() {
    if (this.tooltipMode === "never") return nothing;

    const position = this.vertical ? "left" : "top";
    const visible =
      this.tooltipMode === "always" ||
      (this.tooltipVisible && this.tooltipMode === "interaction");
    const value = this._boundedValue(this._steppedValue(this.value ?? 0));

    return html`
      <div
        class="tooltip ${classMap({
          visible,
          [position]: true,
        })}"
      >
        ${value}
      </div>
    `;
  }

  protected render(): TemplateResult {
    return html`
      <div
        class="container ${classMap({
          pressed: this.pressed,
        })}"
        style=${styleMap({
          "--value": `${this._valueToPercentage(this.value ?? 0)}`,
          "--min": `${this.min / this._range}`,
          "--max": `${1 - this.max / this._range}`,
        })}
      >
        <div id="slider" class="slider">
          <div class="track">
            <div class="background"></div>
            <div class="active"></div>
            ${Array(this._range / this.step)
              .fill(0)
              .map((_, i) => {
                const disabled = this.min >= i * this.step || i * this.step > this.max;
                if (disabled) return nothing;
                return html`
                  <div
                    class="dot"
                    style=${styleMap({
                      "--value": `${i / (this._range / this.step)}`,
                    })}
                  ></div>
                `;
              })}
            ${this.value !== undefined ? html`<div class="handle"></div>` : nothing}
          </div>
          ${this._renderTooltip()}
        </div>
      </div>
    `;
  }

  static styles = css`
    :host {
      display: block;
      --grid-layout-slider: 36px;
      height: var(--grid-layout-slider);
      width: 100%;
      outline: none;
      transition: box-shadow 180ms ease-in-out;
    }
    :host(:focus-visible) {
      box-shadow: 0 0 0 2px var(--primary-color);
    }
    :host([vertical]) {
      width: var(--grid-layout-slider);
      height: 100%;
    }
    .container {
      position: relative;
      height: 100%;
      width: 100%;
    }
    .slider {
      position: relative;
      height: 100%;
      width: 100%;
      transform: translateZ(0);
      overflow: visible;
      cursor: pointer;
    }
    .slider * {
      pointer-events: none;
      user-select: none;
    }
    .track {
      position: absolute;
      inset: 0;
      margin: auto;
      height: 16px;
      width: 100%;
      border-radius: var(--ha-border-radius-md, 8px);
      overflow: hidden;
    }
    :host([vertical]) .track {
      width: 16px;
      height: 100%;
    }
    .background {
      position: absolute;
      inset: 0;
      background: var(--disabled-color, #9e9e9e);
      opacity: 0.4;
    }
    .active {
      position: absolute;
      background: var(--primary-color);
      top: 0;
      right: calc(var(--max) * 100%);
      bottom: 0;
      left: calc(var(--min) * 100%);
    }
    :host([vertical]) .active {
      top: calc(var(--min) * 100%);
      right: 0;
      bottom: calc(var(--max) * 100%);
      left: 0;
    }
    .handle {
      position: absolute;
      top: 0;
      height: 100%;
      width: 16px;
      transform: translate(-50%, 0);
      background: var(--card-background-color);
      left: calc(var(--value, 0%) * 100%);
      transition:
        left 180ms ease-in-out,
        top 180ms ease-in-out;
    }
    :host([vertical]) .handle {
      transform: translate(0, -50%);
      left: 0;
      top: calc(var(--value, 0%) * 100%);
      height: 16px;
      width: 100%;
    }
    .dot {
      position: absolute;
      top: 0;
      bottom: 0;
      opacity: 0.6;
      margin: auto;
      width: 4px;
      height: 4px;
      flex-shrink: 0;
      transform: translate(-50%, 0);
      background: var(--card-background-color);
      left: calc(var(--value, 0%) * 100%);
      border-radius: 2px;
    }
    :host([vertical]) .dot {
      transform: translate(0, -50%);
      left: 0;
      right: 0;
      bottom: inherit;
      top: calc(var(--value, 0%) * 100%);
    }
    .handle::after {
      position: absolute;
      inset: 0;
      width: 4px;
      border-radius: 2px;
      height: 100%;
      margin: auto;
      background: var(--primary-color);
      content: "";
    }
    :host([vertical]) .handle::after {
      height: 4px;
      width: 100%;
    }
    :host([disabled]) .slider {
      cursor: not-allowed;
    }
    :host([disabled]) .track {
      opacity: 0.5;
    }
    :host([disabled]) .handle::after {
      background: var(--disabled-color, #9e9e9e);
    }
    :host([disabled]) .active {
      background: var(--disabled-color, #9e9e9e);
    }
    .tooltip {
      position: absolute;
      background-color: var(--card-background-color, #fff);
      color: var(--primary-text-color);
      font-size: 12px;
      border-radius: var(--ha-border-radius-lg, 12px);
      padding: 0.2em 0.4em;
      opacity: 0;
      white-space: nowrap;
      box-shadow: 0 2px 5px rgba(0, 0, 0, 0.2);
      transition:
        opacity 180ms ease-in-out,
        left 180ms ease-in-out,
        bottom 180ms ease-in-out;
      --slider-tooltip-position: calc(var(--value) * 100%);
    }
    .tooltip.visible {
      opacity: 1;
    }
    .tooltip.top {
      transform: translate3d(-50%, -100%, 0);
      top: -4px;
      left: var(--slider-tooltip-position);
    }
    .tooltip.left {
      transform: translate3d(-100%, -50%, 0);
      top: var(--slider-tooltip-position);
      left: -4px;
    }
    .pressed .handle {
      transition: none;
    }
    .pressed .tooltip {
      transition: opacity 180ms ease-in-out;
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "ha-grid-layout-slider": HaGridLayoutSlider;
  }
}
