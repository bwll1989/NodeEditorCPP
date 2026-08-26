import { css, html, LitElement, nothing } from "lit";
import { customElement, property, state } from "lit/decorators.js";
import { repeat } from "lit/directives/repeat.js";
import { consume } from "@lit/context";
import type { Flow, LovelaceSectionConfig, LovelaceViewConfig } from "../../types";
import { flowContext } from "../../context/flow-context";
import { moveSectionInView } from "../../data/layout-mutations";
import {
  clampSectionColumnSpan,
  computeResponsiveMaxColumns,
  computeViewGridColumnCount,
  DEFAULT_MAX_COLUMNS,
  parseCssPx,
  sumSectionColumnSpans,
} from "./common/sections-layout";
import "../../components/ha-sortable";
import "./flow-section";
import "./components/hui-section-edit-mode";

const SECTION_SORTABLE_OPTIONS = {
  delay: 100,
  delayOnTouchOnly: true,
  animation: 150,
};

@customElement("flow-view")
export class FlowView extends LitElement {
  @consume({ context: flowContext, subscribe: true })
  @property({ attribute: false })
  public flow?: Flow;

  @property({ attribute: false }) public config?: LovelaceViewConfig;

  @property({ type: Number }) public viewIndex = 0;

  @property({ type: Boolean }) public editMode = false;

  @property({ type: Number }) public layoutRevision = 0;

  /** Viewport-based cap (1–max_columns), HA `_maxColumns` */
  @state() private _maxColumns = 1;

  @state() private _dragging = false;

  private _resizeObserver?: ResizeObserver;

  private _sectionConfigKeys = new WeakMap<LovelaceSectionConfig, string>();

  private _getSectionKey(section: LovelaceSectionConfig): string {
    if (!this._sectionConfigKeys.has(section)) {
      this._sectionConfigKeys.set(section, Math.random().toString(36));
    }
    return this._sectionConfigKeys.get(section)!;
  }

  connectedCallback(): void {
    super.connectedCallback();
    this._resizeObserver = new ResizeObserver((entries) => {
      const width = entries[0]?.contentRect.width ?? 0;
      if (!width) return;
      this._updateMaxColumns(width);
    });
    this._resizeObserver.observe(this);
  }

  disconnectedCallback(): void {
    this._resizeObserver?.disconnect();
    super.disconnectedCallback();
  }

  private _updateMaxColumns(totalWidth: number): void {
    const hostStyle = getComputedStyle(this);
    const minColumnWidth =
      parseCssPx(hostStyle.getPropertyValue("--column-min-width")) || 320;
    const columnGap = parseCssPx(hostStyle.getPropertyValue("--column-gap")) || 32;
    const wrapper = this.renderRoot.querySelector(".wrapper");
    const wrapperStyle = wrapper ? getComputedStyle(wrapper) : null;
    const horizontalPadding = wrapperStyle
      ? parseCssPx(wrapperStyle.paddingLeft) + parseCssPx(wrapperStyle.paddingRight)
      : columnGap * 2;

    const configMax = this.config?.max_columns ?? DEFAULT_MAX_COLUMNS;
    const maxColumns = computeResponsiveMaxColumns(totalWidth, {
      minColumnWidth,
      columnGap,
      horizontalPadding,
      configMaxColumns: configMax,
    });

    if (maxColumns !== this._maxColumns) {
      this._maxColumns = maxColumns;
    }
  }

  private _addSection(): void {
    this.dispatchEvent(new CustomEvent("add-section", { bubbles: true, composed: true }));
  }

  private _sectionMoved(ev: CustomEvent<{ oldIndex: number; newIndex: number }>): void {
    ev.stopPropagation();
    if (!this.config) return;
    const { oldIndex, newIndex } = ev.detail;
    if (oldIndex === newIndex) return;
    this.dispatchEvent(
      new CustomEvent("layout-will-change", { bubbles: true, composed: true }),
    );
    moveSectionInView(this.config, oldIndex, newIndex);
    this.dispatchEvent(
      new CustomEvent("layout-changed", { bubbles: true, composed: true }),
    );
    this.requestUpdate();
  }

  private _dragStart(): void {
    this._dragging = true;
  }

  private _dragEnd(): void {
    this._dragging = false;
  }

  protected render() {
    if (!this.config) return nothing;

    const sections = this.config.sections;
    const sectionColumnSpanSum = sumSectionColumnSpans(sections);
    const contentColumnCount = computeViewGridColumnCount({
      sectionColumnSpanSum,
      maxColumns: this._maxColumns,
      editMode: this.editMode,
    });
    const dense = Boolean(this.config.dense_section_placement);

    const container = html`
      <div class="container ${this._dragging ? "dragging" : ""} ${dense ? "dense" : ""}">
        ${repeat(
          sections,
          (section) => this._getSectionKey(section),
          (section, index) => {
            const columnSpan = clampSectionColumnSpan(
              section.column_span,
              contentColumnCount,
            );
            const rowSpan = section.row_span ?? 1;
            return html`
            <div
              class="section-slot ${this.editMode ? "edit-mode" : ""}"
              style=${`--column-span: ${columnSpan}; --row-span: ${rowSpan};`}
            >
              <hui-section-edit-mode .sectionIndex=${index} .inactive=${!this.editMode}>
                <flow-section
                  style=${`--column-span: ${columnSpan};`}
                  .view=${this.config}
                  .viewIndex=${this.viewIndex}
                  .config=${section}
                  .sectionIndex=${index}
                  .editMode=${this.editMode}
                  .layoutRevision=${this.layoutRevision}
                ></flow-section>
              </hui-section-edit-mode>
            </div>
          `;
          },
        )}
        ${this.editMode
          ? html`
              <button type="button" class="create-section" @click=${this._addSection}>
                <span class="create-icon">▦+</span>
                <span>拖放卡片到此处以创建新分区</span>
              </button>
            `
          : nothing}
      </div>
    `;

    return html`
      <div
        class="wrapper"
        style=${`--column-count: ${this._maxColumns}; --content-column-count: ${contentColumnCount};`}
      >
        <ha-sortable
          no-style
          .disabled=${!this.editMode}
          draggable-selector=".section-slot"
          handle-selector=".section-handle"
          filter=".create-section,button,.trigger,.menu"
          .options=${SECTION_SORTABLE_OPTIONS}
          @item-moved=${this._sectionMoved}
          @drag-start=${this._dragStart}
          @drag-end=${this._dragEnd}
        >
          ${container}
        </ha-sortable>
      </div>
    `;
  }

  static styles = css`
    :host {
      --row-height: var(--ha-view-sections-row-height, 56px);
      --row-gap: var(--ha-view-sections-row-gap, 24px);
      --column-gap: var(--ha-view-sections-column-gap, 32px);
      --column-max-width: var(--ha-view-sections-column-max-width, 500px);
      --column-min-width: var(--ha-view-sections-column-min-width, 320px);
      display: block;
      flex: 1;
    }

    .wrapper {
      padding: 0 var(--column-gap);
      box-sizing: content-box;
      margin: var(--ha-view-sections-extra-top-margin, 24px) auto 0;
      max-width: calc(
        var(--column-count) * var(--column-max-width) +
          (var(--column-count) - 1) * var(--column-gap)
      );
    }

    .container {
      display: grid;
      grid-template-columns: repeat(var(--content-column-count), minmax(0, 1fr));
      grid-auto-flow: row;
      gap: var(--row-gap) var(--column-gap);
      padding: var(--row-gap) 0;
      align-items: start;
    }

    .container.dense {
      grid-auto-flow: row dense;
    }

    .section-slot {
      grid-column: span var(--column-span, 1);
      grid-row: span var(--row-span, 1);
      min-width: 0;
      display: flex;
      flex-direction: column;
      position: relative;
    }

    .container.dragging .section-slot {
      cursor: grabbing;
    }

    .create-section {
      grid-column: span 1;
      min-height: calc(var(--row-height) + 2 * var(--row-gap));
      border: 2px dashed var(--primary-color);
      border-radius: var(--ha-section-border-radius, 16px);
      background: transparent;
      color: var(--primary-text-color);
      cursor: pointer;
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      gap: 8px;
      padding: 16px;
      font-size: 14px;
      line-height: 1.4;
      text-align: center;
    }

    .create-section:hover {
      background: rgba(3, 169, 244, 0.04);
    }

    .create-icon {
      font-size: 28px;
      line-height: 1;
      color: var(--primary-color);
      font-weight: 300;
    }

    @media (max-width: 600px) {
      .container {
        grid-template-columns: 1fr;
      }
      .section-slot {
        grid-column: 1 / -1;
      }
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "flow-view": FlowView;
  }
}
