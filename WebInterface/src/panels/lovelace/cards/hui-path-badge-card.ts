import { css, html, LitElement, nothing, type PropertyValues } from "lit";
import { customElement, property, state } from "lit/decorators.js";
import { computeCssColor } from "../../../common/color/theme-colors";
import { stateActive } from "../../../common/entity/state-active";
import type {
  Flow,
  FlowEntityState,
  LovelaceCard,
  LovelacePathBadgeConfig,
  LovelaceCardConfig,
  LovelaceGridOptions,
  LovelacePathPoint,
  LovelacePictureElementConfig,
} from "../../../types";
import { DEFAULT_FLOORPLAN } from "../picture-elements/default-floorplan";
import { formatPictureBadgeState } from "../picture-elements/format-state";
import { badgeShowIcon, badgeShowText } from "../heading-badges/badge-flags";
import "../../../components/ha-card";
import "../../../components/ha-state-badge";
import "../picture-elements/hui-picture-action-element";

const DEFAULT_POINTS: LovelacePathPoint[] = [
  { x: 10, y: 88 },
  { x: 28, y: 18 },
  { x: 72, y: 18 },
  { x: 90, y: 88 },
];
const DEFAULT_TRACK_COLOR = "primary";
const DEFAULT_BADGE_COLOR = "accent";
const DEFAULT_ICON = "mdi:map-marker";
const DEFAULT_BADGE_SIZE = 44;
const DEFAULT_TRACK_WIDTH = 1;
const SVG_NS = "http://www.w3.org/2000/svg";
const DEFAULT_PROGRESS_MIN = 0;
const DEFAULT_PROGRESS_MAX = 1;
const DEFAULT_BADGES: LovelacePathBadgeConfig[] = [
  {
    type: "state-badge",
    entity: "/demo/path_progress",
    name: "轨迹点 1",
    icon: "mdi:robot-industrial",
    color: DEFAULT_BADGE_COLOR,
    show_icon: true,
    show_name: false,
    show_state: false,
  },
];

function clampPercent(value: number): number {
  return Math.min(100, Math.max(0, value));
}

function normalizePoint(point: LovelacePathPoint): LovelacePathPoint {
  return {
    x: Number(clampPercent(Number(point.x)).toFixed(2)),
    y: Number(clampPercent(Number(point.y)).toFixed(2)),
  };
}

function buildPath(points: LovelacePathPoint[]): string {
  if (points.length === 0) return "";
  if (points.length === 1) {
    const point = normalizePoint(points[0]);
    return `M ${point.x} ${point.y}`;
  }
  if (points.length === 2) {
    const [start, end] = points.map(normalizePoint);
    return `M ${start.x} ${start.y} L ${end.x} ${end.y}`;
  }

  const normalized = points.map(normalizePoint);
  let d = `M ${normalized[0].x} ${normalized[0].y}`;
  for (let index = 1; index < normalized.length - 1; index += 1) {
    const current = normalized[index];
    const next = normalized[index + 1];
    const midX = Number(((current.x + next.x) / 2).toFixed(2));
    const midY = Number(((current.y + next.y) / 2).toFixed(2));
    d += ` Q ${current.x} ${current.y} ${midX} ${midY}`;
  }
  const penultimate = normalized[normalized.length - 2];
  const last = normalized[normalized.length - 1];
  d += ` Q ${penultimate.x} ${penultimate.y} ${last.x} ${last.y}`;
  return d;
}

function normalizeBadge(badge: LovelacePathBadgeConfig): LovelacePathBadgeConfig {
  return {
    entity: String(badge.entity ?? "").trim(),
    type: String(badge.type ?? "state-badge").trim() || "state-badge",
    ...(String(badge.name ?? "").trim() ? { name: String(badge.name).trim() } : {}),
    ...(String(badge.icon ?? "").trim() ? { icon: String(badge.icon).trim() } : {}),
    ...(String(badge.color ?? "").trim() ? { color: String(badge.color).trim() } : {}),
    ...(String(badge.unit ?? "").trim() ? { unit: String(badge.unit).trim() } : {}),
    ...(String(badge.text ?? "").trim() ? { text: String(badge.text).trim() } : {}),
    ...(badge.show_icon === false ? { show_icon: false } : {}),
    ...(typeof badge.show_name === "boolean" ? { show_name: badge.show_name } : {}),
    ...(String(badge.attribute ?? "").trim()
      ? { attribute: String(badge.attribute).trim() }
      : {}),
    ...(typeof badge.show_state === "boolean" ? { show_state: badge.show_state } : {}),
    ...(badge.value !== undefined ? { value: badge.value } : {}),
  };
}

function pointAtProgress(pathData: string, progress: number): LovelacePathPoint | undefined {
  if (!pathData.trim()) return undefined;
  try {
    const path = document.createElementNS(SVG_NS, "path");
    path.setAttribute("d", pathData);
    const totalLength = path.getTotalLength();
    if (!Number.isFinite(totalLength) || totalLength <= 0) return undefined;
    const point = path.getPointAtLength(totalLength * progress);
    return { x: point.x, y: point.y };
  } catch {
    return undefined;
  }
}

@customElement("hui-path-badge-card")
export class HuiPathBadgeCard extends LitElement implements LovelaceCard {
  /**
   * 提供卡片选择器和新建卡片时的默认配置。
   */
  public static getStubConfig(): LovelaceCardConfig {
    return {
      type: "path-badge",
      image: DEFAULT_FLOORPLAN,
      points: DEFAULT_POINTS.map((point) => ({ ...point })),
      path: buildPath(DEFAULT_POINTS),
      badges: DEFAULT_BADGES.map((badge) => ({ ...badge })),
      track_color: DEFAULT_TRACK_COLOR,
      badge_size: DEFAULT_BADGE_SIZE,
      track_width: DEFAULT_TRACK_WIDTH,
    };
  }

  @property({ attribute: false }) public flow?: Flow;

  @state() private _config?: LovelaceCardConfig;

  @state() private _progressRevision = 0;

  private _entityUnsubs: Array<() => void> = [];

  /**
   * 校验并保存卡片配置。
   */
  public setConfig(config: LovelaceCardConfig): void {
    const points = Array.isArray(config.points)
      ? (config.points as LovelacePathPoint[])
      : [];
    const badges = this._badgesFromConfig(config);
    const path = points.length > 0 ? buildPath(points) : String(config.path ?? "");
    if (!path.trim()) {
      throw new Error("Path badge card requires a path");
    }
    if (badges.length === 0) {
      throw new Error("Path badge card requires at least one badge");
    }
    this._config = {
      badge_size: DEFAULT_BADGE_SIZE,
      track_width: DEFAULT_TRACK_WIDTH,
      track_color: DEFAULT_TRACK_COLOR,
      progress_min: DEFAULT_PROGRESS_MIN,
      progress_max: DEFAULT_PROGRESS_MAX,
      ...config,
      badges,
      ...(points.length > 0 ? { points: points.map(normalizePoint) } : {}),
      path,
    };
    this._syncEntitySubscription();
    this.requestUpdate();
  }

  /**
   * 返回适合该卡片的默认网格尺寸。
   */
  public getGridOptions(): LovelaceGridOptions {
    return {
      columns: 12,
      rows: "auto",
      min_columns: 6,
      min_rows: 3,
    };
  }

  /**
   * 建立实体订阅，保证数值变化时徽章位置实时更新。
   */
  connectedCallback(): void {
    super.connectedCallback();
    this._syncEntitySubscription();
  }

  /**
   * 清理实体订阅。
   */
  disconnectedCallback(): void {
    this._clearEntitySubscription();
    super.disconnectedCallback();
  }

  /**
   * 在配置或远程状态变化后重新计算徽章位置。
   */
  protected updated(changed: PropertyValues): void {
    super.updated(changed);
    if (changed.has("flow")) {
      this._syncEntitySubscription();
    }
  }

  /**
   * 渲染轨迹卡片，包括底图、轨迹线和沿轨迹移动的徽章。
   */
  protected render() {
    if (!this._config) return nothing;
    void this.flow?.statesRevision;

    const title = String(this._config.title ?? "");
    const image = String(this._config.image ?? "");
    const badges = this._activeBadgesFromConfig(this._config);
    const pathData = this._pathData();
    const badgeSize = Number(this._config.badge_size ?? DEFAULT_BADGE_SIZE);
    const trackColor = computeCssColor(String(this._config.track_color ?? DEFAULT_TRACK_COLOR));
    const caption = String(this._config.caption ?? "").trim();
    const badgeScale = Math.max(0.5, badgeSize / 56);

    return html`
      <ha-card>
        ${title ? html`<h1 class="card-header">${title}</h1>` : nothing}
        <div class="root">
          ${image
            ? html`<img class="background" alt="" src=${image} draggable="false" />`
            : html`<div class="placeholder">添加底图后，可让徽章沿轨迹运动</div>`}

          <svg class="overlay" viewBox="0 0 100 100" preserveAspectRatio="none" aria-hidden="true">
            <path
              id="motion-path"
              d=${pathData}
              fill="none"
              stroke=${trackColor}
              stroke-width=${String(this._config.track_width ?? DEFAULT_TRACK_WIDTH)}
              stroke-linecap="round"
              stroke-linejoin="round"
            ></path>
          </svg>

          ${badges.map((badge) => {
            const state = this.flow?.states[badge.entity];
            const badgeTitle = String(
              badge.name ?? state?.attributes?.friendly_name ?? badge.entity ?? "轨迹徽章",
            );
            const point = pointAtProgress(pathData, this._progressForBadge(badge));

            return point
              ? html`
                <div
                  title=${badgeTitle}
                  style=${[
                    `left:${point.x}%`,
                    `top:${point.y}%`,
                    `--badge-scale:${badgeScale}`,
                  ].join(";")}
                  class=${this._isButtonBadge(badge) ? "badge interactive" : "badge"}
                >
                  ${this._renderBadgeContent(badge, state)}
                </div>
              `
                  : nothing;
          })}

          ${caption
            ? html`<div class="caption">${caption}</div>`
            : nothing}
        </div>
      </ha-card>
    `;
  }

  /**
   * 从配置中解析可用的徽章列表，同时兼容旧版单徽章配置。
   */
  private _badgesFromConfig(config: LovelaceCardConfig): LovelacePathBadgeConfig[] {
    const badges = Array.isArray(config.badges)
      ? (config.badges as LovelacePathBadgeConfig[]).map(normalizeBadge)
      : [];
    if (badges.length > 0) {
      return badges;
    }

    const legacyEntity = String(config.entity ?? "").trim();
    if (!legacyEntity) {
      return [];
    }

    return [
      normalizeBadge({
        entity: legacyEntity,
        type: "state-badge",
        name: String(config.name ?? "").trim() || undefined,
        icon: String(config.icon ?? "").trim() || undefined,
        color: String(config.badge_color ?? "").trim() || DEFAULT_BADGE_COLOR,
        show_icon: true,
        show_name: false,
        attribute: String(config.attribute ?? "").trim() || undefined,
        show_state: Boolean(config.show_state),
      }),
    ];
  }

  /**
   * 仅返回已配置实体的徽章，用于渲染和订阅。
   */
  private _activeBadgesFromConfig(config: LovelaceCardConfig): LovelacePathBadgeConfig[] {
    return this._badgesFromConfig(config).filter((badge) => badge.entity);
  }

  /**
   * 返回当前应使用的 SVG 轨迹数据。
   */
  private _pathData(): string {
    if (!this._config) return "";
    const points = Array.isArray(this._config.points)
      ? (this._config.points as LovelacePathPoint[])
      : [];
    if (points.length > 0) {
      return buildPath(points);
    }
    return String(this._config.path ?? "");
  }

  /**
   * 将实体值或属性值转换为 0-1 范围内的轨迹进度。
   */
  private _progressForBadge(badge: LovelacePathBadgeConfig): number {
    if (!this.flow) return 0;
    const state = this.flow.states[badge.entity];
    if (!state) return 0;

    const attributeName = String(badge.attribute ?? "").trim();
    const rawValue =
      attributeName && attributeName in state.attributes
        ? state.attributes[attributeName]
        : state.state;
    const numeric = Number(rawValue);
    if (!Number.isFinite(numeric)) return 0;

    const progressMin = Number(this._config?.progress_min ?? DEFAULT_PROGRESS_MIN);
    const progressMax = Number(this._config?.progress_max ?? DEFAULT_PROGRESS_MAX);
    if (!Number.isFinite(progressMin) || !Number.isFinite(progressMax)) {
      return Math.min(1, Math.max(0, numeric));
    }
    if (progressMax === progressMin) {
      return numeric >= progressMax ? 1 : 0;
    }

    const mapped = (numeric - progressMin) / (progressMax - progressMin);
    return Math.min(1, Math.max(0, mapped));
  }

  /**
   * 根据当前进度采样 SVG 轨迹并更新徽章坐标。
   */
  private _syncEntitySubscription(): void {
    this._clearEntitySubscription();
    if (!this.flow || !this._config) return;

    const entities = [
      ...new Set(this._activeBadgesFromConfig(this._config).map((badge) => badge.entity)),
    ];
    this._entityUnsubs = entities.map((entity) =>
      this.flow!.subscribeEntity(entity, () => {
        this._progressRevision += 1;
      }),
    );
  }

  /**
   * 清理旧的实体订阅。
   */
  private _clearEntitySubscription(): void {
    this._entityUnsubs.forEach((unsub) => unsub());
    this._entityUnsubs = [];
  }

  /**
   * 生成和图片元素一致的徽章显示文本与单位。
   */
  private _displayForBadge(
    state: FlowEntityState | undefined,
    badge: LovelacePathBadgeConfig,
  ): { value: string; unit: string } {
    const attributeName = String(badge.attribute ?? "").trim();
    if (!attributeName || !state) {
      const formatted = formatPictureBadgeState(this.flow, badge.entity, badge.unit);
      return { value: formatted.value, unit: formatted.unit };
    }

    const rawValue =
      attributeName && attributeName in state.attributes
        ? state.attributes[attributeName]
        : state.state;
    const unit = String(
      badge.unit ?? state.attributes.unit_of_measurement ?? "",
    ).trim();
    if (typeof rawValue === "boolean") {
      return { value: rawValue ? "开" : "关", unit: "" };
    }
    if (rawValue === null || rawValue === undefined || rawValue === "") {
      return { value: "—", unit: "" };
    }
    const asNum = Number(rawValue);
    if (Number.isFinite(asNum)) {
      const value = Number.isInteger(asNum)
        ? String(asNum)
        : asNum.toFixed(1).replace(/\.0$/, "");
      return { value, unit };
    }
    return { value: String(rawValue), unit };
  }

  /**
   * 判断当前轨迹徽章是否为控制徽章。
   */
  private _isButtonBadge(badge: LovelacePathBadgeConfig): boolean {
    return String(badge.type ?? "state-badge") === "button";
  }

  /**
   * 生成状态徽章颜色，尽量和图片元素中的状态徽章行为保持一致。
   */
  private _stateBadgeColor(
    state: FlowEntityState | undefined,
    badge: LovelacePathBadgeConfig,
  ): string {
    const configured = badge.color ? String(badge.color).trim() : "";
    const active = stateActive(state);

    if (!active) {
      return "var(--state-inactive-color)";
    }

    if (!configured || configured === "none") {
      return "";
    }

    if (configured === "state") {
      return "var(--state-icon-color)";
    }

    return configured;
  }

  /**
   * 按徽章类型渲染轨迹元素上的内容。
   */
  private _renderBadgeContent(
    badge: LovelacePathBadgeConfig,
    state: FlowEntityState | undefined,
  ) {
    if (this._isButtonBadge(badge)) {
      return html`
        <hui-picture-action-element
          .flow=${this.flow}
          .config=${this._buttonConfig(badge)}
          .preview=${false}
          .statesRevision=${this.flow?.statesRevision ?? 0}
        ></hui-picture-action-element>
      `;
    }

    const display = this._displayForBadge(state, badge);
    const icon = String(badge.icon ?? state?.attributes?.icon ?? DEFAULT_ICON);

    return html`
      <ha-state-badge
        .value=${display.value}
        .unit=${display.unit}
        .icon=${icon}
        .color=${this._stateBadgeColor(state, badge)}
        .showIcon=${badgeShowIcon(badge)}
        .showValue=${badgeShowText(badge)}
      ></ha-state-badge>
    `;
  }

  /**
   * 转成控制徽章组件可直接复用的配置结构。
   */
  private _buttonConfig(badge: LovelacePathBadgeConfig): LovelacePictureElementConfig {
    return {
      type: "button",
      entity: badge.entity,
      name: badge.name,
      text: badge.text ?? badge.name,
      icon: badge.icon ?? DEFAULT_ICON,
      color: badge.color,
      show_name: badgeShowText(badge),
      value: badge.value,
    };
  }

  static styles = css`
    :host {
      display: block;
      height: 100%;
    }

    ha-card {
      height: 100%;
      overflow: hidden;
    }

    .card-header {
      margin: 0;
      padding: 12px 16px 0;
      font-size: 16px;
      font-weight: 600;
    }

    .root {
      position: relative;
      overflow: hidden;
      background: var(--secondary-background-color, #f4f6f8);
    }

    .background {
      position: relative;
      display: block;
      width: 100%;
      height: auto;
      user-select: none;
      pointer-events: none;
    }

    .overlay {
      position: absolute;
      inset: 0;
      width: 100%;
      height: 100%;
      z-index: 1;
    }

    .placeholder {
      min-height: 220px;
      height: 100%;
      display: flex;
      align-items: center;
      justify-content: center;
      color: var(--secondary-text-color);
      font-size: 13px;
      background: repeating-linear-gradient(
        45deg,
        var(--secondary-background-color, #f3f4f6),
        var(--secondary-background-color, #f3f4f6) 12px,
        transparent 12px,
        transparent 24px
      );
    }

    .badge {
      position: absolute;
      z-index: 2;
      transform: translate(-50%, -50%) scale(var(--badge-scale, 1));
      pointer-events: none;
      transform-origin: center center;
    }

    .badge.interactive {
      pointer-events: auto;
    }

    .caption {
      position: absolute;
      right: 12px;
      bottom: 12px;
      z-index: 2;
      padding: 8px 10px;
      border-radius: 10px;
      background: color-mix(in srgb, var(--card-background-color, #fff) 84%, transparent);
      box-shadow: 0 2px 8px rgba(0, 0, 0, 0.12);
      backdrop-filter: blur(4px);
      color: var(--primary-text-color);
      font-size: 12px;
      line-height: 1.4;
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "hui-path-badge-card": HuiPathBadgeCard;
  }
}
