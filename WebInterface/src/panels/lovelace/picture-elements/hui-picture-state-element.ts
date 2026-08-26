import { css, html, LitElement, nothing, type PropertyValues } from "lit";
import { customElement, property, state } from "lit/decorators.js";
import type { Flow, LovelacePictureElementConfig } from "../../../types";
import { stateActive } from "../../../common/entity/state-active";
import { formatPictureBadgeState } from "./format-state";
import { badgeShowIcon, badgeShowText } from "../heading-badges/badge-flags";
import "../../../components/ha-state-badge";

@customElement("hui-picture-state-element")
export class HuiPictureStateElement extends LitElement {
  @property({ attribute: false }) public flow?: Flow;

  @property({ attribute: false }) public config?: LovelacePictureElementConfig;

  @property({ type: Number, attribute: false }) public statesRevision = 0;

  @state() private _tick = 0;

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

  private _badgeColor(): string {
    const configured = this.config?.color ? String(this.config.color).trim() : "";
    const entity = this._entityId();
    const state = entity && this.flow ? this.flow.states[entity] : undefined;
    const active = stateActive(state);

    if (!active) {
      return "var(--state-inactive-color)";
    }

    if (!configured || configured === "none") {
      return "";
    }

    if (configured === "state") {
      return "var(--state-icon-color)";
    }

    return configured;
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

  protected render() {
    const config = this.config;
    if (!config) return nothing;
    void this._tick;
    void this.statesRevision;

    const entity = this._entityId();
    const formatted = formatPictureBadgeState(this.flow, entity, config.unit);
    const state = entity && this.flow ? this.flow.states[entity] : undefined;
    const icon = String(config.icon ?? state?.attributes?.icon ?? "mdi:eye");
    const color = this._badgeColor();
    const showIcon = badgeShowIcon(config);
    const showText = badgeShowText(config);

    return html`
      <ha-state-badge
        .value=${formatted.value}
        .unit=${formatted.unit}
        .icon=${icon}
        .color=${color}
        .showIcon=${showIcon}
        .showValue=${showText}
      ></ha-state-badge>
    `;
  }

  static styles = css`
    :host {
      display: inline-flex;
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "hui-picture-state-element": HuiPictureStateElement;
  }
}
