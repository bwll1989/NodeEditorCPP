import { css, html, LitElement, nothing } from "lit";
import { customElement, property, state } from "lit/decorators.js";
import { styleMap } from "lit/directives/style-map.js";
import type { Flow, FlowValue, LovelaceCard, LovelaceCardConfig, LovelaceGridOptions } from "../../../types";
import { computeSensorColor } from "../../../common/entity/tile-color";
import { tileCardHostStyle, tileCardStyle } from "../../../styles/tile-card-style";
import "../../../components/ha-tile-container";
import "../../../components/ha-tile-icon";
import "../../../components/ha-tile-info";
import "../../../components/ha-card";

function formatLabelValue(raw: FlowValue | undefined): string {
  if (raw === undefined || raw === null) return "—";
  if (typeof raw === "string") return raw.trim() ? raw : "—";
  if (typeof raw === "boolean") return raw ? "是" : "否";
  if (typeof raw === "number") return String(raw);
  if (Array.isArray(raw)) return raw.map((item) => String(item)).join(", ") || "—";
  try {
    return JSON.stringify(raw);
  } catch {
    return String(raw);
  }
}

/** Read-only text / status label — binds `/string` style addresses. */
@customElement("hui-label-card")
export class HuiLabelCard extends LitElement implements LovelaceCard {
  public static getStubConfig(): LovelaceCardConfig {
    return {
      type: "label",
      entity: "/demo/string",
      name: "当前素材",
      icon: "mdi:label-outline",
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

  protected render() {
    if (!this._config) return nothing;

    const entity = this._config.entity;
    const state = entity && this.flow ? this.flow.states[entity] : undefined;
    const name = String(
      this._config.name ?? state?.attributes?.friendly_name ?? entity ?? "标签",
    );
    const icon = String(this._config.icon ?? state?.attributes?.icon ?? "mdi:label-outline");
    const hideState = Boolean(this._config.hide_state);
    const vertical = Boolean(this._config.vertical);
    const value = hideState ? "" : formatLabelValue(state?.state);
    const colorStyle = styleMap({
      "--tile-color": computeSensorColor(this._config.color),
    });
    const containerStyle = vertical
      ? styleMap({
          "--ha-tile-info-gap": "4px",
          "--ha-tile-info-min-height": "auto",
          "--ha-tile-info-primary-min-height": "auto",
          "--ha-tile-info-primary-line-height": "1.3",
          "--ha-tile-info-align-items": "center",
          "--ha-tile-info-text-align": "center",
        })
      : nothing;

    return html`
      <ha-card style=${colorStyle}>
        <ha-tile-container ?vertical=${vertical} ?fixed-info-height=${vertical} style=${containerStyle}>
          <ha-tile-icon slot="icon" .icon=${icon}></ha-tile-icon>
          <ha-tile-info slot="info" .primary=${name} .secondary=${value}></ha-tile-info>
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
        display: flex;
        flex-direction: column;
        cursor: default;
        overflow: hidden;
      }
      ha-tile-container {
        flex: 1;
        min-height: 0;
      }
    `,
  ];
}

declare global {
  interface HTMLElementTagNameMap {
    "hui-label-card": HuiLabelCard;
  }
}
