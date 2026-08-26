import { css, html, LitElement, nothing } from "lit";
import { customElement, property, state } from "lit/decorators.js";
import { classMap } from "lit/directives/class-map.js";
import { styleMap } from "lit/directives/style-map.js";
import type { Flow, LovelaceCard, LovelaceCardConfig, LovelaceGridOptions } from "../../../types";
import { stateActive, toggleValue } from "../../../common/entity/state-active";
import { computeTileColor } from "../../../common/entity/tile-color";
import { tileCardHostStyle, tileCardStyle } from "../../../styles/tile-card-style";
import "../../../components/ha-tile-container";
import "../../../components/ha-tile-icon";
import "../../../components/ha-tile-info";
import "../../../components/ha-card";

@customElement("hui-tile-card")
export class HuiTileCard extends LitElement implements LovelaceCard {
  public static getStubConfig(): LovelaceCardConfig {
    return {
      type: "tile",
      entity: "/demo/light",
      name: "Demo Light",
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
    const rows = vertical ? 2 : 1;
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

  private async _toggle(ev?: Event): Promise<void> {
    ev?.stopPropagation();
    const entity = this._config?.entity;
    if (!entity || !this.flow) return;
    await this.flow.callService(entity, toggleValue(this._entityState()));
  }

  protected render() {
    if (!this._config) return nothing;

    const entity = this._config.entity;
    const state = this._entityState();
    const active = stateActive(state);
    const name = String(this._config.name ?? state?.attributes?.friendly_name ?? entity ?? "Toggle");
    const icon = String(this._config.icon ?? state?.attributes?.icon ?? "mdi:help-circle");
    const hideState = Boolean(this._config.hide_state);
    const stateText = hideState
      ? ""
      : entity && this.flow
        ? this.flow.formatState(entity)
        : "—";
    const vertical = Boolean(this._config.vertical);
    const showIconToggle = Boolean(this._config.show_icon_action ?? true);
    const colorStyle = styleMap({
      "--tile-color": computeTileColor(active, this._config.color),
    });

    return html`
      <ha-card
        class=${classMap({ active })}
        style=${colorStyle}
        tabindex="0"
        @click=${this._toggle}
        @keydown=${(ev: KeyboardEvent) => {
          if (ev.key === "Enter" || ev.key === " ") {
            ev.preventDefault();
            void this._toggle();
          }
        }}
      >
        <ha-tile-container ?vertical=${vertical} ?fixed-info-height=${vertical}>
          <ha-tile-icon
            slot="icon"
            .icon=${icon}
            .active=${active}
            ?interactive=${showIconToggle}
            @click=${showIconToggle ? this._toggle : undefined}
          ></ha-tile-icon>
          <ha-tile-info
            slot="info"
            .primary=${name}
            .secondary=${stateText}
          ></ha-tile-info>
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
      }
      ha-card {
        height: 100%;
        min-height: 0;
      }
    `,
  ];
}

declare global {
  interface HTMLElementTagNameMap {
    "hui-tile-card": HuiTileCard;
  }
}
