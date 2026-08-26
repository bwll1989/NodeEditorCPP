import { css, html, LitElement, nothing, type PropertyValues } from "lit";
import { customElement, property, state } from "lit/decorators.js";
import { consume } from "@lit/context";
import { styleMap } from "lit/directives/style-map.js";
import type { Flow, LovelaceHeadingBadgeConfig } from "../../../types";
import { flowContext } from "../../../context/flow-context";
import { stateActive } from "../../../common/entity/state-active";
import { computeCssColor } from "../../../common/color/theme-colors";
import { badgeShowIcon, badgeShowText } from "./badge-flags";
import "../../../components/ha-heading-badge";
import "../../../components/ha-icon";

@customElement("hui-entity-heading-badge")
export class HuiEntityHeadingBadge extends LitElement {
  @consume({ context: flowContext, subscribe: true })
  @property({ attribute: false })
  public flow?: Flow;

  @property({ attribute: false }) public config?: LovelaceHeadingBadgeConfig;

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

  private _stateText(): string {
    const entity = this._entityId();
    if (!entity || !this.flow) return "—";
    return this.flow.formatState(entity);
  }

  private _iconColor(): string | undefined {
    const color = this.config?.color ? String(this.config.color).trim() : "";
    if (!color || color === "none") return undefined;
    const entity = this._entityId();
    const state = entity && this.flow ? this.flow.states[entity] : undefined;
    const active = stateActive(state);
    if (color === "state") {
      return active ? "var(--state-icon-color)" : undefined;
    }
    return active ? computeCssColor(color) : undefined;
  }

  protected render() {
    const config = this.config;
    if (!config) return nothing;
    void this._tick;
    void this.statesRevision;

    const entity = this._entityId();
    const state = entity && this.flow ? this.flow.states[entity] : undefined;
    const showIcon = badgeShowIcon(config);
    const showText = badgeShowText(config);
    const missingAddress = !entity;
    const icon = missingAddress
      ? "mdi:alert-circle"
      : config.icon
        ? String(config.icon)
        : state?.attributes?.icon
          ? String(state.attributes.icon)
          : "mdi:eye";
    const color = missingAddress ? "var(--error-color)" : this._iconColor();

    return html`
      <ha-heading-badge
        class=${missingAddress ? "error" : ""}
        style=${styleMap({ "--icon-color": color })}
      >
        ${showIcon ? html`<ha-icon slot="icon" .icon=${icon}></ha-icon>` : nothing}
        ${showText ? html`<span>${missingAddress ? "—" : this._stateText()}</span>` : nothing}
      </ha-heading-badge>
    `;
  }

  static styles = css`
    :host {
      display: inline-flex;
    }
    ha-heading-badge.error {
      --icon-color: var(--error-color, #b91c1c);
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "hui-entity-heading-badge": HuiEntityHeadingBadge;
  }
}
