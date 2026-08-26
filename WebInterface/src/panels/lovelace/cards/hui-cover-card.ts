import { css, html, LitElement, nothing } from "lit";
import { customElement, property, state } from "lit/decorators.js";
import { classMap } from "lit/directives/class-map.js";
import { styleMap } from "lit/directives/style-map.js";
import type { Flow, LovelaceCard, LovelaceCardConfig, LovelaceGridOptions } from "../../../types";
import {
  coverActive,
  DEFAULT_COVER_POSITION_COUNT,
  formatCoverSecondary,
  getCoverPosition,
  resolveCoverPositions,
} from "../../../common/entity/cover";
import { computeFeatureColor } from "../../../common/entity/tile-color";
import { tileCardHostStyle, tileCardStyle } from "../../../styles/tile-card-style";
import "../../../components/ha-tile-container";
import "../../../components/ha-tile-icon";
import "../../../components/ha-tile-info";
import "../../../components/ha-control-cover-position";
import "../../../components/ha-card";

@customElement("hui-cover-card")
export class HuiCoverCard extends LitElement implements LovelaceCard {
  public static getStubConfig(): LovelaceCardConfig {
    return {
      type: "cover",
      entity: "/demo/kitchen_shutter",
      name: "Kitchen shutter",
      icon: "mdi:window-shutter",
      position_count: DEFAULT_COVER_POSITION_COUNT,
    };
  }

  @property({ attribute: false }) public flow?: Flow;

  @state() private _config?: LovelaceCardConfig;

  setConfig(config: LovelaceCardConfig): void {
    this._config = config;
    this.requestUpdate();
  }

  getGridOptions(): LovelaceGridOptions {
    const vertical = Boolean(this._config?.vertical);
    const rows = vertical ? 3 : 2;
    return {
      columns: 6,
      rows,
      min_columns: vertical ? 3 : 6,
      min_rows: rows,
    };
  }

  private _entityState() {
    const entity = this._config?.entity;
    if (!entity || !this.flow) return undefined;
    return this.flow.states[entity];
  }

  private _positions(): number[] {
    return resolveCoverPositions(this._config as Record<string, unknown> | undefined);
  }

  private async _onPositionChange(ev: CustomEvent<{ value: number }>): Promise<void> {
    ev.stopPropagation();
    const entity = this._config?.entity;
    if (!entity || !this.flow) return;
    await this.flow.callService(entity, ev.detail.value);
  }

  protected render() {
    if (!this._config) return nothing;

    const entity = this._config.entity;
    const state = this._entityState();
    const positions = this._positions();
    const active = coverActive(state, positions);
    const vertical = Boolean(this._config.vertical);
    const name = String(
      this._config.name ?? state?.attributes?.friendly_name ?? entity ?? "Cover",
    );
    const icon = String(this._config.icon ?? state?.attributes?.icon ?? "mdi:window-shutter");
    const position = getCoverPosition(state, positions);
    const tileColor = computeFeatureColor(active, this._config.color, "deep-purple");
    const colorStyle = styleMap({ "--tile-color": tileColor });
    const featureStyle = styleMap({
      "--feature-color": tileColor ?? (active ? "var(--deep-purple-color, #7e57c2)" : "var(--state-inactive-color)"),
    });

    return html`
      <ha-card class=${classMap({ active })} style=${colorStyle}>
        <ha-tile-container ?vertical=${vertical} ?fixed-info-height=${vertical}>
          <ha-tile-icon slot="icon" .icon=${icon} .active=${active}></ha-tile-icon>
          <ha-tile-info
            slot="info"
            .primary=${name}
            .secondary=${formatCoverSecondary(state, positions)}
          ></ha-tile-info>
          <div slot="features" class="feature" style=${featureStyle}>
            <ha-control-cover-position
              .value=${position}
              .positions=${positions}
              @value-changed=${this._onPositionChange}
              @click=${(ev: Event) => ev.stopPropagation()}
            ></ha-control-cover-position>
          </div>
        </ha-tile-container>
      </ha-card>
    `;
  }

  static styles = [
    tileCardStyle,
    tileCardHostStyle,
    css`
      :host {
        display: block;
        height: 100%;
        min-height: 0;
        --feature-height: 40px;
        --feature-border-radius: var(--ha-card-border-radius, 12px);
      }
      ha-card {
        height: 100%;
        min-height: 0;
        display: flex;
        flex-direction: column;
        cursor: default;
        overflow: hidden;
      }
      ha-tile-container {
        flex: 1;
        min-height: 0;
      }
      ha-card:hover {
        box-shadow: var(--ha-card-box-shadow);
      }
      .feature {
        box-sizing: border-box;
      }
    `,
  ];
}

declare global {
  interface HTMLElementTagNameMap {
    "hui-cover-card": HuiCoverCard;
  }
}
