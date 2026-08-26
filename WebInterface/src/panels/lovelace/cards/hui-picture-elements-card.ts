import { css, html, LitElement, nothing } from "lit";
import { customElement, property, query, state } from "lit/decorators.js";
import { classMap } from "lit/directives/class-map.js";
import { styleMap } from "lit/directives/style-map.js";
import { fireEvent } from "../../../common/dom/fire_event";
import type {
  Flow,
  LovelaceCard,
  LovelaceCardConfig,
  LovelaceGridOptions,
  LovelacePictureElementConfig,
} from "../../../types";
import "../../../components/ha-card";
import { DEFAULT_FLOORPLAN } from "../picture-elements/default-floorplan";
import { clampPercent, elementPosition, withPosition } from "../picture-elements/position";
import "../picture-elements/hui-picture-state-element";
import "../picture-elements/hui-picture-action-element";

@customElement("hui-picture-elements-card")
export class HuiPictureElementsCard extends LitElement implements LovelaceCard {
  public static getStubConfig(): LovelaceCardConfig {
    return {
      type: "picture-elements",
      image: DEFAULT_FLOORPLAN,
      elements: [
        {
          type: "state-badge",
          entity: "/demo/temperature",
          name: "温度",
          icon: "mdi:thermometer",
          color: "red",
          style: { top: "32%", left: "24%" },
        },
        {
          type: "button",
          entity: "/demo/trigger",
          icon: "mdi:gesture-tap-button",
          text: "触发",
          name: "触发",
          value: true,
          style: { top: "58%", left: "72%" },
        },
      ],
      grid_options: { columns: 12, rows: "auto", min_columns: 6, min_rows: 3 },
    };
  }

  @property({ attribute: false }) public flow?: Flow;

  @property({ type: Boolean }) public preview = false;

  @state() private _config?: LovelaceCardConfig;

  @state() private _elements: LovelacePictureElementConfig[] = [];

  @query(".root") private _root?: HTMLElement;

  private _dragIndex: number | null = null;

  private _moved = false;

  setConfig(config: LovelaceCardConfig): void {
    this._config = config;
    if (this._dragIndex === null) {
      this._elements = [...((config.elements ?? []) as LovelacePictureElementConfig[])];
    }
  }

  getGridOptions(): LovelaceGridOptions {
    return { columns: 12, rows: "auto", min_columns: 6, min_rows: 3 };
  }

  private _emitConfig(): void {
    if (!this._config) return;
    fireEvent(this, "picture-elements-changed", {
      config: { ...this._config, elements: this._elements.map((el) => ({ ...el })) },
    });
  }

  private _pointPercent(ev: PointerEvent): { left: number; top: number } | undefined {
    const root = this._root;
    if (!root) return undefined;
    const rect = root.getBoundingClientRect();
    if (rect.width <= 0 || rect.height <= 0) return undefined;
    return {
      left: clampPercent(((ev.clientX - rect.left) / rect.width) * 100),
      top: clampPercent(((ev.clientY - rect.top) / rect.height) * 100),
    };
  }

  private _onElementPointerDown(index: number, ev: PointerEvent): void {
    if (!this.preview || ev.button !== 0) return;
    ev.preventDefault();
    ev.stopPropagation();
    this._dragIndex = index;
    this._moved = false;
    (ev.currentTarget as HTMLElement).setPointerCapture(ev.pointerId);
  }

  private _onElementPointerMove(ev: PointerEvent): void {
    if (this._dragIndex === null) return;
    const point = this._pointPercent(ev);
    if (!point) return;
    this._moved = true;
    this._elements = this._elements.map((el, i) =>
      i === this._dragIndex ? withPosition(el, point.top, point.left) : el,
    );
  }

  private _onElementPointerUp(ev: PointerEvent): void {
    if (this._dragIndex === null) return;
    ev.stopPropagation();
    this._dragIndex = null;
    if (this._moved) this._emitConfig();
  }

  private _onRootPointerUp(ev: PointerEvent): void {
    if (!this.preview || ev.button !== 0 || this._dragIndex !== null || this._moved) return;
    const target = ev.target as HTMLElement | null;
    if (target?.closest(".element")) return;
    const point = this._pointPercent(ev);
    if (!point) return;
    fireEvent(this, "picture-element-position", point);
  }

  private _renderElement(element: LovelacePictureElementConfig, index: number) {
    const pos = elementPosition(element);
    const type = String(element.type ?? "state-badge");
    const inner =
      type === "button"
        ? html`
            <hui-picture-action-element
              .flow=${this.flow}
              .config=${element}
              .preview=${this.preview}
              .statesRevision=${this.flow?.statesRevision ?? 0}
            ></hui-picture-action-element>
          `
        : html`
            <hui-picture-state-element
              .flow=${this.flow}
              .config=${element}
              .statesRevision=${this.flow?.statesRevision ?? 0}
            ></hui-picture-state-element>
          `;

    return html`
      <div
        class=${classMap({ element: true, preview: this.preview })}
        style=${styleMap({ top: `${pos.top}%`, left: `${pos.left}%` })}
        @pointerdown=${(ev: PointerEvent) => this._onElementPointerDown(index, ev)}
        @pointermove=${this._onElementPointerMove}
        @pointerup=${this._onElementPointerUp}
        @pointercancel=${this._onElementPointerUp}
      >
        ${inner}
      </div>
    `;
  }

  protected render() {
    if (!this._config) return nothing;
    void this.flow?.statesRevision;

    const title = this._config.title ? String(this._config.title) : "";
    const image = this._config.image ? String(this._config.image) : "";

    return html`
      <ha-card>
        ${title ? html`<h1 class="card-header">${title}</h1>` : nothing}
        <div class="root" @pointerup=${this._onRootPointerUp}>
          ${image
            ? html`<img alt="" src=${image} draggable="false" />`
            : html`<div class="placeholder">添加底图后可在图上放置徽章</div>`}
          ${this._elements.map((element, index) => this._renderElement(element, index))}
        </div>
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
    }
    img {
      display: block;
      width: 100%;
      height: auto;
      user-select: none;
      pointer-events: none;
    }
    .placeholder {
      min-height: 180px;
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
    .element {
      position: absolute;
      transform: translate(-50%, -50%);
      z-index: 1;
      touch-action: none;
    }
    .element.preview {
      cursor: grab;
    }
    .element.preview:active {
      cursor: grabbing;
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "hui-picture-elements-card": HuiPictureElementsCard;
  }
}
