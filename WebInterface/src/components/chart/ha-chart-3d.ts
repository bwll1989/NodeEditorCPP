import { css, html, LitElement, type PropertyValues } from "lit";
import { customElement, property, query, state } from "lit/decorators.js";
import { styleMap } from "lit/directives/style-map.js";
import * as echarts from "echarts/core";
import { TooltipComponent } from "echarts/components";
import { CanvasRenderer } from "echarts/renderers";
import { Line3DChart, Scatter3DChart } from "echarts-gl/charts";
import { Grid3DComponent } from "echarts-gl/components";
import type { Point3 } from "../../common/chart/point3d";

echarts.use([TooltipComponent, CanvasRenderer, Line3DChart, Scatter3DChart, Grid3DComponent]);

type ChartMode = "line" | "scatter";

const DEFAULT_VIEW_ALPHA = 25;
const DEFAULT_VIEW_BETA = 40;

function hasDomSize(el: HTMLElement | null | undefined): boolean {
  return !!el && el.clientWidth > 0 && el.clientHeight > 0;
}

const AXIS_LINE_COLOR = "rgba(0, 0, 0, 0.2)";
const AXIS_LABEL_COLOR = "rgba(0, 0, 0, 0.54)";
const AXIS_NAME_COLOR = "rgba(0, 0, 0, 0.6)";
const SPLIT_LINE_COLOR = "rgba(0, 0, 0, 0.08)";

@customElement("ha-chart-3d")
export class HaChart3d extends LitElement {
  @property({ type: String }) mode: ChartMode = "line";

  @property({ attribute: false }) points: Point3[] = [];

  @property({ type: String }) bgColor = "";

  @property({ type: Number }) lineWidth = 3;

  @property({ type: String }) lineColor = "#03a9f4";

  @property({ type: Number }) symbolSize = 8;

  @property({ type: String }) pointColor = "#03a9f4";

  @property({ type: String }) xLabel = "X";

  @property({ type: String }) yLabel = "Y";

  @property({ type: String }) zLabel = "Z";

  @property({ type: Boolean }) showAxes = true;

  @property({ type: Boolean }) autoRotate = false;

  @state() private _viewAlpha = DEFAULT_VIEW_ALPHA;

  @state() private _viewBeta = DEFAULT_VIEW_BETA;

  @query(".chart-host") private _chartHost?: HTMLElement;

  @query(".chart-root") private _chartRoot?: HTMLElement;

  private _chart: echarts.ECharts | null = null;

  private _resizeObserver: ResizeObserver | null = null;

  private _initRetryTimer?: ReturnType<typeof setTimeout>;

  private _initRetries = 0;

  private _pointerDragging = false;

  private _pointerLastX = 0;

  private _pointerLastY = 0;

  private _pointerBoundDom: HTMLElement | null = null;

  private _onChartPointerDown = (ev: PointerEvent): void => {
    if (ev.button !== 0) return;
    this._pointerDragging = true;
    this._pointerLastX = ev.clientX;
    this._pointerLastY = ev.clientY;
    try {
      (ev.currentTarget as HTMLElement).setPointerCapture(ev.pointerId);
    } catch {
      // ignore
    }
    ev.preventDefault();
    ev.stopPropagation();
  };

  private _onChartPointerMove = (ev: PointerEvent): void => {
    if (!this._pointerDragging) return;
    const dx = ev.clientX - this._pointerLastX;
    const dy = ev.clientY - this._pointerLastY;
    if (dx === 0 && dy === 0) return;

    this._pointerLastX = ev.clientX;
    this._pointerLastY = ev.clientY;
    this._viewBeta += dx * 0.4;
    this._viewAlpha = Math.max(-90, Math.min(90, this._viewAlpha + dy * 0.4));
    this._applyViewAngles(false);
    ev.preventDefault();
    ev.stopPropagation();
  };

  private _onChartPointerUp = (ev: PointerEvent): void => {
    if (!this._pointerDragging) return;
    this._pointerDragging = false;
    try {
      (ev.currentTarget as HTMLElement).releasePointerCapture(ev.pointerId);
    } catch {
      // ignore
    }
    ev.stopPropagation();
  };

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
    if (changed.has("points")) {
      this._renderChart(this._chart ? "data" : "full");
      return;
    }
    if (
      changed.has("mode") ||
      changed.has("lineWidth") ||
      changed.has("lineColor") ||
      changed.has("symbolSize") ||
      changed.has("pointColor") ||
      changed.has("xLabel") ||
      changed.has("yLabel") ||
      changed.has("zLabel") ||
      changed.has("showAxes") ||
      changed.has("autoRotate") ||
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
    this._unbindPointerHandlers();
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

  private _axisOptions(showAxes: boolean) {
    const axisLine = showAxes
      ? { show: true, lineStyle: { color: AXIS_LINE_COLOR } }
      : { show: false };
    const axisTick = showAxes
      ? { show: true, lineStyle: { color: AXIS_LINE_COLOR } }
      : { show: false };
    const axisLabel = showAxes ? { show: true, color: AXIS_LABEL_COLOR } : { show: false };
    const splitLine = showAxes
      ? { show: true, lineStyle: { color: SPLIT_LINE_COLOR } }
      : { show: false };

    return {
      grid3D: {
        show: showAxes,
        axisLine,
        axisTick,
        axisLabel,
        splitLine,
        splitArea: { show: false },
      },
      xAxis3D: {
        type: "value" as const,
        name: showAxes ? this.xLabel || "X" : "",
        show: showAxes,
        axisLine,
        axisTick,
        axisLabel,
        nameTextStyle: { color: AXIS_NAME_COLOR },
      },
      yAxis3D: {
        type: "value" as const,
        name: showAxes ? this.yLabel || "Y" : "",
        show: showAxes,
        axisLine,
        axisTick,
        axisLabel,
        nameTextStyle: { color: AXIS_NAME_COLOR },
      },
      zAxis3D: {
        type: "value" as const,
        name: showAxes ? this.zLabel || "Z" : "",
        show: showAxes,
        axisLine,
        axisTick,
        axisLabel,
        nameTextStyle: { color: AXIS_NAME_COLOR },
      },
    };
  }

  private _buildOption(includeView = true) {
    const data = Array.isArray(this.points) ? this.points : [];
    const showAxes = this._showAxesEnabled();
    const axis = this._axisOptions(showAxes);
    const series =
      this.mode === "scatter"
        ? [
            {
              type: "scatter3D" as const,
              data,
              silent: true,
              symbolSize: Number(this.symbolSize) || 8,
              itemStyle: {
                color: this.pointColor || "#03a9f4",
                opacity: 0.92,
              },
              emphasis: {
                itemStyle: { color: "#ff9800" },
              },
            },
          ]
        : [
            {
              type: "line3D" as const,
              data,
              silent: true,
              lineStyle: {
                width: Number(this.lineWidth) || 3,
                color: this.lineColor || "#03a9f4",
                opacity: 0.95,
              },
            },
          ];

    const viewControl = {
      projection: "perspective" as const,
      autoRotate: !!this.autoRotate,
      autoRotateSpeed: 8,
      distance: 200,
      rotate: true,
      zoom: true,
      pan: true,
      rotateSensitivity: 1,
      zoomSensitivity: 1,
      panSensitivity: 1,
      ...(includeView
        ? {
            alpha: this._viewAlpha,
            beta: this._viewBeta,
          }
        : {}),
    };

    return {
      backgroundColor: "transparent",
      tooltip: {},
      grid3D: {
        boxWidth: 100,
        boxHeight: 100,
        boxDepth: 100,
        ...axis.grid3D,
        viewControl,
        light: {
          main: { intensity: 1.1, shadow: false },
          ambient: { intensity: 0.55 },
        },
      },
      xAxis3D: axis.xAxis3D,
      yAxis3D: axis.yAxis3D,
      zAxis3D: axis.zAxis3D,
      series,
    };
  }

  private _renderChart(mode: "full" | "data" = "data"): void {
    if (!this._chart) return;
    if (mode === "full") {
      this._chart.setOption(this._buildOption(true), { notMerge: true });
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
      this._bindPointerHandlers();
    }
    this._renderChart("full");
    return true;
  }

  private _bindPointerHandlers(): void {
    const dom = this._chart?.getZr()?.dom as HTMLElement | undefined;
    if (!dom || this._pointerBoundDom === dom) return;
    this._unbindPointerHandlers();
    this._pointerBoundDom = dom;
    dom.addEventListener("pointerdown", this._onChartPointerDown);
    dom.addEventListener("pointermove", this._onChartPointerMove);
    dom.addEventListener("pointerup", this._onChartPointerUp);
    dom.addEventListener("pointercancel", this._onChartPointerUp);
  }

  private _unbindPointerHandlers(): void {
    if (!this._pointerBoundDom) return;
    const dom = this._pointerBoundDom;
    dom.removeEventListener("pointerdown", this._onChartPointerDown);
    dom.removeEventListener("pointermove", this._onChartPointerMove);
    dom.removeEventListener("pointerup", this._onChartPointerUp);
    dom.removeEventListener("pointercancel", this._onChartPointerUp);
    this._pointerBoundDom = null;
  }

  private _applyViewAngles(animate = true): void {
    if (!this._chart) {
      this._ensureChart();
      return;
    }
    const showAxes = this._showAxesEnabled();
    const axis = this._axisOptions(showAxes);
    try {
      this._chart.setOption({
        grid3D: {
          ...axis.grid3D,
          viewControl: {
            alpha: this._viewAlpha,
            beta: this._viewBeta,
            animation: animate,
            animationDurationUpdate: animate ? 280 : 0,
          },
        },
        xAxis3D: axis.xAxis3D,
        yAxis3D: axis.yAxis3D,
        zAxis3D: axis.zAxis3D,
      });
    } catch {
      // ignore
    }
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

  protected render() {
    const shellStyle = styleMap({
      ...(this.bgColor ? { backgroundColor: this.bgColor } : {}),
    });

    return html`
      <div class="chart-root chart-interactive" style=${shellStyle}>
        <div class="chart-host chart-interactive"></div>
        <div class="chart-meta">${this.points?.length ?? 0} pts</div>
      </div>
    `;
  }

  static styles = css`
    :host {
      display: block;
      width: 100%;
      height: 100%;
      min-width: 0;
      min-height: 180px;
    }
    .chart-root {
      position: relative;
      width: 100%;
      height: 100%;
      min-width: 0;
      min-height: 180px;
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
      touch-action: none;
      cursor: grab;
    }
    .chart-host:active {
      cursor: grabbing;
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
    "ha-chart-3d": HaChart3d;
  }
}
