import { css, html, LitElement, nothing, type PropertyValues } from "lit";
import { customElement, property, state } from "lit/decorators.js";
import { classMap } from "lit/directives/class-map.js";
import { styleMap } from "lit/directives/style-map.js";
import type { Flow, FlowValue, LovelacePictureElementConfig } from "../../../types";
import { stateActive } from "../../../common/entity/state-active";
import { computeTileColor } from "../../../common/entity/tile-color";
import { parseBadgeWriteValue } from "../heading-badges/badge-value";
import { badgeShowText } from "../heading-badges/badge-flags";
import "../../../components/ha-icon";

@customElement("hui-picture-action-element")
export class HuiPictureActionElement extends LitElement {
  @property({ attribute: false }) public flow?: Flow;

  @property({ attribute: false }) public config?: LovelacePictureElementConfig;

  @property({ type: Boolean }) public preview = false;

  @property({ type: Number, attribute: false }) public statesRevision = 0;

  @state() private _flashing = false;

  @state() private _tick = 0;

  private _pulseTimer?: ReturnType<typeof setTimeout>;

  private _unsub?: () => void;

  private _subscribedEntity?: string;

  connectedCallback(): void {
    super.connectedCallback();
    this._syncSubscription();
  }

  disconnectedCallback(): void {
    this._unsub?.();
    this._unsub = undefined;
    this._subscribedEntity = undefined;
    if (this._pulseTimer) clearTimeout(this._pulseTimer);
    super.disconnectedCallback();
  }

  protected updated(changed: PropertyValues): void {
    if (changed.has("flow") || changed.has("config")) {
      this._syncSubscription();
    }
  }

  private _entityId(): string {
    return this.config?.entity ? String(this.config.entity).trim() : "";
  }

  private _syncSubscription(): void {
    const entity = this._entityId();
    if (entity === this._subscribedEntity && this._unsub) return;
    this._unsub?.();
    this._unsub = undefined;
    this._subscribedEntity = undefined;
    if (!entity || !this.flow) return;
    this._subscribedEntity = entity;
    this._unsub = this.flow.subscribeEntity(entity, () => {
      this._tick += 1;
    });
  }

  private _circleColor(): string {
    const entity = this._entityId();
    const state = entity && this.flow ? this.flow.states[entity] : undefined;
    const active = this._flashing || stateActive(state);

    if (!active) {
      return "#000";
    }

    return computeTileColor(true, this.config?.color) ?? "var(--state-icon-color)";
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
    if (this.preview) return;
    const entity = this.config?.entity ? String(this.config.entity).trim() : "";
    if (!entity || !this.flow) return;
    const value = parseBadgeWriteValue(this.config?.value) as FlowValue;
    await this.flow.callService(entity, value);
    this._pulse();
  }

  protected render() {
    const config = this.config;
    if (!config) return nothing;
    void this._tick;
    void this.statesRevision;

    const icon = config.icon ? String(config.icon) : "mdi:gesture-tap-button";
    const text = config.text ? String(config.text) : config.name ? String(config.name) : "";
    const showName = badgeShowText(config) && Boolean(text);
    const color = this._circleColor();
    const hasCustomColor = Boolean(config.color && String(config.color).trim() && String(config.color).trim() !== "none");

    return html`
      <button
        type="button"
        class=${classMap({ badge: true, colored: hasCustomColor, flashing: this._flashing })}
        style=${styleMap({ "--color": color })}
        ?disabled=${this.preview || !config.entity}
        @click=${this._tap}
      >
        <span class="circle">
          <ha-icon .icon=${icon}></ha-icon>
        </span>
        ${showName ? html`<span class="label">${text}</span>` : nothing}
      </button>
    `;
  }

  static styles = css`
    :host {
      display: inline-flex;
    }
    .badge {
      display: inline-flex;
      flex-direction: column;
      align-items: center;
      gap: 4px;
      border: none;
      background: transparent;
      padding: 0;
      cursor: pointer;
      font: inherit;
      color: var(--primary-text-color);
    }
    .circle {
      width: 40px;
      height: 40px;
      border-radius: 50%;
      display: flex;
      align-items: center;
      justify-content: center;
      background: var(--card-background-color, #fff);
      color: var(--color, var(--primary-color));
      box-shadow: 0 1px 3px rgba(0, 0, 0, 0.18);
      border: 2px solid color-mix(in srgb, var(--color, var(--primary-color)) 55%, transparent);
    }
    .badge.colored .circle {
      color: var(--color);
    }
    .badge.flashing .circle {
      color: #fff;
      background: var(--color, var(--primary-color));
    }
    .badge:disabled {
      cursor: grab;
    }
    .badge:not(:disabled):hover .circle {
      filter: brightness(0.96);
    }
    ha-icon {
      --mdc-icon-size: 22px;
    }
    .label {
      font-size: 12px;
      font-weight: 500;
      max-width: 88px;
      overflow: hidden;
      text-overflow: ellipsis;
      white-space: nowrap;
      text-shadow: 0 0 4px var(--card-background-color, #fff);
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "hui-picture-action-element": HuiPictureActionElement;
  }
}
