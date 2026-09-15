import { css, html, LitElement, nothing } from "lit";
import { customElement, property, state } from "lit/decorators.js";
import type { LovelacePathPoint } from "../../../types";
import { DEFAULT_FLOORPLAN } from "../picture-elements/default-floorplan";

interface PointsChangedDetail {
  points: LovelacePathPoint[];
  path: string;
}

const SVG_NS = "http://www.w3.org/2000/svg";
const DEFAULT_POINTS: LovelacePathPoint[] = [
  { x: 10, y: 88 },
  { x: 28, y: 18 },
  { x: 72, y: 18 },
  { x: 90, y: 88 },
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

function toPath(points: LovelacePathPoint[]): string {
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

function samplePath(pathData: string, sampleCount = 12): LovelacePathPoint[] {
  if (!pathData.trim()) return [];
  try {
    const svg = document.createElementNS(SVG_NS, "svg");
    const path = document.createElementNS(SVG_NS, "path");
    path.setAttribute("d", pathData);
    svg.append(path);
    const totalLength = path.getTotalLength();
    if (!Number.isFinite(totalLength) || totalLength <= 0) return [];

    const count = Math.max(2, sampleCount);
    const points: LovelacePathPoint[] = [];
    for (let index = 0; index < count; index += 1) {
      const progress = index / (count - 1);
      const point = path.getPointAtLength(totalLength * progress);
      points.push(normalizePoint({ x: point.x, y: point.y }));
    }
    return points;
  } catch {
    return [];
  }
}

@customElement("flow-path-points-editor")
export class FlowPathPointsEditor extends LitElement {
  @property() public label = "轨迹";

  @property() public helper = "";

  @property() public image = "";

  @property({ attribute: false }) public points: LovelacePathPoint[] = [];

  @property() public path = "";

  @state() private _draftPoints: LovelacePathPoint[] = [];

  @state() private _selectedIndex = -1;

  private _dragIndex: number | null = null;

  protected willUpdate(changed: import("lit").PropertyValues): void {
    if (!changed.has("points") && !changed.has("path")) {
      return;
    }
    if (this._dragIndex !== null) {
      return;
    }

    const incomingPoints = Array.isArray(this.points) ? this.points : [];
    const normalizedPoints = incomingPoints.map(normalizePoint);
    const fallbackPoints =
      normalizedPoints.length > 0
        ? normalizedPoints
        : this.path
          ? samplePath(this.path)
          : [];
    const nextPoints = fallbackPoints.length > 0 ? fallbackPoints : [];
    this._draftPoints = nextPoints;
    if (this._selectedIndex >= nextPoints.length) {
      this._selectedIndex = nextPoints.length - 1;
    }
  }

  private _editorPoint(event: PointerEvent): LovelacePathPoint | undefined {
    const editor = this.renderRoot.querySelector(".canvas") as HTMLElement | null;
    if (!editor) return undefined;
    const rect = editor.getBoundingClientRect();
    if (rect.width <= 0 || rect.height <= 0) return undefined;
    return normalizePoint({
      x: ((event.clientX - rect.left) / rect.width) * 100,
      y: ((event.clientY - rect.top) / rect.height) * 100,
    });
  }

  private _emit(points: LovelacePathPoint[]): void {
    const normalized = points.map(normalizePoint);
    this._draftPoints = normalized;
    this.dispatchEvent(
      new CustomEvent<PointsChangedDetail>("points-changed", {
        bubbles: true,
        composed: true,
        detail: {
          points: normalized,
          path: toPath(normalized),
        },
      }),
    );
  }

  private _onCanvasPointerDown(event: PointerEvent): void {
    if (event.button !== 0) return;
    const target = event
      .composedPath()
      .find(
        (node) =>
          node instanceof HTMLElement &&
          node.dataset &&
          node.dataset.pointIndex !== undefined,
      ) as HTMLElement | undefined;
    if (target) return;

    const point = this._editorPoint(event);
    if (!point) return;
    this._selectedIndex = this._draftPoints.length;
    this._emit([...this._draftPoints, point]);
  }

  private _onPointPointerDown(index: number, event: PointerEvent): void {
    if (event.button !== 0) return;
    event.preventDefault();
    event.stopPropagation();
    this._dragIndex = index;
    this._selectedIndex = index;
    (event.currentTarget as HTMLElement).setPointerCapture(event.pointerId);
  }

  private _onPointPointerMove(event: PointerEvent): void {
    if (this._dragIndex === null) return;
    const point = this._editorPoint(event);
    if (!point) return;
    const points = this._draftPoints.map((item, index) =>
      index === this._dragIndex ? point : item,
    );
    this._emit(points);
  }

  private _onPointPointerUp(): void {
    this._dragIndex = null;
  }

  private _removeSelected(): void {
    if (this._selectedIndex < 0) return;
    const points = this._draftPoints.filter((_, index) => index !== this._selectedIndex);
    this._selectedIndex = Math.min(this._selectedIndex, points.length - 1);
    this._emit(points);
  }

  private _clear(): void {
    this._selectedIndex = -1;
    this._emit([]);
  }

  private _resetDefault(): void {
    this._selectedIndex = DEFAULT_POINTS.length - 1;
    this._emit(DEFAULT_POINTS);
  }

  protected render() {
    const points = this._draftPoints;
    const path = toPath(points);
    const image = this.image || DEFAULT_FLOORPLAN;

    return html`
      <div class="field">
        <div class="label-row">
          <span class="label">${this.label}</span>
          <div class="actions">
            <button type="button" class="link-btn" @click=${this._resetDefault}>默认轨迹</button>
            <button
              type="button"
              class="link-btn"
              ?disabled=${this._selectedIndex < 0}
              @click=${this._removeSelected}
            >
              删除点
            </button>
            <button
              type="button"
              class="link-btn danger"
              ?disabled=${points.length === 0}
              @click=${this._clear}
            >
              清空
            </button>
          </div>
        </div>
        <div class="canvas" @pointerdown=${this._onCanvasPointerDown}>
          <img class="background" alt="" src=${image} draggable="false" />
          <svg class="overlay" viewBox="0 0 100 100" preserveAspectRatio="none" aria-hidden="true">
            ${path
              ? html`
                  <path
                    class="path"
                    d=${path}
                    fill="none"
                    stroke="var(--primary-color)"
                    stroke-width="2.8"
                    stroke-linecap="round"
                    stroke-linejoin="round"
                  ></path>
                `
              : nothing}
            ${points.map(
              (point, index) => html`
                <circle
                  class=${index === this._selectedIndex ? "point selected" : "point"}
                  cx=${String(point.x)}
                  cy=${String(point.y)}
                  r="2.25"
                ></circle>
              `,
            )}
          </svg>
          ${points.map(
            (point, index) => html`
              <button
                type="button"
                class=${index === this._selectedIndex ? "handle selected" : "handle"}
                style=${`left:${point.x}%;top:${point.y}%;`}
                data-point-index=${String(index)}
                @click=${() => {
                  this._selectedIndex = index;
                }}
                @pointerdown=${(event: PointerEvent) => this._onPointPointerDown(index, event)}
                @pointermove=${this._onPointPointerMove}
                @pointerup=${this._onPointPointerUp}
                @pointercancel=${this._onPointPointerUp}
                title=${`点 ${index + 1}`}
              >
                ${index + 1}
              </button>
            `,
          )}
        </div>
        <p class="helper">
          ${this.helper || "点击底图新增轨迹点，拖动点调整位置；卡片会按这些点自动生成轨迹。"}
        </p>
        <p class="meta">
          ${points.length > 0
            ? `当前 ${points.length} 个点，轨迹数据：${path}`
            : "还没有轨迹点，先在底图上点击开始绘制。"}
        </p>
      </div>
    `;
  }

  static styles = css`
    .field {
      display: flex;
      flex-direction: column;
      gap: 8px;
      margin-bottom: 12px;
    }

    .label-row {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 12px;
    }

    .label {
      font-size: 13px;
      color: var(--secondary-text-color);
    }

    .actions {
      display: flex;
      gap: 12px;
      flex-wrap: wrap;
      justify-content: flex-end;
    }

    .link-btn {
      border: none;
      background: transparent;
      color: var(--primary-color);
      font-size: 13px;
      font-weight: 600;
      cursor: pointer;
      padding: 0;
    }

    .link-btn.danger {
      color: var(--error-color, #d32f2f);
    }

    .link-btn:disabled {
      cursor: default;
      opacity: 0.45;
    }

    .canvas {
      position: relative;
      width: 100%;
      min-height: 240px;
      border-radius: 12px;
      overflow: hidden;
      border: 1px solid var(--divider-color);
      background: var(--secondary-background-color, #f4f6f8);
      touch-action: none;
      cursor: crosshair;
    }

    .background,
    .overlay {
      position: absolute;
      inset: 0;
      width: 100%;
      height: 100%;
    }

    .background {
      object-fit: cover;
      user-select: none;
      pointer-events: none;
    }

    .overlay {
      pointer-events: none;
    }

    .path {
      filter: drop-shadow(0 1px 2px rgba(0, 0, 0, 0.12));
    }

    .point {
      fill: white;
      stroke: var(--primary-color);
      stroke-width: 0.8;
    }

    .point.selected {
      fill: var(--primary-color);
      stroke: white;
    }

    .handle {
      position: absolute;
      transform: translate(-50%, -50%);
      width: 22px;
      height: 22px;
      border-radius: 999px;
      border: 2px solid var(--primary-color);
      background: white;
      color: var(--primary-color);
      font-size: 11px;
      font-weight: 700;
      display: flex;
      align-items: center;
      justify-content: center;
      cursor: grab;
      touch-action: none;
      box-shadow: 0 1px 4px rgba(0, 0, 0, 0.18);
    }

    .handle.selected {
      background: var(--primary-color);
      color: white;
    }

    .handle:active {
      cursor: grabbing;
    }

    .helper,
    .meta {
      margin: 0;
      font-size: 12px;
      color: var(--secondary-text-color);
      line-height: 1.45;
      word-break: break-word;
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "flow-path-points-editor": FlowPathPointsEditor;
  }
}
