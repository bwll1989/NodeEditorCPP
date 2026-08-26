import { css, html, LitElement, nothing } from "lit";
import { customElement, property, state } from "lit/decorators.js";
import type { Flow, LovelaceCard, LovelaceCardConfig, LovelaceGridOptions } from "../../../types";
import {
  buildHistorySeries,
  historyGraphEntities,
  parseOptionalNumber,
} from "../../../common/chart/history-graph";
import "../../../components/ha-card";
import "../../../components/chart/ha-history-chart";

@customElement("hui-history-graph-card")
export class HuiHistoryGraphCard extends LitElement implements LovelaceCard {
  public static getStubConfig(): LovelaceCardConfig {
    return {
      type: "history-graph",
      hours_to_show: 24,
      show_names: true,
      entities: [{ entity: "/demo/battery_input", name: "Battery Input" }],
      grid_options: { columns: 12, rows: 4, min_columns: 6, min_rows: 3 },
    };
  }

  @property({ attribute: false }) public flow?: Flow;

  @state() private _config?: LovelaceCardConfig;

  private _unsubs: Array<() => void> = [];

  setConfig(config: LovelaceCardConfig): void {
    this._config = config;
    this._resubscribe();
  }

  getGridOptions(): LovelaceGridOptions {
    return { columns: 12, rows: 4, min_columns: 6, min_rows: 3 };
  }

  disconnectedCallback(): void {
    this._clearSubs();
    super.disconnectedCallback();
  }

  private _clearSubs(): void {
    this._unsubs.forEach((unsub) => unsub());
    this._unsubs = [];
  }

  private _resubscribe(): void {
    this._clearSubs();
    const flow = this.flow;
    if (!flow || !this._config) return;
    for (const item of historyGraphEntities(this._config)) {
      if (!item.entity) continue;
      this._unsubs.push(
        flow.subscribeEntity(item.entity, () => {
          this.requestUpdate();
        }),
      );
    }
  }

  protected updated(): void {
    if (this.flow && this._unsubs.length === 0 && this._config) {
      this._resubscribe();
    }
  }

  protected render() {
    if (!this._config) return nothing;
    void this.flow?.statesRevision;

    const hours = Number(this._config.hours_to_show) || 24;
    const entities = historyGraphEntities(this._config);
    const series = buildHistorySeries(this.flow, entities, hours);
    const title = this._config.title ? String(this._config.title) : "";
    const unit = series.find((item) => item.unit)?.unit ?? "";

    return html`
      <ha-card>
        ${title ? html`<h1 class="card-header">${title}</h1>` : nothing}
        <ha-history-chart
          .series=${series}
          .hoursToShow=${hours}
          .logarithmic=${Boolean(this._config.logarithmic)}
          .yMin=${parseOptionalNumber(this._config.ymin)}
          .yMax=${parseOptionalNumber(this._config.ymax)}
          .showNames=${this._config.show_names !== false}
          .expandLegend=${Boolean(this._config.expand_legend)}
        ></ha-history-chart>
        ${unit && !title ? html`<div class="unit">${unit}</div>` : nothing}
      </ha-card>
    `;
  }

  static styles = css`
    :host {
      display: block;
      height: 100%;
    }
    ha-card {
      height: 100%;
      display: flex;
      flex-direction: column;
    }
    .card-header {
      margin: 0;
      padding: 12px 16px 0;
      font-size: 16px;
      font-weight: 500;
    }
    ha-history-chart {
      flex: 1 1 auto;
    }
    .unit {
      display: none;
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "hui-history-graph-card": HuiHistoryGraphCard;
  }
}
