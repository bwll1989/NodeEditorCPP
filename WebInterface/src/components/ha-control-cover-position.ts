import { css, html, LitElement } from "lit";
import { customElement, property } from "lit/decorators.js";
import { classMap } from "lit/directives/class-map.js";
import { DEFAULT_COVER_POSITIONS } from "../common/entity/cover";

/** HA-style segmented cover position control. */
@customElement("ha-control-cover-position")
export class HaControlCoverPosition extends LitElement {
  @property({ type: Number }) public value = 0;

  @property({ type: Array }) public positions: number[] = DEFAULT_COVER_POSITIONS;

  @property({ type: Boolean }) public disabled = false;

  private _emit(value: number): void {
    this.dispatchEvent(
      new CustomEvent("value-changed", {
        bubbles: true,
        composed: true,
        detail: { value },
      }),
    );
  }

  private _select(position: number, ev: Event): void {
    ev.stopPropagation();
    if (this.disabled) return;
    this.value = position;
    this._emit(position);
  }

  protected render() {
    const positions = this.positions?.length ? this.positions : DEFAULT_COVER_POSITIONS;

    return html`
      <div class="container" role="group" aria-label="单选位置">
        ${positions.map(
          (position) => html`
            <button
              type="button"
              class=${classMap({ segment: true, selected: this.value === position })}
              ?disabled=${this.disabled}
              @click=${(ev: Event) => this._select(position, ev)}
            >
              ${position}
            </button>
          `,
        )}
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
      grid-template-columns: repeat(auto-fit, minmax(0, 1fr));
      gap: 4px;
      height: 100%;
      padding: 4px;
      box-sizing: border-box;
      border-radius: var(--control-border-radius);
      background: color-mix(in srgb, var(--control-background) calc(var(--control-background-opacity) * 100%), transparent);
    }
    .segment {
      appearance: none;
      border: none;
      border-radius: calc(var(--control-border-radius) - 4px);
      background: transparent;
      color: var(--secondary-text-color);
      font-size: 14px;
      font-weight: 500;
      cursor: pointer;
      min-width: 0;
      padding: 0 4px;
      transition:
        background-color 180ms ease,
        color 180ms ease;
    }
    .segment.selected {
      background: var(--control-color);
      color: white;
    }
    .segment:hover:not(:disabled):not(.selected) {
      background: color-mix(in srgb, var(--control-color) 12%, transparent);
    }
    .segment:disabled {
      opacity: 0.4;
      cursor: not-allowed;
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "ha-control-cover-position": HaControlCoverPosition;
  }
}
