import { css, html, LitElement, nothing } from "lit";
import { customElement, property, state } from "lit/decorators.js";
import { repeat } from "lit/directives/repeat.js";
import { consume } from "@lit/context";
import type { Flow, LovelaceCardConfig, LovelaceSectionConfig, LovelaceViewConfig } from "../../types";
import { flowContext } from "../../context/flow-context";
import { resolveSectionBackground } from "../../data/section-config";
import { moveCardInView, parseCardPath } from "../../data/layout-mutations";
import "../../components/ha-sortable";
import "./hui-card-container";
import "./components/flow-section-background";

const CARD_SORTABLE_OPTIONS = {
  delay: 100,
  delayOnTouchOnly: true,
  direction: "vertical" as const,
  invertedSwapThreshold: 0.7,
  group: "card",
};

@customElement("flow-section")
export class FlowSection extends LitElement {
  @consume({ context: flowContext, subscribe: true })
  @property({ attribute: false })
  public flow?: Flow;

  @property({ attribute: false }) public config?: LovelaceSectionConfig;

  @property({ attribute: false }) public view?: LovelaceViewConfig;

  @property({ type: Number }) public viewIndex = 0;

  @property({ type: Number }) public sectionIndex = 0;

  @property({ type: Boolean }) public editMode = false;

  @property({ type: Boolean }) public preview = false;

  @property({ type: Number }) public layoutRevision = 0;

  @state() private _dragging = false;

  private _cardConfigKeys = new WeakMap<LovelaceCardConfig, string>();

  private _getCardKey(cardConfig: LovelaceCardConfig): string {
    if (!this._cardConfigKeys.has(cardConfig)) {
      this._cardConfigKeys.set(cardConfig, Math.random().toString(36));
    }
    return this._cardConfigKeys.get(cardConfig)!;
  }

  private _addCard(): void {
    this.dispatchEvent(
      new CustomEvent("add-card", {
        bubbles: true,
        composed: true,
        detail: { section: this.config },
      }),
    );
  }

  private _notifyLayoutChange(): void {
    this.dispatchEvent(
      new CustomEvent("layout-will-change", { bubbles: true, composed: true }),
    );
    this.dispatchEvent(
      new CustomEvent("layout-changed", { bubbles: true, composed: true }),
    );
    this.requestUpdate();
  }

  private _cardMoved(ev: CustomEvent<{ oldIndex: number; newIndex: number }>): void {
    ev.stopPropagation();
    if (!this.view) return;
    const { oldIndex, newIndex } = ev.detail;
    if (oldIndex === newIndex) return;
    moveCardInView(this.view, this.sectionIndex, oldIndex, this.sectionIndex, newIndex);
    this._notifyLayoutChange();
  }

  private _cardAdded(ev: CustomEvent<{ index: number; data: unknown }>): void {
    ev.stopPropagation();
    if (!this.view) return;

    const from = parseCardPath(ev.detail.data);
    if (!from) return;
    if (from.sectionIndex === this.sectionIndex && from.viewIndex === this.viewIndex) return;

    moveCardInView(
      this.view,
      from.sectionIndex,
      from.cardIndex,
      this.sectionIndex,
      ev.detail.index,
    );
    this._notifyLayoutChange();
  }

  private _cardRemoved(ev: CustomEvent<{ index: number }>): void {
    ev.stopPropagation();
    // Cross-section moves are applied in the target section's item-added handler.
    void ev.detail.index;
  }

  private _dragStart(): void {
    this._dragging = true;
  }

  private _dragEnd(): void {
    this._dragging = false;
  }

  protected render() {
    if (!this.config) return nothing;

    const cards = this.config.cards;
    const hasBackground = resolveSectionBackground(this.config.background) !== undefined;

    const grid = html`
      <div class="card-grid ${this.editMode ? "edit-mode" : ""} ${this._dragging ? "dragging" : ""}">
        ${repeat(
          cards,
          (cardConfig, idx) => `${this.layoutRevision}:${idx}:${this._getCardKey(cardConfig)}`,
          (cardConfig, idx) => html`
            <hui-card-container
              .sortableData=${[this.viewIndex, this.sectionIndex, idx]}
              .config=${cardConfig}
              .sectionIndex=${this.sectionIndex}
              .cardIndex=${idx}
              .editMode=${this.editMode}
              .preview=${this.preview}
            ></hui-card-container>
          `,
        )}
        ${this.editMode
          ? html`
              <button type="button" class="add-card" @click=${this._addCard}>
                <span class="add-plus">+</span>
              </button>
            `
          : nothing}
      </div>
    `;

    return html`
      <div class="section-root ${this.editMode ? "edit-mode" : ""}">
        <div
          class="section-container ${hasBackground ? "has-background" : ""}"
        >
          ${hasBackground
            ? html`
                <flow-section-background
                  .background=${this.config.background}
                ></flow-section-background>
              `
            : nothing}
          <div class="section-wrapper">
            <ha-sortable
              no-style
              invert-swap
              .disabled=${!this.editMode}
              .rollback=${false}
              draggable-selector="hui-card-container"
              filter=".add-card,button,.trigger,.menu,.chart-interactive,canvas"
              .options=${CARD_SORTABLE_OPTIONS}
              @item-moved=${this._cardMoved}
              @item-added=${this._cardAdded}
              @item-removed=${this._cardRemoved}
              @drag-start=${this._dragStart}
              @drag-end=${this._dragEnd}
            >
              ${grid}
            </ha-sortable>
          </div>
        </div>
      </div>
    `;
  }

  static styles = css`
    :host {
      --base-column-count: 12;
      --row-gap: var(--ha-section-grid-row-gap, 8px);
      --column-gap: var(--ha-section-grid-column-gap, 8px);
      --row-height: var(--ha-section-grid-row-height, 56px);
      display: block;
    }

    .section-root {
      display: flex;
      flex-direction: column;
    }

    .section-container {
      position: relative;
      border-radius: var(--ha-section-border-radius, 16px);
    }

    .section-container.has-background {
      padding: var(--ha-space-2, 8px);
    }

    .section-wrapper {
      position: relative;
      z-index: 1;
      border-radius: inherit;
    }

    .card-grid {
      --grid-column-count: calc(var(--base-column-count) * var(--column-span, 1));
      display: grid;
      grid-template-columns: repeat(var(--grid-column-count), minmax(0, 1fr));
      grid-auto-rows: auto;
      grid-auto-flow: row dense;
      row-gap: var(--row-gap);
      column-gap: var(--column-gap);
      align-items: stretch;
    }

    .card-grid.dragging hui-card-container {
      cursor: grabbing;
    }

    .card-grid.edit-mode hui-card-container:not([auto-height]) {
      min-height: calc((var(--row-height) - var(--row-gap)) / 2);
    }

    .add-card {
      grid-column: span 3;
      grid-row: span 1;
      height: var(--row-height);
      border: 2px dashed var(--primary-color);
      border-radius: var(--ha-card-border-radius, 12px);
      background: transparent;
      color: var(--primary-color);
      cursor: pointer;
      display: flex;
      align-items: center;
      justify-content: center;
      padding: 0;
      order: 1;
    }

    .add-card:hover {
      background: rgba(3, 169, 244, 0.04);
    }

    .add-plus {
      font-size: 28px;
      line-height: 1;
      font-weight: 300;
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "flow-section": FlowSection;
  }
}
