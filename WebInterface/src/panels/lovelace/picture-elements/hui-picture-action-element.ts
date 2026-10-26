import { css, html, LitElement, nothing, type PropertyValues } from "lit";
import { customElement, property, state } from "lit/decorators.js";
import { classMap } from "lit/directives/class-map.js";
import { styleMap } from "lit/directives/style-map.js";
import type { Flow, LovelacePictureElementConfig } from "../../../types";
import { stateActive } from "../../../common/entity/state-active";
import { computeTileColor } from "../../../common/entity/tile-color";
import { isLatchBadgeAction, normalizeBadgeConfig } from "../heading-badges/badge-model";
import {
  badgePressedVisual,
  badgeTapEnabled,
  handleBadgeTap,
  resolveLatchPressedFromState,
} from "../heading-badges/handle-badge-action";
import {
  badgeShowBackground,
  badgeShowText,
  badgeVisualSize,
  DEFAULT_BADGE_VISUAL_SIZE,
} from "../heading-badges/badge-flags";
import "../../../components/ha-icon";

/** 与标题行控制徽章一致的横向胶囊（图像 / 轨迹共用）。 */
@customElement("hui-picture-action-element")
export class HuiPictureActionElement extends LitElement {
  @property({ attribute: false }) public flow?: Flow;

  @property({ attribute: false }) public config?: LovelacePictureElementConfig;

  @property({ type: Boolean }) public preview = false;

  @property({ type: Number, attribute: false }) public statesRevision = 0;

  @state() private _flashing = false;

  @state() private _latched?: boolean;

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
    this._syncLatchedFromState();
  }

  private _entityId(): string {
    return this.config?.entity ? String(this.config.entity).trim() : "";
  }

  private _entityState() {
    const entity = this._entityId();
    if (!entity || !this.flow) return undefined;
    return this.flow.states[entity];
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
      this._syncLatchedFromState();
      this._tick += 1;
    });
  }

  private _syncLatchedFromState(): void {
    if (typeof this._latched !== "boolean") return;
    const resolved = resolveLatchPressedFromState(this.config, this._entityState());
    if (resolved === this._latched) {
      this._latched = undefined;
    }
  }

  private _iconColor(pressed: boolean): string {
    const active = pressed || stateActive(this._entityState());
    if (!active) return "var(--state-inactive-color, var(--secondary-text-color))";
    return (
      computeTileColor(true, this.config?.color) ??
      "var(--state-icon-color, var(--primary-color))"
    );
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
    const result = await handleBadgeTap(this.flow, this.config, {
      preview: this.preview,
      latched: this._latched,
    });
    if (result.mode === "pulse") {
      this._pulse();
      return;
    }
    if (result.mode === "latch") {
      this._latched = result.pressed;
    }
  }

  protected render() {
    const config = this.config;
    if (!config) return nothing;
    void this._tick;
    void this.statesRevision;

    const normalized = normalizeBadgeConfig(config);
    const icon = normalized.icon || "mdi:gesture-tap-button";
    const text = normalized.text || normalized.name || "";
    const showName = badgeShowText(config) && Boolean(text);
    const showBackground = badgeShowBackground(config);
    const height = badgeVisualSize(config, DEFAULT_BADGE_VISUAL_SIZE);
    const iconSize = Math.max(12, Math.round((height * 16) / 26));
    const fontSize = Math.max(11, Math.round((height * 12) / 26));
    const isLatch = isLatchBadgeAction(normalized.tap_action.action);
    const pressed = badgePressedVisual(config, this._entityState(), {
      flashing: this._flashing,
      latched: this._latched,
    });
    const color = this._iconColor(pressed);
    const clickable = badgeTapEnabled(config, this.preview);

    return html`
      <button
        type="button"
        class=${classMap({
          badge: true,
          "with-text": showName,
          transparent: !showBackground,
          flashing: !isLatch && this._flashing,
          pressed: isLatch && pressed,
        })}
        style=${styleMap({
          "--color": color,
          "--badge-height": `${height}px`,
          "--badge-icon-size": `${iconSize}px`,
          "--badge-font-size": `${fontSize}px`,
        })}
        ?disabled=${!clickable}
        aria-pressed=${isLatch ? (pressed ? "true" : "false") : "undefined"}
        @click=${this._tap}
      >
        <ha-icon .icon=${icon}></ha-icon>
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
      height: var(--badge-height, 26px);
      min-width: var(--badge-height, 26px);
      padding: 0;
      border: none;
      border-radius: 999px;
      background: color-mix(in srgb, var(--primary-text-color) 8%, transparent);
      color: var(--color, var(--secondary-text-color));
      cursor: pointer;
      font: inherit;
      font-size: var(--badge-font-size, 12px);
      font-weight: 500;
      line-height: 1;
      white-space: nowrap;
      user-select: none;
    }
    .badge.with-text {
      padding: 0 8px;
    }
    .badge.transparent {
      background: transparent;
    }
    .badge:not(.transparent).flashing,
    .badge:not(.transparent).pressed {
      background: color-mix(in srgb, var(--color, var(--primary-color)) 22%, transparent);
    }
    .badge:disabled {
      cursor: grab;
      opacity: 0.55;
    }
    .badge:not(:disabled):hover {
      filter: brightness(0.96);
    }
    .badge.transparent:not(:disabled):hover {
      filter: none;
      opacity: 0.85;
    }
    ha-icon {
      --mdc-icon-size: var(--badge-icon-size, 16px);
    }
    .text {
      padding: 0 4px;
      max-width: 96px;
      overflow: hidden;
      text-overflow: ellipsis;
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "hui-picture-action-element": HuiPictureActionElement;
  }
}
