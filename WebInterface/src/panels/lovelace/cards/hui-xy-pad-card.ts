import { css, html, LitElement, nothing } from "lit";
import { customElement, property, state } from "lit/decorators.js";
import { classMap } from "lit/directives/class-map.js";
import { styleMap } from "lit/directives/style-map.js";
import type { Flow, LovelaceCard, LovelaceCardConfig, LovelaceGridOptions } from "../../../types";
import {
  centerXyPoint,
  formatXyDisplay,
  parseXyValue,
  XY_PAD_DEFAULTS,
  xyToFlowValue,
  type XyPoint,
} from "../../../common/entity/xy-pad";
import { computeFeatureColor } from "../../../common/entity/tile-color";
import { tileCardHostStyle, tileCardStyle } from "../../../styles/tile-card-style";
import "../../../components/ha-tile-container";
import "../../../components/ha-tile-icon";
import "../../../components/ha-tile-info";
import "../../../components/ha-control-xy-pad";
import "../../../components/ha-card";

function configNumber(value: unknown, fallback: number): number {
  if (value === undefined || value === null || value === "") return fallback;
  const parsed = Number(value);
  return Number.isNaN(parsed) ? fallback : parsed;
}

@customElement("hui-xy-pad-card")
export class HuiXyPadCard extends LitElement implements LovelaceCard {
  public static getStubConfig(): LovelaceCardConfig {
    return {
      type: "xy-pad",
      entity: "/demo/xy",
      name: "XY Pad",
      icon: "mdi:axis-arrow",
      x_min: XY_PAD_DEFAULTS.x_min,
      x_max: XY_PAD_DEFAULTS.x_max,
      y_min: XY_PAD_DEFAULTS.y_min,
      y_max: XY_PAD_DEFAULTS.y_max,
      step: XY_PAD_DEFAULTS.step,
      invert_y: XY_PAD_DEFAULTS.invert_y,
      show_grid: XY_PAD_DEFAULTS.show_grid,
      snap_center: XY_PAD_DEFAULTS.snap_center,
    };
  }

  @property({ attribute: false }) public flow?: Flow;

  @state() private _config?: LovelaceCardConfig;

  /** Local drag value so remote updates don't fight while dragging. */
  @state() private _local?: XyPoint;

  setConfig(config: LovelaceCardConfig): void {
    this._config = config;
    this._local = undefined;
    this.requestUpdate();
  }

  getGridOptions(): LovelaceGridOptions {
    const vertical = Boolean(this._config?.vertical);
    const rows = vertical ? 4 : 3;
    return {
      columns: 6,
      rows,
      min_columns: vertical ? 3 : 6,
      min_rows: rows,
    };
  }

  private _ranges() {
    return {
      xMin: configNumber(this._config?.x_min, XY_PAD_DEFAULTS.x_min),
      xMax: configNumber(this._config?.x_max, XY_PAD_DEFAULTS.x_max),
      yMin: configNumber(this._config?.y_min, XY_PAD_DEFAULTS.y_min),
      yMax: configNumber(this._config?.y_max, XY_PAD_DEFAULTS.y_max),
      step: configNumber(this._config?.step, XY_PAD_DEFAULTS.step),
      invertY:
        this._config?.invert_y === undefined
          ? XY_PAD_DEFAULTS.invert_y
          : Boolean(this._config.invert_y),
      showGrid:
        this._config?.show_grid === undefined
          ? XY_PAD_DEFAULTS.show_grid
          : Boolean(this._config.show_grid),
      snapCenter: Boolean(this._config?.snap_center),
    };
  }

  private _entityX(): string | undefined {
    const dual = this._config?.entity_x;
    if (typeof dual === "string" && dual.trim()) return dual.trim();
    return undefined;
  }

  private _entityY(): string | undefined {
    const dual = this._config?.entity_y;
    if (typeof dual === "string" && dual.trim()) return dual.trim();
    return undefined;
  }

  private _usesDualEntities(): boolean {
    return Boolean(this._entityX() && this._entityY());
  }

  private _readRemotePoint(): XyPoint {
    const { xMin, xMax, yMin, yMax } = this._ranges();
    const fallback = centerXyPoint(xMin, xMax, yMin, yMax);

    if (!this.flow) return fallback;

    if (this._usesDualEntities()) {
      const xEntity = this._entityX()!;
      const yEntity = this._entityY()!;
      const xRaw = this.flow.states[xEntity]?.state;
      const yRaw = this.flow.states[yEntity]?.state;
      const x = typeof xRaw === "number" ? xRaw : Number(xRaw);
      const y = typeof yRaw === "number" ? yRaw : Number(yRaw);
      return {
        x: Number.isNaN(x) ? fallback.x : x,
        y: Number.isNaN(y) ? fallback.y : y,
      };
    }

    const entity = this._config?.entity;
    if (!entity) return fallback;
    return parseXyValue(this.flow.states[entity]?.state) ?? fallback;
  }

  private _currentPoint(): XyPoint {
    return this._local ?? this._readRemotePoint();
  }

  private async _writePoint(point: XyPoint): Promise<void> {
    if (!this.flow) return;

    if (this._usesDualEntities()) {
      const xEntity = this._entityX()!;
      const yEntity = this._entityY()!;
      await Promise.all([
        this.flow.callService(xEntity, point.x),
        this.flow.callService(yEntity, point.y),
      ]);
      return;
    }

    const entity = this._config?.entity;
    if (!entity) return;
    await this.flow.callService(entity, xyToFlowValue(point));
  }

  private async _onValueChanged(ev: CustomEvent<XyPoint>): Promise<void> {
    ev.stopPropagation();
    this._local = ev.detail;
    await this._writePoint(ev.detail);
  }

  private async _onDragEnd(ev: CustomEvent<XyPoint>): Promise<void> {
    ev.stopPropagation();
    const { snapCenter, xMin, xMax, yMin, yMax } = this._ranges();
    if (snapCenter) {
      const center = centerXyPoint(xMin, xMax, yMin, yMax);
      this._local = center;
      await this._writePoint(center);
    }
    // Allow remote state to take over after a short settle.
    window.setTimeout(() => {
      this._local = undefined;
    }, 250);
  }

  protected render() {
    if (!this._config) return nothing;

    const ranges = this._ranges();
    const point = this._currentPoint();
    const dual = this._usesDualEntities();
    const entity = dual ? this._entityX() : this._config.entity;
    const state = entity && this.flow ? this.flow.states[entity] : undefined;
    const vertical = Boolean(this._config.vertical);
    const name = String(
      this._config.name ?? state?.attributes?.friendly_name ?? entity ?? "XY Pad",
    );
    const icon = String(this._config.icon ?? state?.attributes?.icon ?? "mdi:axis-arrow");
    const secondary = formatXyDisplay(point);
    const active = true;
    const tileColor = computeFeatureColor(active, this._config.color, "primary");
    const colorStyle = styleMap({ "--tile-color": tileColor });
    const featureStyle = styleMap({
      "--feature-color": tileColor ?? "var(--primary-color)",
    });

    return html`
      <ha-card class=${classMap({ active })} style=${colorStyle}>
        <ha-tile-container ?vertical=${vertical} expand-features>
          <ha-tile-icon slot="icon" .icon=${icon} .active=${active}></ha-tile-icon>
          <ha-tile-info slot="info" .primary=${name} .secondary=${secondary}></ha-tile-info>
          <div slot="features" class="feature" style=${featureStyle}>
            <ha-control-xy-pad
              .x=${point.x}
              .y=${point.y}
              .xMin=${ranges.xMin}
              .xMax=${ranges.xMax}
              .yMin=${ranges.yMin}
              .yMax=${ranges.yMax}
              .step=${ranges.step}
              ?invert-y=${ranges.invertY}
              ?show-grid=${ranges.showGrid}
              @value-changed=${this._onValueChanged}
              @drag-end=${this._onDragEnd}
              @click=${(ev: Event) => ev.stopPropagation()}
            ></ha-control-xy-pad>
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
        flex: 1 1 auto;
        min-height: 120px;
        height: 100%;
        display: flex;
        flex-direction: column;
      }
      ha-control-xy-pad {
        flex: 1 1 auto;
        width: 100%;
        min-height: 120px;
        height: 100%;
      }
    `,
  ];
}

declare global {
  interface HTMLElementTagNameMap {
    "hui-xy-pad-card": HuiXyPadCard;
  }
}
