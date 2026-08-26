import { css, html, LitElement, nothing, type PropertyValues } from "lit";
import { customElement, property, state } from "lit/decorators.js";
import type {
  Flow,
  LovelaceCard,
  LovelaceCardConfig,
  LovelaceGridOptions,
  LovelaceHeadingBadgeConfig,
} from "../../../types";
import { DEFAULT_HEADING_CARD_CONFIG } from "../../../data/section-config";
import "../../../components/ha-icon";
import "../../../components/ha-card";
import "../heading-badges/hui-entity-heading-badge";
import "../heading-badges/hui-button-heading-badge";

@customElement("hui-heading-card")
export class HuiHeadingCard extends LitElement implements LovelaceCard {
  public static getStubConfig(): LovelaceCardConfig {
    return { ...DEFAULT_HEADING_CARD_CONFIG };
  }

  @property({ attribute: false }) public flow?: Flow;

  @state() private _config?: LovelaceCardConfig;

  private _badgeUnsubs: Array<() => void> = [];

  private _subscribedKey = "";

  setConfig(config: LovelaceCardConfig): void {
    this._config = config;
    this._syncBadgeSubscriptions();
  }

  connectedCallback(): void {
    super.connectedCallback();
    this._syncBadgeSubscriptions();
  }

  disconnectedCallback(): void {
    this._clearBadgeSubscriptions();
    super.disconnectedCallback();
  }

  protected updated(changed: PropertyValues): void {
    if (changed.has("flow") || changed.has("_config")) {
      this._syncBadgeSubscriptions();
    }
  }

  private _clearBadgeSubscriptions(): void {
    this._badgeUnsubs.forEach((unsub) => unsub());
    this._badgeUnsubs = [];
    this._subscribedKey = "";
  }

  private _syncBadgeSubscriptions(): void {
    const flow = this.flow;
    const badges = (this._config?.badges ?? []) as LovelaceHeadingBadgeConfig[];
    const key = `${flow ? "1" : "0"}:${badges.map((b) => String(b.entity ?? "").trim()).join("\0")}`;
    if (key === this._subscribedKey && this._badgeUnsubs.length > 0) return;
    this._clearBadgeSubscriptions();
    if (!flow) return;
    this._subscribedKey = key;
    badges.forEach((badge) => {
      const entity = badge.entity ? String(badge.entity).trim() : "";
      if (!entity) return;
      this._badgeUnsubs.push(
        flow.subscribeEntity(entity, () => {
          this.requestUpdate();
        }),
      );
    });
  }

  getGridOptions(): LovelaceGridOptions {
    return { columns: "full", rows: "auto", min_columns: 3 };
  }

  private _renderBadge(badge: LovelaceHeadingBadgeConfig) {
    const type = String(badge.type ?? "entity");
    if (type === "button") {
      return html`
        <hui-button-heading-badge .flow=${this.flow} .config=${badge}></hui-button-heading-badge>
      `;
    }
    return html`
        <hui-entity-heading-badge
          .flow=${this.flow}
          .config=${badge}
          .statesRevision=${this.flow?.statesRevision ?? 0}
        ></hui-entity-heading-badge>
    `;
  }

  protected render() {
    if (!this._config) return nothing;
    void this.flow?.statesRevision;

    const heading = String(this._config.heading ?? "");
    const icon = this._config.icon ? String(this._config.icon) : undefined;
    const style = String(this._config.heading_style ?? "title");
    const badges = (this._config.badges ?? []) as LovelaceHeadingBadgeConfig[];

    return html`
      <ha-card>
        <div class="container">
          <div class="content ${style}">
            ${icon ? html`<ha-icon .icon=${icon}></ha-icon>` : nothing}
            ${heading ? html`<p>${heading}</p>` : nothing}
          </div>
          ${badges.length
            ? html`
                <div class="badges">
                  <div class="badges-row">
                    ${badges.map((badge) => this._renderBadge(badge))}
                  </div>
                </div>
              `
            : nothing}
        </div>
      </ha-card>
    `;
  }

  static styles = css`
    :host {
      display: block;
      width: 100%;
      height: 100%;
    }
    ha-card {
      background: none;
      border: none;
      box-shadow: none;
      backdrop-filter: none;
      padding: 0;
      display: flex;
      flex-direction: column;
      justify-content: flex-end;
      height: 100%;
      min-height: 24px;
      --ha-card-overflow: visible;
    }
    .container {
      padding: 0 var(--ha-space-1, 4px);
      display: flex;
      flex-direction: row;
      justify-content: space-between;
      flex-wrap: nowrap;
      align-items: center;
      overflow: visible;
      gap: var(--ha-space-2, 8px);
      min-height: 28px;
      width: 100%;
      box-sizing: border-box;
    }
    .content {
      display: flex;
      flex-direction: row;
      align-items: center;
      gap: var(--ha-space-2, 8px);
      min-width: 0;
      flex: 0 1 max-content;
      color: var(--ha-heading-card-title-color, var(--primary-text-color));
      font-size: var(--ha-heading-card-title-font-size, 18px);
      font-weight: var(--ha-heading-card-title-font-weight, 400);
      line-height: var(--ha-line-height-normal, 1.4);
      letter-spacing: 0.1px;
      --mdc-icon-size: 18px;
    }
    .container .content:not(:only-child) {
      flex: 1 0 var(--ha-heading-card-title-min-width, 150px);
      max-width: max-content;
    }
    .content.subtitle {
      color: var(--ha-heading-card-subtitle-color, var(--secondary-text-color));
      font-size: var(--ha-heading-card-subtitle-font-size, 14px);
      font-weight: var(--ha-font-weight-medium, 500);
      line-height: var(--ha-line-height-condensed, 1.25);
    }
    .content p {
      margin: 0;
      white-space: nowrap;
      overflow: hidden;
      text-overflow: ellipsis;
      flex-shrink: 1;
      min-width: 0;
    }
    .badges {
      position: relative;
      display: flex;
      flex: 1 1 auto;
      min-width: 0;
      overflow: auto;
      max-width: 100%;
      scrollbar-width: none;
      justify-content: flex-end;
    }
    .badges-row {
      display: flex;
      flex-direction: row;
      align-items: center;
      flex-wrap: nowrap;
      justify-content: flex-end;
      gap: var(--ha-space-2, 8px);
    }
    .badges-row > * {
      min-width: fit-content;
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "hui-heading-card": HuiHeadingCard;
  }
}
