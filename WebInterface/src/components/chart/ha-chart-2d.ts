import { css, html, LitElement, type PropertyValues } from "lit";
import { customElement, property, query } from "lit/decorators.js";
import { styleMap } from "lit/directives/style-map.js";
import * as echarts from "echarts/core";
import { BarChart, LineChart, ScatterChart } from "echarts/charts";
import { GridComponent, TooltipComponent } from "echarts/components";
import { CanvasRenderer } from "echarts/renderers";
import type { BarItem } from "../../common/chart/chart-bar-entity";
import type { Point2 } from "../../common/chart/point2d";

echarts.use([TooltipComponent, GridComponent, CanvasRenderer, LineChart, ScatterChart, BarChart]);

type ChartMode = "line" | "scatter" | "bar";

function hasDomSize(el: HTMLElement | null | undefined): boolean {
  return !!el && el.clientWidth > 0 && el.clientHeight > 0;
}

const AXIS_LINE_COLOR = "rgba(0, 0, 0, 0.2)";
const AXIS_LABEL_COLOR = "rgba(0, 0, 0, 0.54)";
const AXIS_NAME_COLOR = "rgba(0, 0, 0, 0.6)";
const SPLIT_LINE_COLOR = "rgba(0, 0, 0, 0.08)";

@customElement("ha-chart-2d")
export class HaChart2d extends LitElement {
  @property({ type: String }) mode: ChartMode = "line";

  @property({ attribute: false }) points: Point2[] = [];

  @property({ attribute: false }) bars: BarItem[] = [];

  @property({ type: String }) bgColor = "";

  @property({ type: Number }) lineWidth = 2;

  @property({ type: String }) lineColor = "#03a9f4";

  @property({ type: Number }) symbolSize = 8;

  @property({ type: String }) pointColor = "#03a9f4";

  @property({ type: String }) barColor = "#03a9f4";

  @property({ type: String }) xLabel = "X";

  @property({ type: String }) yLabel = "Y";

  @property({ type: Boolean }) showAxes = true;

  @property({ type: Boolean }) showGrid = true;

  @query(".chart-host") private _chartHost?: HTMLElement;

  @query(".chart-root") private _chartRoot?: HTMLElement;

  private _chart: echarts.ECharts | null = null;

  private _resizeObserver: ResizeObserver | null = null;

  private _initRetryTimer?: ReturnType<typeof setTimeout>;

  private _initRetries = 0;

  disconnectedCallback(): void {
    if (this._initRetryTimer) clearTimeout(this._initRetryTimer);
    this._teardownChart();
    super.disconnectedCallback();
  }

  protected firstUpdated(): void {
    this._setupResizeObserver();
    this._scheduleEnsureChart();
  }

  private _scheduleEnsureChart(): void {
    if (this._ensureChart()) {
      this._initRetries = 0;
      return;
    }
    if (this._initRetries >= 40) return;
    this._initRetries += 1;
    if (this._initRetryTimer) clearTimeout(this._initRetryTimer);
    this._initRetryTimer = setTimeout(() => {
      this._initRetryTimer = undefined;
      this._scheduleEnsureChart();
    }, 120);
  }

  protected updated(changed: PropertyValues): void {
    if (changed.has("points") || changed.has("bars")) {
      this._renderChart(this._chart ? "data" : "full");
      return;
    }
    if (
      changed.has("mode") ||
      changed.has("lineWidth") ||
      changed.has("lineColor") ||
      changed.has("symbolSize") ||
      changed.has("pointColor") ||
      changed.has("barColor") ||
      changed.has("xLabel") ||
      changed.has("yLabel") ||
      changed.has("showAxes") ||
      changed.has("showGrid") ||
      changed.has("bgColor")
    ) {
      this._renderChart("full");
    }
  }

  private _setupResizeObserver(): void {
    if (typeof ResizeObserver === "undefined") return;
    this._resizeObserver = new ResizeObserver(() => this._resizeChart());
    if (this._chartRoot) this._resizeObserver.observe(this._chartRoot);
    if (this._chartHost) this._resizeObserver.observe(this._chartHost);
    this._resizeObserver.observe(this);
  }

  private _teardownChart(): void {
    try {
      this._resizeObserver?.disconnect();
    } catch {
      // ignore
    }
    this._resizeObserver = null;
    try {
      this._chart?.dispose();
    } catch {
      // ignore
    }
    this._chart = null;
  }

  private _showAxesEnabled(): boolean {
    return this.showAxes !== false;
  }

  private _showGridEnabled(): boolean {
    return this.showGrid !== false;
  }

  private _buildOption() {
    const showAxes = this._showAxesEnabled();
    const showGrid = this._showGridEnabled();

    if (this.mode === "bar") {
      const bars = Array.isArray(this.bars) ? this.bars : [];
      return {
        backgroundColor: "transparent",
        tooltip: { trigger: "axis" as const },
        grid: {
          left: 8,
          right: 8,
          top: 24,
          bottom: 8,
          containLabel: true,
        },
        xAxis: {
          type: "category" as const,
          data: bars.map((bar) => bar.name),
          name: showAxes ? this.xLabel || "" : "",
          show: showAxes,
          axisLine: { show: showAxes, lineStyle: { color: AXIS_LINE_COLOR } },
          axisTick: { show: showAxes, lineStyle: { color: AXIS_LINE_COLOR } },
          axisLabel: { show: showAxes, color: AXIS_LABEL_COLOR },
          nameTextStyle: { color: AXIS_NAME_COLOR },
        },
        yAxis: {
          type: "value" as const,
          name: showAxes ? this.yLabel || "Y" : "",
          show: showAxes,
          axisLine: { show: showAxes, lineStyle: { color: AXIS_LINE_COLOR } },
          axisTick: { show: showAxes, lineStyle: { color: AXIS_LINE_COLOR } },
          axisLabel: { show: showAxes, color: AXIS_LABEL_COLOR },
          splitLine: { show: showGrid, lineStyle: { color: SPLIT_LINE_COLOR } },
          nameTextStyle: { color: AXIS_NAME_COLOR },
        },
        series: [
          {
            type: "bar" as const,
            data: bars.map((bar) => ({
              value: bar.value,
              itemStyle: {
                color: bar.color || this.barColor || "#03a9f4",
                borderRadius: [4, 4, 0, 0],
              },
            })),
          },
        ],
      };
    }

    const data = Array.isArray(this.points) ? this.points : [];
    const series =
      this.mode === "scatter"
        ? [
            {
              type: "scatter" as const,
              data,
              symbolSize: Number(this.symbolSize) || 8,
              itemStyle: { color: this.pointColor || "#03a9f4", opacity: 0.92 },
            },
          ]
        : [
            {
              type: "line" as const,
              data,
              showSymbol: false,
              lineStyle: {
                width: Number(this.lineWidth) || 2,
                color: this.lineColor || "#03a9f4",
              },
            },
          ];

    return {
      backgroundColor: "transparent",
      tooltip: { trigger: "axis" as const },
      grid: {
        left: 8,
        right: 8,
        top: 24,
        bottom: 8,
        containLabel: true,
      },
      xAxis: {
        type: "value" as const,
        name: showAxes ? this.xLabel || "X" : "",
        show: showAxes,
        axisLine: { show: showAxes, lineStyle: { color: AXIS_LINE_COLOR } },
        axisTick: { show: showAxes, lineStyle: { color: AXIS_LINE_COLOR } },
        axisLabel: { show: showAxes, color: AXIS_LABEL_COLOR },
        splitLine: { show: showGrid, lineStyle: { color: SPLIT_LINE_COLOR } },
        nameTextStyle: { color: AXIS_NAME_COLOR },
      },
      yAxis: {
        type: "value" as const,
        name: showAxes ? this.yLabel || "Y" : "",
        show: showAxes,
        axisLine: { show: showAxes, lineStyle: { color: AXIS_LINE_COLOR } },
        axisTick: { show: showAxes, lineStyle: { color: AXIS_LINE_COLOR } },
        axisLabel: { show: showAxes, color: AXIS_LABEL_COLOR },
        splitLine: { show: showGrid, lineStyle: { color: SPLIT_LINE_COLOR } },
        nameTextStyle: { color: AXIS_NAME_COLOR },
      },
      series,
    };
  }

  private _renderChart(mode: "full" | "data" = "data"): void {
    if (!this._chart) return;
    if (mode === "full") {
      this._chart.setOption(this._buildOption(), { notMerge: true });
      return;
    }

    if (this.mode === "bar") {
      const bars = Array.isArray(this.bars) ? this.bars : [];
      this._chart.setOption({
        xAxis: { data: bars.map((bar) => bar.name) },
        series: [
          {
            data: bars.map((bar) => ({
              value: bar.value,
              itemStyle: {
                color: bar.color || this.barColor || "#03a9f4",
                borderRadius: [4, 4, 0, 0],
              },
            })),
          },
        ],
      });
      return;
    }

    const data = Array.isArray(this.points) ? this.points : [];
    this._chart.setOption({
      series: [{ data }],
    });
  }

  private _ensureChart(): boolean {
    const el = this._chartHost;
    if (!el || !hasDomSize(el)) return false;
    if (!this._chart) {
      this._chart = echarts.init(el);
    }
    this._renderChart("full");
    return true;
  }

  private _resizeChart(): void {
    if (!hasDomSize(this._chartHost) && !hasDomSize(this._chartRoot)) {
      this._scheduleEnsureChart();
      return;
    }
    if (!this._chart) {
      this._ensureChart();
      return;
    }
    try {
      this._chart.resize();
    } catch {
      // ignore
    }
  }

  private _pointCount(): number {
    if (this.mode === "bar") return this.bars?.length ?? 0;
    return this.points?.length ?? 0;
  }

  protected render() {
    const shellStyle = styleMap({
      ...(this.bgColor ? { backgroundColor: this.bgColor } : {}),
    });

    const metaLabel = this.mode === "bar" ? "bars" : "pts";

    return html`
      <div class="chart-root chart-interactive" style=${shellStyle}>
        <div class="chart-host chart-interactive"></div>
        <div class="chart-meta">${this._pointCount()} ${metaLabel}</div>
      </div>
    `;
  }

  static styles = css`
    :host {
      display: block;
      width: 100%;
      height: 100%;
      min-width: 0;
      min-height: 160px;
    }
    .chart-root {
      position: relative;
      width: 100%;
      height: 100%;
      min-width: 0;
      min-height: 160px;
      box-sizing: border-box;
      overflow: hidden;
      background: var(--card-background-color, #fff);
      border-radius: inherit;
    }
    .chart-host {
      position: absolute;
      inset: 0;
      width: 100%;
      height: 100%;
    }
    .chart-meta {
      position: absolute;
      right: 8px;
      bottom: 6px;
      color: var(--secondary-text-color, rgba(0, 0, 0, 0.6));
      font-size: 11px;
      pointer-events: none;
      z-index: 2;
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "ha-chart-2d": HaChart2d;
  }
}
