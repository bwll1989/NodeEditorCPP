import { css, html, LitElement, nothing } from "lit";
import { customElement, property, state } from "lit/decorators.js";
import { classMap } from "lit/directives/class-map.js";
import { styleMap } from "lit/directives/style-map.js";
import type { Flow, LovelaceCard, LovelaceCardConfig, LovelaceGridOptions } from "../../../types";
import { stateActive, toggleValue } from "../../../common/entity/state-active";
import { computeSwitchFeatureColor } from "../../../common/entity/tile-color";
import { tileCardHostStyle, tileCardStyle } from "../../../styles/tile-card-style";
import "../../../components/ha-tile-container";
import "../../../components/ha-tile-icon";
import "../../../components/ha-tile-info";
import "../../../components/ha-control-switch";
import "../../../components/ha-card";

@customElement("hui-switch-card")
export class HuiSwitchCard extends LitElement implements LovelaceCard {
  public static getStubConfig(): LovelaceCardConfig {
    return {
      type: "switch",
      entity: "/demo/floor_lamp",
      name: "Flood light",
      icon: "mdi:lightbulb",
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

  private async _toggle(ev: Event): Promise<void> {
    ev.stopPropagation();
    const entity = this._config?.entity;
    if (!entity || !this.flow) return;
    await this.flow.callService(entity, toggleValue(this._entityState()));
  }

  protected render() {
    if (!this._config) return nothing;

    const entity = this._config.entity;
    const state = this._entityState();
    const active = stateActive(state);
    const vertical = Boolean(this._config.vertical);
    const name = String(this._config.name ?? state?.attributes?.friendly_name ?? entity ?? "Switch");
    const icon = String(this._config.icon ?? state?.attributes?.icon ?? "mdi:lightbulb");
    const stateText = entity && this.flow ? this.flow.formatState(entity) : active ? "开启" : "关闭";
    const tileColor = computeSwitchFeatureColor(active, this._config.color);
    const colorStyle = styleMap({
      "--tile-color": tileColor,
    });
    const featureStyle = styleMap({
      "--feature-color": tileColor ?? (active ? "var(--state-icon-color)" : "var(--state-inactive-color)"),
    });

    return html`
      <ha-card class=${classMap({ active })} style=${colorStyle}>
        <ha-tile-container ?vertical=${vertical} ?fixed-info-height=${vertical}>
          <ha-tile-icon slot="icon" .icon=${icon} .active=${active}></ha-tile-icon>
          <ha-tile-info slot="info" .primary=${name} .secondary=${stateText}></ha-tile-info>
          <div slot="features" class="feature" style=${featureStyle}>
            <ha-control-switch
              .checked=${active}
              icon-on="mdi:lightbulb"
              icon-off="mdi:lightbulb-outline"
              @change=${this._toggle}
              @click=${(ev: Event) => ev.stopPropagation()}
            ></ha-control-switch>
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
        --feature-color: var(--tile-color);
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
        --feature-color: var(--tile-color);
      }
    `,
  ];
}

declare global {
  interface HTMLElementTagNameMap {
    "hui-switch-card": HuiSwitchCard;
  }
}
