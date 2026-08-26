import { css, html, LitElement, nothing } from "lit";
import { property } from "lit/decorators.js";
import type { Flow, LovelaceCard, LovelaceCardConfig, LovelaceGridOptions } from "../../../types";
import { Chart3dPointTracker, CHART3D_DEFAULTS } from "../../../common/chart/chart-3d-entity";
import "../../../components/ha-card";
import "../../../components/chart/ha-chart-3d";

export type Chart3dMode = "line" | "scatter";

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

export function readChart3dConfig(config: LovelaceCardConfig | undefined) {
  const maxPoints = Number(config?.max_points ?? CHART3D_DEFAULTS.max_points);
  return {
    bgColor: String(config?.bg_color ?? CHART3D_DEFAULTS.bg_color),
    maxPoints: Number.isFinite(maxPoints) && maxPoints > 0 ? Math.floor(maxPoints) : CHART3D_DEFAULTS.max_points,
    lineWidth: Number(config?.line_width ?? CHART3D_DEFAULTS.line_width),
    lineColor: String(config?.line_color ?? CHART3D_DEFAULTS.line_color),
    symbolSize: Number(config?.symbol_size ?? CHART3D_DEFAULTS.symbol_size),
    pointColor: String(config?.point_color ?? CHART3D_DEFAULTS.point_color),
    xLabel: String(config?.x_label ?? CHART3D_DEFAULTS.x_label),
    yLabel: String(config?.y_label ?? CHART3D_DEFAULTS.y_label),
    zLabel: String(config?.z_label ?? CHART3D_DEFAULTS.z_label),
    showAxes: parseChartBool(config?.show_axes, CHART3D_DEFAULTS.show_axes),
    autoRotate: parseChartBool(config?.auto_rotate, CHART3D_DEFAULTS.auto_rotate),
  };
}

export const chart3dCardStyles = css`
  :host {
    display: block;
    height: 100%;
    min-height: 180px;
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
  ha-chart-3d {
    display: block;
    flex: 1 1 auto;
    width: 100%;
    min-height: 180px;
    touch-action: none;
  }
`;

export abstract class HuiChart3dCardBase extends LitElement implements LovelaceCard {
  abstract readonly chartMode: Chart3dMode;

  abstract readonly stubEntity: string;

  abstract readonly stubName: string;

  @property({ attribute: false }) public flow?: Flow;

  protected _config?: LovelaceCardConfig;

  protected _tracker = new Chart3dPointTracker();

  protected _pointsRevision = 0;

  setConfig(config: LovelaceCardConfig): void {
    const prevEntity = this._config?.entity;
    this._config = config;
    if (config.entity !== prevEntity) {
      this._tracker.resetForEntity(String(config.entity ?? ""));
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
    this._tracker.dispose();
    super.disconnectedCallback();
  }

  protected updated(changed: import("lit").PropertyValues): void {
    if (changed.has("flow")) {
      this._syncTracker();
    }
  }

  private _syncTracker(): void {
    const entity = this._config?.entity ? String(this._config.entity) : undefined;
    const opts = readChart3dConfig(this._config);
    this._tracker.bind(this.flow, entity, opts.maxPoints, () => {
      this._pointsRevision += 1;
      this.requestUpdate();
    });
  }

  getGridOptions(): LovelaceGridOptions {
    return {
      columns: 12,
      rows: 4,
      min_columns: 6,
      min_rows: 3,
    };
  }

  protected renderChart3d() {
    if (!this._config) return nothing;

    const opts = readChart3dConfig(this._config);
    void this._pointsRevision;

    const title = this._config.name ? String(this._config.name) : "";

    return html`
      <ha-card>
        ${title ? html`<h1 class="card-header">${title}</h1>` : nothing}
        <ha-chart-3d
          .mode=${this.chartMode}
          .points=${this._tracker.points}
          .bgColor=${opts.bgColor}
          .lineWidth=${opts.lineWidth}
          .lineColor=${opts.lineColor}
          .symbolSize=${opts.symbolSize}
          .pointColor=${opts.pointColor}
          .xLabel=${opts.xLabel}
          .yLabel=${opts.yLabel}
          .zLabel=${opts.zLabel}
          .showAxes=${opts.showAxes}
          .autoRotate=${opts.autoRotate}
        ></ha-chart-3d>
      </ha-card>
    `;
  }
}
