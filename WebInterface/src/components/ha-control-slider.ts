import { css, html, LitElement, nothing } from "lit";
import { customElement, property, query, state } from "lit/decorators.js";
import { styleMap } from "lit/directives/style-map.js";

/** HA-style control slider with optional vertical (fader) orientation. */
@customElement("ha-control-slider")
export class HaControlSlider extends LitElement {
  @property({ type: Number }) public value?: number;

  @property({ type: Number }) public min = 0;

  @property({ type: Number }) public max = 100;

  @property({ type: Number }) public step = 1;

  @property({ type: Boolean, reflect: true }) public disabled = false;

  @property({ type: Boolean, attribute: "show-handle" }) public showHandle = true;

  @property({ type: Boolean, attribute: "round-value" }) public roundValue = true;

  /** Vertical fader: bottom = min, top = max. */
  @property({ type: Boolean, reflect: true }) public vertical = false;

  @state() private _pressed = false;

  @query(".slider") private _slider?: HTMLElement;

  private _pointerId?: number;

  private boundedValue(value: number): number {
    return Math.min(Math.max(value, this.min), this.max);
  }

  private steppedValue(value: number): number {
    return this.boundedValue(Math.round(value / this.step) * this.step);
  }

  private percentage(): number {
    const value = this.value ?? this.min;
    if (this.max === this.min) return 0;
    return (this.boundedValue(value) - this.min) / (this.max - this.min);
  }

  private _emitChange(value: number): void {
    this.dispatchEvent(
      new CustomEvent("value-changed", {
        bubbles: true,
        composed: true,
        detail: { value },
      }),
    );
  }

  private _setFromPointer(clientX: number, clientY: number): void {
    const slider = this._slider;
    if (!slider) return;
    const rect = slider.getBoundingClientRect();
    let pct: number;
    if (this.vertical) {
      // Bottom = min, top = max
      pct = Math.max(0, Math.min(1, (rect.bottom - clientY) / rect.height));
    } else {
      pct = Math.max(0, Math.min(1, (clientX - rect.left) / rect.width));
    }
    const next = this.steppedValue(this.min + pct * (this.max - this.min));
    this.value = next;
    this._emitChange(next);
  }

  private _onPointerDown = (ev: PointerEvent): void => {
    if (this.disabled) return;
    ev.preventDefault();
    this._pressed = true;
    this._pointerId = ev.pointerId;
    this._slider?.setPointerCapture(ev.pointerId);
    this._setFromPointer(ev.clientX, ev.clientY);
  };

  private _onPointerMove = (ev: PointerEvent): void => {
    if (!this._pressed || ev.pointerId !== this._pointerId) return;
    this._setFromPointer(ev.clientX, ev.clientY);
  };

  private _onPointerUp = (ev: PointerEvent): void => {
    if (ev.pointerId !== this._pointerId) return;
    this._pressed = false;
    this._pointerId = undefined;
    try {
      this._slider?.releasePointerCapture(ev.pointerId);
    } catch {
      // ignore
    }
  };

  private _onKeyDown = (ev: KeyboardEvent): void => {
    if (this.disabled) return;
    let next = this.value ?? this.min;
    switch (ev.key) {
      case "ArrowRight":
      case "ArrowUp":
        next += this.step;
        break;
      case "ArrowLeft":
      case "ArrowDown":
        next -= this.step;
        break;
      case "Home":
        next = this.min;
        break;
      case "End":
        next = this.max;
        break;
      default:
        return;
    }
    ev.preventDefault();
    next = this.steppedValue(next);
    this.value = next;
    this._emitChange(next);
  };

  protected render() {
    const value = this.value ?? this.min;
    const displayed = this.roundValue ? Math.round(value) : value;

    return html`
      <div class="container ${this._pressed ? "pressed" : ""}">
        <div
          class="slider"
          tabindex=${this.disabled ? nothing : "0"}
          role="slider"
          aria-orientation=${this.vertical ? "vertical" : "horizontal"}
          aria-valuemin=${String(this.min)}
          aria-valuemax=${String(this.max)}
          aria-valuenow=${String(displayed)}
          aria-disabled=${this.disabled ? "true" : "false"}
          @pointerdown=${this._onPointerDown}
          @pointermove=${this._onPointerMove}
          @pointerup=${this._onPointerUp}
          @pointercancel=${this._onPointerUp}
          @keydown=${this._onKeyDown}
        >
          <div class="slider-track-background"></div>
          <div
            class="slider-track-bar ${this.showHandle ? "show-handle" : ""}"
            style=${styleMap({ "--value": String(this.percentage()) })}
          ></div>
        </div>
      </div>
    `;
  }

  static styles = css`
    :host {
      display: block;
      --control-slider-color: var(--feature-color, var(--tile-color, var(--primary-color)));
      --control-slider-background: var(--feature-color, var(--tile-color, var(--primary-color)));
      --control-slider-background-opacity: 0.2;
      --control-slider-thickness: var(--feature-height, 40px);
      --control-slider-border-radius: var(
        --feature-border-radius,
        var(--ha-card-border-radius, var(--ha-border-radius-lg, 12px))
      );
      height: var(--control-slider-thickness);
      width: 100%;
    }
    :host([vertical]) {
      height: 100%;
      width: var(--control-slider-thickness);
    }
    .container {
      position: relative;
      height: 100%;
      width: 100%;
      --handle-size: 4px;
      --handle-margin: calc(var(--control-slider-thickness) / 8);
    }
    .slider {
      position: relative;
      height: 100%;
      width: 100%;
      border-radius: var(--control-slider-border-radius);
      outline: none;
      overflow: hidden;
      cursor: pointer;
      touch-action: none;
    }
    .slider:focus-visible {
      box-shadow: 0 0 0 2px var(--control-slider-color);
    }
    .slider * {
      pointer-events: none;
    }
    .slider-track-background {
      position: absolute;
      inset: 0;
      background: var(--control-slider-background);
      opacity: var(--control-slider-background-opacity);
    }
    .slider-track-bar {
      --ha-border-radius: var(--control-slider-border-radius);
      --slider-size: 100%;
      position: absolute;
      top: 0;
      left: 0;
      height: 100%;
      width: 100%;
      background-color: var(--control-slider-color);
      transform: translate3d(calc((var(--value, 0) - 1) * var(--slider-size)), 0, 0);
      border-radius: var(--control-slider-border-radius);
      transition:
        transform 180ms ease-in-out,
        background-color 180ms ease-in-out;
    }
    :host([vertical]) .slider-track-bar {
      transform: translate3d(0, calc((1 - var(--value, 0)) * var(--slider-size)), 0);
    }
    .slider-track-bar.show-handle {
      --slider-size: calc(100% - 2 * var(--handle-margin) - var(--handle-size));
    }
    .slider-track-bar::after {
      display: block;
      content: "";
      position: absolute;
      top: 0;
      bottom: 0;
      right: var(--handle-margin);
      margin: auto;
      height: 50%;
      width: var(--handle-size);
      border-radius: var(--handle-size);
      background-color: white;
    }
    :host([vertical]) .slider-track-bar::after {
      top: var(--handle-margin);
      bottom: auto;
      right: 0;
      left: 0;
      margin: auto;
      width: 50%;
      height: var(--handle-size);
    }
    .pressed .slider-track-bar {
      transition: none;
    }
    :host([disabled]) .slider {
      cursor: not-allowed;
      opacity: 0.5;
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "ha-control-slider": HaControlSlider;
  }
}
