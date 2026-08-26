import { css, html, LitElement } from "lit";
import { customElement, property, query, state } from "lit/decorators.js";
import { styleMap } from "lit/directives/style-map.js";
import {
  hsvaToRgba,
  rgbaToCss,
  rgbaToHsva,
  type HsvaColor,
  type RgbaColor,
} from "../common/entity/color";

/**
 * Inline HS color pad (HA-style, no native color popup).
 * X = hue (0–360), Y = saturation (0–1, top = full).
 * Value (brightness) is preserved from the current color.
 */
@customElement("ha-control-color-pad")
export class HaControlColorPad extends LitElement {
  @property({ type: Number }) public hue = 200;

  @property({ type: Number }) public saturation = 0.8;

  @property({ type: Number }) public value = 1;

  @property({ type: Number }) public alpha = 1;

  @property({ type: Boolean, reflect: true }) public disabled = false;

  @state() private _pressed = false;

  @query(".pad") private _pad?: HTMLElement;

  private _pointerId?: number;

  private _hsva(): HsvaColor {
    return {
      h: this.hue,
      s: this.saturation,
      v: this.value,
      a: this.alpha,
    };
  }

  private _rgba(): RgbaColor {
    return hsvaToRgba(this._hsva());
  }

  private _emitChange(): void {
    const rgba = this._rgba();
    this.dispatchEvent(
      new CustomEvent("value-changed", {
        bubbles: true,
        composed: true,
        detail: { rgba, hsva: this._hsva() },
      }),
    );
  }

  private _setFromPointer(clientX: number, clientY: number): void {
    const pad = this._pad;
    if (!pad) return;
    const rect = pad.getBoundingClientRect();
    if (rect.width <= 0 || rect.height <= 0) return;

    const px = Math.max(0, Math.min(1, (clientX - rect.left) / rect.width));
    const py = Math.max(0, Math.min(1, (clientY - rect.top) / rect.height));

    this.hue = Math.round(px * 360 * 10) / 10;
    this.saturation = Math.round((1 - py) * 1000) / 1000;
    this._emitChange();
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
  };

  private _onKeyDown = (ev: KeyboardEvent): void => {
    if (this.disabled) return;
    const hueStep = 2;
    const satStep = 0.02;
    let nextH = this.hue;
    let nextS = this.saturation;

    switch (ev.key) {
      case "ArrowRight":
        nextH += hueStep;
        break;
      case "ArrowLeft":
        nextH -= hueStep;
        break;
      case "ArrowUp":
        nextS += satStep;
        break;
      case "ArrowDown":
        nextS -= satStep;
        break;
      default:
        return;
    }

    ev.preventDefault();
    this.hue = ((nextH % 360) + 360) % 360;
    this.saturation = Math.max(0, Math.min(1, nextS));
    this._emitChange();
  };

  private _handleStyle(): Record<string, string> {
    return {
      left: `${(this.hue / 360) * 100}%`,
      top: `${(1 - this.saturation) * 100}%`,
      background: rgbaToCss(this._rgba()),
    };
  }

  /** Sync from an RGBA tuple (card remote state). */
  public applyRgba(rgba: RgbaColor): void {
    const hsva = rgbaToHsva(rgba);
    this.hue = hsva.h;
    this.saturation = hsva.s;
    this.value = hsva.v;
    this.alpha = hsva.a;
  }

  protected render() {
    return html`
      <div
        class="pad"
        role="application"
        tabindex=${this.disabled ? -1 : 0}
        aria-label="颜色触控板"
        aria-valuetext=${rgbaToCss(this._rgba())}
        aria-disabled=${this.disabled ? "true" : "false"}
        @pointerdown=${this._onPointerDown}
        @pointermove=${this._onPointerMove}
        @pointerup=${this._onPointerUp}
        @pointercancel=${this._onPointerUp}
        @keydown=${this._onKeyDown}
      >
        <div class="spectrum" aria-hidden="true"></div>
        <div class="sat-overlay" aria-hidden="true"></div>
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
      --control-color-border-radius: var(--ha-card-border-radius, 12px);
      --control-color-handle-size: 22px;
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
      border-radius: var(--control-color-border-radius);
      overflow: hidden;
      outline: none;
      cursor: crosshair;
      border: 1px solid rgba(0, 0, 0, 0.08);
      box-sizing: border-box;
    }
    .pad:focus-visible {
      box-shadow: 0 0 0 2px var(--primary-color, #03a9f4);
    }
    .spectrum {
      position: absolute;
      inset: 0;
      background: linear-gradient(
        to right,
        hsl(0, 100%, 50%),
        hsl(60, 100%, 50%),
        hsl(120, 100%, 50%),
        hsl(180, 100%, 50%),
        hsl(240, 100%, 50%),
        hsl(300, 100%, 50%),
        hsl(360, 100%, 50%)
      );
    }
    .sat-overlay {
      position: absolute;
      inset: 0;
      background: linear-gradient(to top, #fff, transparent);
      pointer-events: none;
    }
    .handle {
      position: absolute;
      width: var(--control-color-handle-size);
      height: var(--control-color-handle-size);
      margin-left: calc(var(--control-color-handle-size) / -2);
      margin-top: calc(var(--control-color-handle-size) / -2);
      border-radius: 50%;
      box-shadow:
        0 0 0 2px #fff,
        0 2px 8px rgba(0, 0, 0, 0.28);
      pointer-events: none;
      transition: transform 80ms ease-out;
      z-index: 1;
    }
    .handle.pressed {
      transform: scale(1.12);
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "ha-control-color-pad": HaControlColorPad;
  }
}
