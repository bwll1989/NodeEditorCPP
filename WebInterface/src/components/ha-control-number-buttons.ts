import { css, html, LitElement } from "lit";
import { customElement, property } from "lit/decorators.js";

/** HA-style +/- stepper for target temperature. */
@customElement("ha-control-number-buttons")
export class HaControlNumberButtons extends LitElement {
  @property({ type: Number }) public value = 0;

  @property({ type: Number }) public min = 0;

  @property({ type: Number }) public max = 100;

  @property({ type: Number }) public step = 1;

  @property({ type: Boolean }) public disabled = false;

  @property({ type: String }) public unit = "";

  @property({ type: Number }) public digits = 0;

  private _bounded(value: number): number {
    return Math.min(Math.max(value, this.min), this.max);
  }

  private _stepped(value: number): number {
    return this._bounded(Math.round(value / this.step) * this.step);
  }

  private _emit(value: number): void {
    this.dispatchEvent(
      new CustomEvent("value-changed", {
        bubbles: true,
        composed: true,
        detail: { value },
      }),
    );
  }

  private _change(delta: number, ev: Event): void {
    ev.stopPropagation();
    if (this.disabled) return;
    const next = this._stepped((this.value ?? this.min) + delta);
    this.value = next;
    this._emit(next);
  }

  protected render() {
    const displayed = (this.value ?? this.min).toFixed(this.digits);
    const unitSuffix = this.unit ? ` ${this.unit}` : "";

    return html`
      <div class="container">
        <button
          type="button"
          class="btn"
          aria-label="减少"
          ?disabled=${this.disabled}
          @click=${(ev: Event) => this._change(-this.step, ev)}
        >
          −
        </button>
        <div class="value">${displayed}${unitSuffix}</div>
        <button
          type="button"
          class="btn"
          aria-label="增加"
          ?disabled=${this.disabled}
          @click=${(ev: Event) => this._change(this.step, ev)}
        >
          +
        </button>
      </div>
    `;
  }

  static styles = css`
    :host {
      display: block;
      --control-color: var(--feature-color, var(--tile-color, var(--primary-color)));
      --control-background: var(--feature-color, var(--tile-color, var(--primary-color)));
      --control-background-opacity: 0.2;
      --control-height: var(--feature-height, 40px);
      --control-border-radius: var(
        --feature-border-radius,
        var(--ha-card-border-radius, var(--ha-border-radius-lg, 12px))
      );
      height: var(--control-height);
      width: 100%;
    }
    .container {
      display: grid;
      grid-template-columns: 1fr auto 1fr;
      align-items: center;
      height: 100%;
      border-radius: var(--control-border-radius);
      background: color-mix(in srgb, var(--control-background) calc(var(--control-background-opacity) * 100%), transparent);
      overflow: hidden;
    }
    .btn {
      appearance: none;
      border: none;
      background: transparent;
      color: var(--control-color);
      font-size: 22px;
      line-height: 1;
      height: 100%;
      cursor: pointer;
      padding: 0 12px;
    }
    .btn:hover:not(:disabled) {
      background: color-mix(in srgb, var(--control-color) 12%, transparent);
    }
    .btn:disabled {
      opacity: 0.4;
      cursor: not-allowed;
    }
    .value {
      color: var(--primary-text-color);
      font-size: 16px;
      font-weight: 500;
      white-space: nowrap;
      text-align: center;
      padding: 0 8px;
      user-select: none;
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "ha-control-number-buttons": HaControlNumberButtons;
  }
}
