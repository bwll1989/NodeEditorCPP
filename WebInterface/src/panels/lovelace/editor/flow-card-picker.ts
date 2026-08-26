import { css, html, LitElement, nothing, type PropertyValues } from "lit";
import { customElement, property, state } from "lit/decorators.js";
import { mdiClose, mdiMagnify } from "@mdi/js";
import type { CardRegistryEntry, Flow, LovelaceCard, LovelaceCardConfig } from "../../../types";
import {
  DEFAULT_SUGGESTED_CARD_TYPES,
  getCardClipboard,
  getRecentCardTypes,
  onCardClipboardChanged,
  RECOMMENDED_CARD_LIMIT,
} from "../../../data/dashboard-card-clipboard";
import { ensureCardLoaded, getCardEntry, getPickerCards } from "../../../registry/card-registry";
import { createBackdropDismissHandlers } from "../../../common/dom/modal-backdrop";
import { pickerPreviewQueue } from "../../../common/dom/preview-load-queue";
import { createCardElement } from "../create-element/create-card-element";
import "../../../components/ha-icon";
import "../../../components/ha-icon-button";
import "../../../components/ha-expandable-panel";

const EAGER_PREVIEW_COUNT = 6;

function filterCards(cards: CardRegistryEntry[], query: string): CardRegistryEntry[] {
  const q = query.trim().toLowerCase();
  if (!q) return cards;
  return cards.filter((card) => {
    const haystack = [card.type, card.name, card.description ?? ""].join(" ").toLowerCase();
    return haystack.includes(q);
  });
}

@customElement("flow-card-picker-item")
export class FlowCardPickerItem extends LitElement {
  @property() public type = "";

  @property() public name = "";

  @property() public icon = "mdi:card-outline";

  /** When set, pick/preview this full config (clipboard paste). */
  @property({ attribute: false }) public config?: LovelaceCardConfig;

  @property({ type: Number }) public index = 0;

  @property({ attribute: false }) public flow?: Flow;

  @property({ attribute: false }) public scrollRoot?: HTMLElement | null;

  @state() private _previewState: "placeholder" | "loading" | "ready" | "error" = "placeholder";

  private _card?: LovelaceCard;

  private _observer?: IntersectionObserver;

  private _scrollListener?: () => void;

  private _loadRequested = false;

  private _disconnected = false;

  connectedCallback(): void {
    super.connectedCallback();
    this._disconnected = false;
    if (this.index < EAGER_PREVIEW_COUNT) {
      this._scheduleVisibilityCheck();
    }
    this._bindScrollRoot();
  }

  disconnectedCallback(): void {
    this._disconnected = true;
    this._teardownObserver();
    super.disconnectedCallback();
  }

  protected override updated(changed: PropertyValues): void {
    if (changed.has("scrollRoot")) {
      this._bindScrollRoot();
    }
    if (changed.has("config") || changed.has("type")) {
      this._loadRequested = false;
      this._card = undefined;
      this._previewState = "placeholder";
      this._scheduleVisibilityCheck();
    }
    if (changed.has("index") && this.index < EAGER_PREVIEW_COUNT) {
      this._scheduleVisibilityCheck();
    }
    if (this._previewState === "ready" && this._card) {
      requestAnimationFrame(() => this._mountPreview());
    }
  }

  private _teardownObserver(): void {
    this._observer?.disconnect();
    this._observer = undefined;
    if (this._scrollListener && this.scrollRoot) {
      this.scrollRoot.removeEventListener("scroll", this._scrollListener);
    }
    this._scrollListener = undefined;
  }

  private _bindScrollRoot(): void {
    this._teardownObserver();
    if (!this.scrollRoot) return;

    this._observer = new IntersectionObserver(
      (entries) => {
        if (entries.some((entry) => entry.isIntersecting)) {
          this._requestPreview();
        }
      },
      { root: this.scrollRoot, rootMargin: "100px 0px", threshold: 0.01 },
    );
    this._observer.observe(this);

    this._scrollListener = () => this._scheduleVisibilityCheck();
    this.scrollRoot.addEventListener("scroll", this._scrollListener, { passive: true });

    this._scheduleVisibilityCheck();
  }

  private _scheduleVisibilityCheck(): void {
    requestAnimationFrame(() => {
      if (this._disconnected || this._loadRequested) return;
      if (this.index < EAGER_PREVIEW_COUNT || this._isVisibleInScrollRoot()) {
        this._requestPreview();
      }
    });
  }

  private _isVisibleInScrollRoot(): boolean {
    if (!this.scrollRoot) return this.index < EAGER_PREVIEW_COUNT;
    const rect = this.getBoundingClientRect();
    if (rect.width <= 0 || rect.height <= 0) return false;
    const rootRect = this.scrollRoot.getBoundingClientRect();
    return rect.bottom > rootRect.top && rect.top < rootRect.bottom;
  }

  private _requestPreview(): void {
    if (this._loadRequested) return;
    this._loadRequested = true;
    this._teardownObserver();

    if (this._previewState === "ready") return;

    this._previewState = "loading";
    pickerPreviewQueue.enqueue(() => this._loadPreview());
  }

  private async _loadPreview(): Promise<void> {
    try {
      const type = this.config?.type || this.type;
      const ctor = await ensureCardLoaded(type);
      if (this._disconnected) return;
      if (!ctor) {
        this._previewState = "error";
        return;
      }
      const stub =
        this.config ??
        (ctor.getStubConfig?.() ?? ({ type } as LovelaceCardConfig));
      const el = await createCardElement(stub);
      if (this._disconnected) return;
      el.flow = this.flow;
      this._card = el;
      this._previewState = "ready";
    } catch {
      if (!this._disconnected) {
        this._previewState = "error";
      }
    }
  }

  private _mountPreview(): void {
    if (!this._card || this._previewState !== "ready") return;
    this._card.flow = this.flow;
    const host = this.renderRoot.querySelector(".preview-inner");
    if (host && !host.contains(this._card)) {
      host.replaceChildren(this._card);
    }
  }

  private _pick(): void {
    this.dispatchEvent(
      new CustomEvent("card-select", {
        detail: {
          type: this.config?.type || this.type,
          config: this.config ? structuredClone(this.config) : undefined,
        },
        bubbles: true,
        composed: true,
      }),
    );
  }

  protected render() {
    return html`
      <button type="button" class="card" @click=${this._pick}>
        <div class="card-header">${this.name}</div>
        <div class="preview">
          ${this._previewState === "ready"
            ? html`<div class="preview-inner"></div>`
            : this._previewState === "loading"
              ? html`<div class="loading" aria-label="加载预览"></div>`
              : html`
                  <div class="preview-icon">
                    <ha-icon .icon=${this.icon}></ha-icon>
                  </div>
                `}
        </div>
      </button>
    `;
  }

  static styles = css`
    :host {
      display: block;
      min-width: 0;
    }
    .card {
      width: 100%;
      border: 1px solid var(--divider-color, #e0e0e0);
      border-radius: 12px;
      overflow: hidden;
      cursor: pointer;
      background: var(--card-background-color, #fff);
      padding: 0;
      text-align: inherit;
      font: inherit;
      color: inherit;
      transition:
        border-color 120ms ease,
        box-shadow 120ms ease;
    }
    .card:hover,
    .card:focus-visible {
      border-color: var(--primary-color, #03a9f4);
      box-shadow: 0 0 0 1px var(--primary-color, #03a9f4);
      outline: none;
    }
    .card-header {
      text-align: start;
      padding: 10px 12px 8px;
      font-size: 13px;
      font-weight: 500;
      color: var(--secondary-text-color, #64748b);
      background: var(--card-background-color, #fff);
    }
    .preview {
      height: 200px;
      overflow: hidden;
      display: flex;
      align-items: center;
      justify-content: center;
      padding: 0 10px 10px;
      box-sizing: border-box;
      background: var(--primary-background-color, #fafafa);
    }
    .preview-inner {
      width: 100%;
      max-width: 280px;
      transform: scale(0.95);
      transform-origin: center center;
      pointer-events: none;
    }
    .preview-inner > * {
      width: 100%;
    }
    .preview-icon {
      width: 56px;
      height: 56px;
      border-radius: 16px;
      display: flex;
      align-items: center;
      justify-content: center;
      background: rgba(3, 169, 244, 0.08);
      color: var(--primary-color, #03a9f4);
      --mdc-icon-size: 30px;
    }
    .loading {
      width: 28px;
      height: 28px;
      border-radius: 50%;
      border: 2px solid var(--divider-color, #e0e0e0);
      border-top-color: var(--primary-color, #03a9f4);
      animation: spin 0.8s linear infinite;
    }
    @keyframes spin {
      to {
        transform: rotate(360deg);
      }
    }
  `;
}

@customElement("flow-card-picker")
export class FlowCardPicker extends LitElement {
  @property({ type: Boolean, reflect: true }) public open = false;

  @property({ attribute: false }) public flow?: Flow;

  @state() private _query = "";

  @state() private _scrollRoot?: HTMLElement;

  @state() private _clipboard?: LovelaceCardConfig;

  private _unsubClipboard?: () => void;

  private readonly _backdropDismiss = createBackdropDismissHandlers(() => this._close());

  connectedCallback(): void {
    super.connectedCallback();
    this._refreshClipboard();
    this._unsubClipboard = onCardClipboardChanged(() => this._refreshClipboard());
  }

  disconnectedCallback(): void {
    this._unsubClipboard?.();
    super.disconnectedCallback();
  }

  protected override firstUpdated(): void {
    this._syncScrollRoot();
  }

  protected override updated(changed: PropertyValues): void {
    if (changed.has("open") && this.open) {
      this._query = "";
      this._refreshClipboard();
      this._syncScrollRoot();
    }
  }

  private _refreshClipboard(): void {
    this._clipboard = getCardClipboard();
  }

  private _syncScrollRoot(): void {
    const body = this.renderRoot.querySelector(".body") as HTMLElement | null;
    if (body && body !== this._scrollRoot) {
      this._scrollRoot = body;
    }
  }

  private _close(): void {
    this.dispatchEvent(new CustomEvent("close", { bubbles: true, composed: true }));
  }

  private _onCardSelect(ev: CustomEvent<{ type: string; config?: LovelaceCardConfig }>): void {
    ev.stopPropagation();
    this.dispatchEvent(
      new CustomEvent("card-picked", {
        detail: ev.detail,
        bubbles: true,
        composed: true,
      }),
    );
  }

  private _onSearchInput(ev: Event): void {
    this._query = (ev.target as HTMLInputElement).value;
  }

  private _allCards(): CardRegistryEntry[] {
    return getPickerCards();
  }

  private _filteredCards(): CardRegistryEntry[] {
    return filterCards(this._allCards(), this._query);
  }

  private _suggestedEntries(limit = RECOMMENDED_CARD_LIMIT): CardRegistryEntry[] {
    const all = this._allCards();
    const byType = new Map(all.map((card) => [card.type, card]));
    const recent = getRecentCardTypes()
      .map((type) => byType.get(type))
      .filter((card): card is CardRegistryEntry => Boolean(card));
    const suggested = DEFAULT_SUGGESTED_CARD_TYPES.map((type) => byType.get(type)).filter(
      (card): card is CardRegistryEntry => Boolean(card),
    );
    const seen = new Set<string>();
    const result: CardRegistryEntry[] = [];
    for (const card of [...recent, ...suggested]) {
      if (seen.has(card.type)) continue;
      seen.add(card.type);
      result.push(card);
      if (result.length >= limit) break;
    }
    return result;
  }

  private _renderItem(
    card: CardRegistryEntry,
    index: number,
    options?: { config?: LovelaceCardConfig; name?: string },
  ) {
    return html`
      <flow-card-picker-item
        .type=${card.type}
        .name=${options?.name ?? card.name}
        .icon=${card.icon ?? "mdi:card-outline"}
        .config=${options?.config}
        .index=${index}
        .flow=${this.flow}
        .scrollRoot=${this._scrollRoot}
        @card-select=${this._onCardSelect}
      ></flow-card-picker-item>
    `;
  }

  private _renderGrid(
    cards: CardRegistryEntry[],
    startIndex = 0,
    options?: { configForType?: (type: string) => LovelaceCardConfig | undefined; nameForType?: (type: string) => string | undefined },
  ) {
    if (!cards.length) {
      return html`<div class="empty">没有匹配的卡片</div>`;
    }
    return html`
      <div class="cards">
        ${cards.map((card, index) =>
          this._renderItem(card, startIndex + index, {
            config: options?.configForType?.(card.type),
            name: options?.nameForType?.(card.type),
          }),
        )}
      </div>
    `;
  }

  private _renderClipboardCard(index: number) {
    const clipboard = this._clipboard;
    if (!clipboard) return nothing;
    const entry =
      getCardEntry(clipboard.type) ??
      ({
        type: clipboard.type,
        name: clipboard.type,
        icon: "mdi:content-paste",
        category: "container",
      } as CardRegistryEntry);

    return this._renderItem(entry, index, {
      config: clipboard,
      name: `从剪贴板粘贴`,
    });
  }

  private _renderSections() {
    const query = this._query.trim();
    if (query) {
      return this._renderGrid(this._filteredCards());
    }

    const hasClipboard = Boolean(this._clipboard);
    // Clipboard paste counts toward the recommended limit of 4.
    const suggested = this._suggestedEntries(
      Math.max(0, RECOMMENDED_CARD_LIMIT - (hasClipboard ? 1 : 0)),
    );
    const core = this._allCards();
    let index = 0;

    return html`
      ${hasClipboard || suggested.length
        ? html`
            <ha-expandable-panel .header=${"推荐卡片"} expanded>
              <div class="cards">
                ${hasClipboard ? this._renderClipboardCard(index++) : nothing}
                ${suggested.map((card) => this._renderItem(card, index++))}
              </div>
            </ha-expandable-panel>
          `
        : nothing}

      <ha-expandable-panel .header=${"核心卡"} expanded>
        ${!hasClipboard && !suggested.length
          ? html`<div class="cards">${this._renderClipboardCard(0)}</div>`
          : nothing}
        ${this._renderGrid(core, index)}
      </ha-expandable-panel>
    `;
  }

  protected render() {
    if (!this.open) return nothing;

    return html`
      <div
        class="backdrop"
        @pointerdown=${this._backdropDismiss.onBackdropPointerDown}
        @pointerup=${this._backdropDismiss.onBackdropPointerUp}
        @pointercancel=${this._backdropDismiss.onBackdropPointerCancel}
      >
        <div class="dialog">
          <header class="header">
            <ha-icon-button
              class="close"
              .path=${mdiClose}
              label="关闭"
              @click=${this._close}
            ></ha-icon-button>
            <h2>添加到仪表板</h2>
          </header>

          <div class="search-wrap">
            <label class="search">
              <ha-svg-icon class="search-icon" .path=${mdiMagnify}></ha-svg-icon>
              <input
                type="search"
                placeholder="搜索卡片"
                .value=${this._query}
                @input=${this._onSearchInput}
              />
            </label>
          </div>

          <div class="body">
            ${this._renderSections()}
          </div>

          <footer class="actions">
            <button type="button" class="secondary" @click=${this._close}>取消</button>
          </footer>
        </div>
      </div>
    `;
  }

  static styles = css`
    .backdrop {
      position: fixed;
      inset: 0;
      background: rgba(15, 23, 42, 0.45);
      display: flex;
      align-items: center;
      justify-content: center;
      z-index: 150;
      padding: 16px;
      box-sizing: border-box;
    }
    .dialog {
      width: min(880px, 100%);
      max-height: min(90vh, 820px);
      background: var(--card-background-color, #fff);
      border-radius: 16px;
      display: flex;
      flex-direction: column;
      box-shadow: 0 20px 50px rgba(0, 0, 0, 0.18);
      overflow: hidden;
      color: var(--primary-text-color, #1e293b);
      font-family: var(--ha-font-family, "Segoe UI", system-ui, sans-serif);
    }
    .header {
      display: flex;
      align-items: center;
      flex-wrap: wrap;
      gap: 4px;
      padding: 8px 8px 0 4px;
      flex: none;
    }
    h2 {
      margin: 0;
      font-size: 18px;
      font-weight: 600;
      text-align: start;
      flex: 1;
      min-width: 0;
    }
    .close {
      flex: none;
    }
    .search-wrap {
      padding: 16px 20px 0;
      flex: none;
    }
    .search {
      display: flex;
      align-items: center;
      gap: 8px;
      border: 1px solid var(--divider-color, #e0e0e0);
      border-radius: 10px;
      padding: 0 12px;
      background: var(--card-background-color, #fff);
      min-height: 44px;
      box-sizing: border-box;
    }
    .search-icon {
      width: 20px;
      height: 20px;
      color: var(--secondary-text-color, #64748b);
      flex: none;
    }
    .search input {
      border: none;
      background: transparent;
      flex: 1;
      min-width: 0;
      font: inherit;
      font-size: 14px;
      color: inherit;
      outline: none;
    }
    .search input::placeholder {
      color: var(--secondary-text-color, #94a3b8);
    }
    .body {
      flex: 1;
      min-height: 0;
      overflow: auto;
      padding: 8px 20px 8px;
    }
    ha-expandable-panel {
      display: block;
      margin-bottom: 8px;
    }
    .cards {
      display: grid;
      grid-template-columns: repeat(2, minmax(0, 1fr));
      gap: 16px;
      padding-bottom: 8px;
    }
    @media (max-width: 520px) {
      .cards {
        grid-template-columns: minmax(0, 1fr);
      }
    }
    .empty {
      padding: 32px 0;
      text-align: center;
      color: var(--secondary-text-color, #64748b);
      font-size: 14px;
    }
    .actions {
      display: flex;
      justify-content: flex-end;
      gap: 8px;
      padding: 12px 20px 16px;
      border-top: 1px solid var(--divider-color, #e0e0e0);
      flex: none;
    }
    .secondary {
      border: none;
      background: transparent;
      color: var(--primary-color, #03a9f4);
      border-radius: 999px;
      height: 36px;
      padding: 0 18px;
      cursor: pointer;
      font-size: 14px;
      font-weight: 600;
    }
    .secondary:hover {
      background: rgba(3, 169, 244, 0.08);
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "flow-card-picker": FlowCardPicker;
    "flow-card-picker-item": FlowCardPickerItem;
  }
}
