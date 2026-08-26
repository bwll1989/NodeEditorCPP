import { css, html, LitElement } from "lit";
import { customElement, property } from "lit/decorators.js";
import { classMap } from "lit/directives/class-map.js";
import type { MediaPlayback } from "../common/entity/media";
import "./ha-icon";

/** Two-option playback control (play / stop). */
@customElement("ha-control-media-playback")
export class HaControlMediaPlayback extends LitElement {
  @property({ type: String }) public value: MediaPlayback = "stop";

  @property({ type: Boolean }) public disabled = false;

  private _emit(action: MediaPlayback): void {
    this.dispatchEvent(
      new CustomEvent("value-changed", {
        bubbles: true,
        composed: true,
        detail: { action },
      }),
    );
  }

  private _select(action: MediaPlayback, ev: Event): void {
    ev.stopPropagation();
    if (this.disabled || this.value === action) return;
    this.value = action;
    this._emit(action);
  }

  protected render() {
    return html`
      <div class="container" role="radiogroup" aria-label="播放控制">
        <button
          type="button"
          class=${classMap({ segment: true, selected: this.value === "play" })}
          role="radio"
          aria-checked=${this.value === "play" ? "true" : "false"}
          aria-label="播放"
          title="播放"
          ?disabled=${this.disabled}
          @click=${(ev: Event) => this._select("play", ev)}
        >
          <ha-icon .icon=${"mdi:play"}></ha-icon>
        </button>
        <button
          type="button"
          class=${classMap({ segment: true, selected: this.value === "stop" })}
          role="radio"
          aria-checked=${this.value === "stop" ? "true" : "false"}
          aria-label="停止"
          title="停止"
          ?disabled=${this.disabled}
          @click=${(ev: Event) => this._select("stop", ev)}
        >
          <ha-icon .icon=${"mdi:stop"}></ha-icon>
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
      grid-template-columns: 1fr 1fr;
      gap: 8px;
      height: 100%;
      box-sizing: border-box;
    }
    .segment {
      appearance: none;
      border: none;
      border-radius: var(--control-border-radius);
      background: color-mix(
        in srgb,
        var(--control-background) calc(var(--control-background-opacity) * 100%),
        transparent
      );
      color: var(--primary-text-color);
      cursor: pointer;
      min-width: 0;
      display: flex;
      align-items: center;
      justify-content: center;
      padding: 0;
      transition:
        background-color 180ms ease,
        color 180ms ease;
      --mdc-icon-size: 22px;
    }
    .segment ha-icon {
      display: flex;
      align-items: center;
      justify-content: center;
      width: var(--mdc-icon-size);
      height: var(--mdc-icon-size);
      color: inherit;
    }
    .segment.selected {
      background: var(--control-color);
      color: white;
    }
    .segment:hover:not(:disabled):not(.selected) {
      background: color-mix(in srgb, var(--control-color) 32%, transparent);
    }
    .segment:disabled {
      opacity: 0.4;
      cursor: not-allowed;
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "ha-control-media-playback": HaControlMediaPlayback;
  }
}
