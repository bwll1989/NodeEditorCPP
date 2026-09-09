import { css, html, LitElement } from "lit";
import { customElement, property, query, state } from "lit/decorators.js";
import { styleMap } from "lit/directives/style-map.js";
import {
  centerXyPoint,
  normalizeXyPoint,
  type XyPoint,
} from "../common/entity/xy-pad";

/**
 * HA-style 2D XY pad control (pointer + keyboard).
 * Emits `value-changed` with `{ x, y }` in configured axis ranges.
 * Default Y: pad top = yMax. When invertY, pad top = yMin (up = negative).
 */
@customElement("ha-control-xy-pad")
export class HaControlXyPad extends LitElement {
  @property({ type: Number }) public x = 0;

  @property({ type: Number }) public y = 0;

  @property({ type: Number, attribute: "x-min" }) public xMin = 0;

  @property({ type: Number, attribute: "x-max" }) public xMax = 1;

  @property({ type: Number, attribute: "y-min" }) public yMin = 0;

  @property({ type: Number, attribute: "y-max" }) public yMax = 1;

  @property({ type: Number }) public step = 0.01;

  /** When true, pad top maps to yMin (upward is negative). */
  @property({ type: Boolean }) public invertY = false;

  @property({ type: Boolean }) public showGrid = true;

  /** When true, pointer release snaps handle to range center. */
  @property({ type: Boolean }) public snapCenter = false;

  @property({ type: Boolean, reflect: true }) public disabled = false;

  @state() private _pressed = false;

  @query(".pad") private _pad?: HTMLElement;

  private _pointerId?: number;

  private _emitChange(point: XyPoint): void {
    this.dispatchEvent(
      new CustomEvent("value-changed", {
        bubbles: true,
        composed: true,
        detail: point,
      }),
    );
  }

  private _normalize(point: XyPoint): XyPoint {
    return normalizeXyPoint(point, {
      xMin: this.xMin,
      xMax: this.xMax,
      yMin: this.yMin,
      yMax: this.yMax,
      step: this.step,
    });
  }

  private _setFromPointer(clientX: number, clientY: number): void {
    const pad = this._pad;
    if (!pad) return;
    const rect = pad.getBoundingClientRect();
    if (rect.width <= 0 || rect.height <= 0) return;

    const px = Math.max(0, Math.min(1, (clientX - rect.left) / rect.width));
    const py = Math.max(0, Math.min(1, (clientY - rect.top) / rect.height));

    const x = this.xMin + px * (this.xMax - this.xMin);
    const yRatio = this.invertY ? py : 1 - py;
    const y = this.yMin + yRatio * (this.yMax - this.yMin);

    const next = this._normalize({ x, y });
    this.x = next.x;
    this.y = next.y;
    this._emitChange(next);
  }

  private _onPointerDown = (ev: PointerEvent): void => {
    if (this.disabled) return;
    ev.preventDefault();
    this._pressed = true;
    this._pointerId = ev.pointerId;
    this._pad?.setPointerCapture(ev.pointerId);
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
      this._pad?.releasePointerCapture(ev.pointerId);
    } catch {
      // ignore
    }

    if (this.snapCenter) {
      const center = this._normalize(
        centerXyPoint(this.xMin, this.xMax, this.yMin, this.yMax),
      );
      this.x = center.x;
      this.y = center.y;
      this._emitChange(center);
    }

    this.dispatchEvent(
      new CustomEvent("drag-end", {
        bubbles: true,
        composed: true,
        detail: { x: this.x, y: this.y },
      }),
    );
  };

  private _onKeyDown = (ev: KeyboardEvent): void => {
    if (this.disabled) return;
    let nextX = this.x;
    let nextY = this.y;
    const step = this.step > 0 ? this.step : 0.01;

    switch (ev.key) {
      case "ArrowRight":
        nextX += step;
        break;
      case "ArrowLeft":
        nextX -= step;
        break;
      case "ArrowUp":
        nextY += this.invertY ? -step : step;
        break;
      case "ArrowDown":
        nextY += this.invertY ? step : -step;
        break;
      case "Home":
        nextX = this.xMin;
        nextY = this.yMin;
        break;
      case "End":
        nextX = this.xMax;
        nextY = this.yMax;
        break;
      default:
        return;
    }

    ev.preventDefault();
    const next = this._normalize({ x: nextX, y: nextY });
    this.x = next.x;
    this.y = next.y;
    this._emitChange(next);
  };

  private _handleStyle(): Record<string, string> {
    const xSpan = this.xMax - this.xMin || 1;
    const ySpan = this.yMax - this.yMin || 1;
    const left = ((this.x - this.xMin) / xSpan) * 100;
    const yRatio = (this.y - this.yMin) / ySpan;
    const top = (this.invertY ? yRatio : 1 - yRatio) * 100;
    return {
      left: `${left}%`,
      top: `${top}%`,
    };
  }

  protected render() {
    return html`
      <div
        class="pad"
        role="application"
        tabindex=${this.disabled ? -1 : 0}
        aria-label="XY 触控板"
        aria-valuetext=${`${this.x}, ${this.y}`}
        aria-disabled=${this.disabled ? "true" : "false"}
        @pointerdown=${this._onPointerDown}
        @pointermove=${this._onPointerMove}
        @pointerup=${this._onPointerUp}
        @pointercancel=${this._onPointerUp}
        @keydown=${this._onKeyDown}
      >
        ${this.showGrid
          ? html`
              <div class="grid" aria-hidden="true">
                <span class="hline"></span>
                <span class="vline"></span>
              </div>
            `
          : null}
        <div
          class="handle ${this._pressed ? "pressed" : ""}"
          style=${styleMap(this._handleStyle())}
          aria-hidden="true"
        ></div>
      </div>
    `;
  }

  static styles = css`
    :host {
      display: block;
      width: 100%;
      height: 100%;
      min-height: 120px;
      --control-xy-color: var(--feature-color, var(--primary-color, #03a9f4));
      --control-xy-background: color-mix(
        in srgb,
        var(--control-xy-color) 12%,
        var(--secondary-background-color, #f5f5f5)
      );
      --control-xy-handle-size: 22px;
      --control-xy-border-radius: var(--ha-card-border-radius, 12px);
      touch-action: none;
      user-select: none;
    }
    :host([disabled]) {
      opacity: 0.45;
      pointer-events: none;
    }
    .pad {
      position: relative;
      width: 100%;
      height: 100%;
      min-height: inherit;
      border-radius: var(--control-xy-border-radius);
      background: var(--control-xy-background);
      box-sizing: border-box;
      outline: none;
      cursor: crosshair;
      overflow: hidden;
      border: 1px solid color-mix(in srgb, var(--control-xy-color) 25%, transparent);
    }
    .pad:focus-visible {
      box-shadow: 0 0 0 2px var(--control-xy-color);
    }
    .grid {
      position: absolute;
      inset: 0;
      pointer-events: none;
    }
    .hline,
    .vline {
      position: absolute;
      background: color-mix(in srgb, var(--control-xy-color) 28%, transparent);
    }
    .hline {
      left: 0;
      right: 0;
      top: 50%;
      height: 1px;
      transform: translateY(-0.5px);
    }
    .vline {
      top: 0;
      bottom: 0;
      left: 50%;
      width: 1px;
      transform: translateX(-0.5px);
    }
    .handle {
      position: absolute;
      width: var(--control-xy-handle-size);
      height: var(--control-xy-handle-size);
      margin-left: calc(var(--control-xy-handle-size) / -2);
      margin-top: calc(var(--control-xy-handle-size) / -2);
      border-radius: 50%;
      background: var(--control-xy-color);
      box-shadow:
        0 0 0 2px #fff,
        0 2px 8px rgba(0, 0, 0, 0.2);
      pointer-events: none;
      transition: transform 80ms ease-out;
    }
    .handle.pressed {
      transform: scale(1.12);
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "ha-control-xy-pad": HaControlXyPad;
  }
}
