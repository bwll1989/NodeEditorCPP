import { css, html, LitElement, nothing } from "lit";
import { property } from "lit/decorators.js";
import type { Flow, LovelaceCard, LovelaceCardConfig, LovelaceGridOptions } from "../../../types";
import { Chart2dPointTracker, CHART2D_DEFAULTS } from "../../../common/chart/chart-2d-entity";
import { ChartMultiBarTracker, normalizeBarEntities } from "../../../common/chart/chart-bar-entity";
import "../../../components/ha-card";
import "../../../components/chart/ha-chart-2d";

export type Chart2dMode = "line" | "scatter" | "bar";

function parseChartBool(value: unknown, defaultValue: boolean): boolean {
  if (value === undefined || value === null) return defaultValue;
  if (typeof value === "boolean") return value;
  if (typeof value === "number") return value !== 0;
  if (typeof value === "string") {
    const normalized = value.trim().toLowerCase();
    if (normalized === "false" || normalized === "0" || normalized === "no" || normalized === "off") {
      return false;
    }
    if (normalized === "true" || normalized === "1" || normalized === "yes" || normalized === "on") {
      return true;
    }
  }
  return Boolean(value);
}

export function readChart2dConfig(config: LovelaceCardConfig | undefined) {
  const maxPoints = Number(config?.max_points ?? CHART2D_DEFAULTS.max_points);
  return {
    bgColor: String(config?.bg_color ?? CHART2D_DEFAULTS.bg_color),
    maxPoints: Number.isFinite(maxPoints) && maxPoints > 0 ? Math.floor(maxPoints) : CHART2D_DEFAULTS.max_points,
    lineWidth: Number(config?.line_width ?? CHART2D_DEFAULTS.line_width),
    lineColor: String(config?.line_color ?? CHART2D_DEFAULTS.line_color),
    symbolSize: Number(config?.symbol_size ?? CHART2D_DEFAULTS.symbol_size),
    pointColor: String(config?.point_color ?? CHART2D_DEFAULTS.point_color),
    barColor: String(config?.bar_color ?? CHART2D_DEFAULTS.bar_color),
    xLabel: String(config?.x_label ?? CHART2D_DEFAULTS.x_label),
    yLabel: String(config?.y_label ?? CHART2D_DEFAULTS.y_label),
    showAxes: parseChartBool(config?.show_axes, CHART2D_DEFAULTS.show_axes),
    showGrid: parseChartBool(config?.show_grid, CHART2D_DEFAULTS.show_grid),
  };
}

export const chart2dCardStyles = css`
  :host {
    display: block;
    height: 100%;
    min-height: 160px;
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
    color: var(--primary-text-color);
  }
  ha-chart-2d {
    display: block;
    flex: 1 1 auto;
    width: 100%;
    min-height: 160px;
  }
`;

export abstract class HuiChart2dCardBase extends LitElement implements LovelaceCard {
  abstract readonly chartMode: Chart2dMode;

  abstract readonly stubEntity: string;

  abstract readonly stubName: string;

  @property({ attribute: false }) public flow?: Flow;

  protected _config?: LovelaceCardConfig;

  protected _pointTracker = new Chart2dPointTracker();

  protected _barTracker = new ChartMultiBarTracker();

  protected _dataRevision = 0;

  private _configTrackKey(config: LovelaceCardConfig | undefined): string {
    if (this.chartMode === "bar") {
      return JSON.stringify(normalizeBarEntities(config?.entities));
    }
    return String(config?.entity ?? "");
  }

  setConfig(config: LovelaceCardConfig): void {
    const prevKey = this._configTrackKey(this._config);
    this._config = config;
    if (this._configTrackKey(config) !== prevKey) {
      if (this.chartMode === "bar") {
        this._barTracker.dispose();
      } else {
        this._pointTracker.resetForEntity(String(config.entity ?? ""));
      }
    }
    this._syncTracker();
    this.requestUpdate();
  }

  connectedCallback(): void {
    super.connectedCallback();
    this.classList.add("chart-interactive");
    this._syncTracker();
  }

  disconnectedCallback(): void {
    this._pointTracker.dispose();
    this._barTracker.dispose();
    super.disconnectedCallback();
  }

  protected updated(changed: import("lit").PropertyValues): void {
    if (changed.has("flow")) {
      this._syncTracker();
    }
  }

  private _syncTracker(): void {
    const entity = this._config?.entity ? String(this._config.entity) : undefined;
    const opts = readChart2dConfig(this._config);
    const onChange = () => {
      this._dataRevision += 1;
      this.requestUpdate();
    };

    if (this.chartMode === "bar") {
      this._pointTracker.dispose();
      const entities = normalizeBarEntities(this._config?.entities);
      this._barTracker.bind(this.flow, entities, opts.barColor, onChange);
      return;
    }

    this._barTracker.dispose();
    this._pointTracker.bind(this.flow, entity, opts.maxPoints, onChange);
  }

  getGridOptions(): LovelaceGridOptions {
    return {
      columns: 12,
      rows: 4,
      min_columns: 6,
      min_rows: 3,
    };
  }

  protected renderChart2d() {
    if (!this._config) return nothing;

    const opts = readChart2dConfig(this._config);
    void this._dataRevision;

    const title = this._config.name ? String(this._config.name) : "";

    return html`
      <ha-card>
        ${title ? html`<h1 class="card-header">${title}</h1>` : nothing}
        <ha-chart-2d
          .mode=${this.chartMode}
          .points=${this._pointTracker.points}
          .bars=${this._barTracker.bars}
          .bgColor=${opts.bgColor}
          .lineWidth=${opts.lineWidth}
          .lineColor=${opts.lineColor}
          .symbolSize=${opts.symbolSize}
          .pointColor=${opts.pointColor}
          .barColor=${opts.barColor}
          .xLabel=${opts.xLabel}
          .yLabel=${opts.yLabel}
          .showAxes=${opts.showAxes}
          .showGrid=${opts.showGrid}
        ></ha-chart-2d>
      </ha-card>
    `;
  }
}
