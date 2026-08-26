import { css, html, LitElement, nothing } from "lit";
import { customElement, property, state } from "lit/decorators.js";
import { classMap } from "lit/directives/class-map.js";
import { styleMap } from "lit/directives/style-map.js";
import type { Flow, FlowValue, LovelaceHeadingBadgeConfig } from "../../../types";
import { stateActive } from "../../../common/entity/state-active";
import { computeTileColor } from "../../../common/entity/tile-color";
import { parseBadgeWriteValue } from "./badge-value";
import { badgeShowText } from "./badge-flags";
import "../../../components/ha-icon";

@customElement("hui-button-heading-badge")
export class HuiButtonHeadingBadge extends LitElement {
  @property({ attribute: false }) public flow?: Flow;

  @property({ attribute: false }) public config?: LovelaceHeadingBadgeConfig;

  @state() private _flashing = false;

  private _pulseTimer?: ReturnType<typeof setTimeout>;

  disconnectedCallback(): void {
    if (this._pulseTimer) clearTimeout(this._pulseTimer);
    super.disconnectedCallback();
  }

  private _pulse(): void {
    this._flashing = true;
    if (this._pulseTimer) clearTimeout(this._pulseTimer);
    this._pulseTimer = setTimeout(() => {
      this._flashing = false;
      this._pulseTimer = undefined;
    }, 700);
  }

  private async _tap(ev: Event): Promise<void> {
    ev.stopPropagation();
    const entity = this.config?.entity ? String(this.config.entity) : "";
    if (!entity || !this.flow) return;
    const value = parseBadgeWriteValue(this.config?.value) as FlowValue;
    await this.flow.callService(entity, value);
    this._pulse();
  }

  protected render() {
    const config = this.config;
    if (!config) return nothing;

    const icon = config.icon ? String(config.icon) : "";
    const text = config.text ? String(config.text) : config.name ? String(config.name) : "";
    const showName = badgeShowText(config) && Boolean(text);
    const entity = config.entity ? String(config.entity).trim() : "";
    const state = entity && this.flow ? this.flow.states[entity] : undefined;
    const active = this._flashing || stateActive(state);
    const color = computeTileColor(active, config.color);
    const clickable = Boolean(config.entity);

    return html`
      <button
        type="button"
        class=${classMap({
          badge: true,
          "with-text": showName,
          colored: Boolean(color),
          flashing: this._flashing && !color,
        })}
        style=${styleMap({ "--color": color })}
        ?disabled=${!clickable}
        @click=${this._tap}
      >
        ${icon ? html`<ha-icon .icon=${icon}></ha-icon>` : nothing}
        ${showName ? html`<span class="text">${text}</span>` : nothing}
      </button>
    `;
  }

  static styles = css`
    :host {
      display: inline-flex;
    }
    .badge {
      display: inline-flex;
      flex-direction: row;
      align-items: center;
      justify-content: center;
      gap: 2px;
      height: 26px;
      min-width: 26px;
      padding: 0;
      border: none;
      border-radius: 999px;
      background: color-mix(in srgb, var(--primary-text-color) 8%, transparent);
      color: var(--secondary-text-color);
      cursor: pointer;
      font: inherit;
      font-size: 12px;
      font-weight: 500;
      line-height: 1;
      white-space: nowrap;
    }
    .badge.with-text {
      padding: 0 8px;
    }
    .badge.colored {
      color: var(--color);
      background: color-mix(in srgb, var(--color) 20%, transparent);
    }
    .badge.flashing {
      color: var(--state-icon-color, var(--primary-color));
      background: color-mix(in srgb, var(--state-icon-color, var(--primary-color)) 20%, transparent);
    }
    .badge:disabled {
      cursor: default;
      opacity: 0.55;
    }
    .badge:not(:disabled):hover {
      filter: brightness(0.96);
    }
    ha-icon {
      --mdc-icon-size: 16px;
    }
    .text {
      padding: 0 4px;
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "hui-button-heading-badge": HuiButtonHeadingBadge;
  }
}
