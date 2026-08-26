import type { PropertyValues } from "lit";
import { html, LitElement, nothing } from "lit";
import { customElement, property } from "lit/decorators.js";
import Sortable from "sortablejs";
import { fireEvent } from "../common/dom/fire_event";

export type HaSortableOptions = Omit<
  Sortable.Options,
  "onStart" | "onChoose" | "onEnd" | "onUpdate" | "onAdd" | "onRemove"
>;

declare global {
  interface HASSDomEvents {
    "item-moved": { oldIndex: number; newIndex: number };
    "item-added": { index: number; data: unknown; item: HTMLElement };
    "item-removed": { index: number };
    "drag-start": undefined;
    "drag-end": undefined;
  }
}

@customElement("ha-sortable")
export class HaSortable extends LitElement {
  private _sortable?: Sortable;

  @property({ type: Boolean }) public disabled = false;

  @property({ type: Boolean, attribute: "no-style" }) public noStyle = false;

  @property({ type: String, attribute: "draggable-selector" })
  public draggableSelector?: string;

  @property({ type: String, attribute: "handle-selector" })
  public handleSelector?: string;

  @property({ type: String }) public filter?: string;

  @property({ attribute: false }) public group?: string | Sortable.GroupOptions;

  @property({ type: Boolean, attribute: "invert-swap" })
  public invertSwap = false;

  @property({ attribute: false }) public options?: HaSortableOptions;

  @property({ type: Boolean }) public rollback = true;

  private _shouldDestroy = false;

  protected updated(changed: PropertyValues): void {
    if (changed.has("disabled")) {
      void this._ensureSortable();
    }
  }

  disconnectedCallback(): void {
    super.disconnectedCallback();
    this._shouldDestroy = true;
    setTimeout(() => {
      if (this._shouldDestroy) {
        this._destroySortable();
        this._shouldDestroy = false;
      }
    }, 1);
  }

  connectedCallback(): void {
    super.connectedCallback();
    this._shouldDestroy = false;
    if (this.hasUpdated && !this._sortable && !this.disabled) {
      void this._ensureSortable();
    }
  }

  protected createRenderRoot() {
    return this;
  }

  protected render() {
    if (this.noStyle) return nothing;
    return html`
      <style>
        .sortable-fallback {
          display: none !important;
        }
        .sortable-ghost {
          box-shadow: 0 0 0 2px var(--primary-color);
          background: rgba(3, 169, 244, 0.2);
          border-radius: var(--ha-card-border-radius, 12px);
          opacity: 0.45;
        }
        .sortable-drag {
          border-radius: var(--ha-card-border-radius, 12px);
          opacity: 1;
          background: var(--card-background-color);
          box-shadow: 0 4px 12px rgba(0, 0, 0, 0.18);
          cursor: grabbing;
        }
      </style>
    `;
  }

  protected firstUpdated(): void {
    if (!this.disabled) {
      void this._ensureSortable();
    }
  }

  private async _ensureSortable(): Promise<void> {
    if (this._sortable) {
      this._sortable.option("disabled", this.disabled);
      return;
    }
    if (this.disabled) return;
    await this._createSortable();
  }

  private async _createSortable(): Promise<void> {
    if (this._sortable) return;
    const container = this.children[0] as HTMLElement | undefined;
    if (!container) return;

    const options: Sortable.Options = {
      scroll: true,
      forceAutoScrollFallback: true,
      scrollSpeed: 20,
      animation: 150,
      disabled: this.disabled,
      ...this.options,
      onChoose: this._handleChoose,
      onStart: this._handleStart,
      onEnd: this._handleEnd,
      onUpdate: this._handleUpdate,
      onAdd: this._handleAdd,
      onRemove: this._handleRemove,
    };

    if (this.draggableSelector) options.draggable = this.draggableSelector;
    if (this.handleSelector) options.handle = this.handleSelector;
    if (this.invertSwap !== undefined) options.invertSwap = this.invertSwap;
    if (this.group) options.group = this.group;
    if (this.filter) options.filter = this.filter;

    this._sortable = Sortable.create(container, options);
  }

  private _handleUpdate = (evt: Sortable.SortableEvent): void => {
    fireEvent(this, "item-moved", {
      newIndex: evt.newIndex!,
      oldIndex: evt.oldIndex!,
    });
  };

  private _handleAdd = (evt: Sortable.SortableEvent): void => {
    fireEvent(this, "item-added", {
      index: evt.newIndex!,
      data: (evt.item as HTMLElement & { sortableData?: unknown }).sortableData,
      item: evt.item,
    });
  };

  private _handleRemove = (evt: Sortable.SortableEvent): void => {
    fireEvent(this, "item-removed", { index: evt.oldIndex! });
  };

  private _handleEnd = (evt: Sortable.SortableEvent): void => {
    fireEvent(this, "drag-end");
    if (this.rollback && (evt.item as HTMLElement & { placeholder?: Comment }).placeholder) {
      const item = evt.item as HTMLElement & { placeholder?: Comment };
      item.placeholder!.replaceWith(item);
      delete item.placeholder;
    }
  };

  private _handleStart = (): void => {
    fireEvent(this, "drag-start");
  };

  private _handleChoose = (evt: Sortable.SortableEvent): void => {
    if (!this.rollback) return;
    const item = evt.item as HTMLElement & { placeholder?: Comment };
    item.placeholder = document.createComment("sort-placeholder");
    item.after(item.placeholder);
  };

  private _destroySortable(): void {
    this._sortable?.destroy();
    this._sortable = undefined;
  }
}

declare global {
  interface HTMLElementTagNameMap {
    "ha-sortable": HaSortable;
  }
}
