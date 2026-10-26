import { css, html, LitElement, nothing, type PropertyValues } from "lit";
import { customElement, property, state } from "lit/decorators.js";
import { createBackdropDismissHandlers } from "../common/dom/modal-backdrop";
import { collectCardEntityAddresses } from "../data/flow";
import type { Flow, LovelaceCard, LovelaceCardConfig } from "../types";
import { createCardElement } from "../panels/lovelace/create-element/create-card-element";

@customElement("flow-badge-card-dialog")
export class FlowBadgeCardDialog extends LitElement {
  @property({ type: Boolean }) public open = false;

  @property({ attribute: false }) public flow?: Flow;

  @property({ attribute: false }) public cardConfig?: LovelaceCardConfig;

  @property() public title = "";

  @state() private _card?: LovelaceCard;

  @state() private _error = "";

  @state() private _statesRevision = 0;

  private readonly _backdropDismiss = createBackdropDismissHandlers(() => this._close());

  private _loadToken = 0;

  private _entityUnsubs: Array<() => void> = [];

  disconnectedCallback(): void {
    this._clearEntitySubscriptions();
    super.disconnectedCallback();
  }

  protected updated(changed: PropertyValues): void {
    if (!this.open) {
      if (changed.has("open")) {
        this._clearEntitySubscriptions();
        this._card = undefined;
        this._error = "";
      }
      return;
    }
    if (changed.has("open") || changed.has("cardConfig")) {
      void this._mountCard();
      return;
    }
    if (changed.has("flow") && this._card) {
      this._card.flow = this.flow;
      this._syncEntitySubscriptions();
      this._refreshCard();
    }
  }

  private _refreshCard(): void {
    const card = this._card as (LovelaceCard & { requestUpdate?: () => void }) | undefined;
    card?.requestUpdate?.();
    this.requestUpdate();
  }

  private _clearEntitySubscriptions(): void {
    this._entityUnsubs.forEach((unsub) => unsub());
    this._entityUnsubs = [];
  }

  private _syncEntitySubscriptions(): void {
    this._clearEntitySubscriptions();
    const flow = this.flow;
    if (!flow || !this._card) return;

    const addresses = collectCardEntityAddresses(this.cardConfig);
    if (addresses.length === 0) {
      this._entityUnsubs.push(
        flow.subscribeStates(() => {
          this._statesRevision = flow.statesRevision;
          this._refreshCard();
        }),
      );
      return;
    }

    addresses.forEach((address) => {
      this._entityUnsubs.push(
        flow.subscribeEntity(address, () => {
          this._statesRevision = flow.statesRevision;
          this._refreshCard();
        }),
      );
    });
  }

  private async _mountCard(): Promise<void> {
    const config = this.cardConfig;
    if (!config) {
      this._clearEntitySubscriptions();
      this._card = undefined;
      this._error = "未配置弹出卡片";
      return;
    }
    const token = ++this._loadToken;
    this._error = "";
    try {
      const card = await createCardElement(config);
      if (token !== this._loadToken) return;
      card.flow = this.flow;
      this._card = card;
      this._syncEntitySubscriptions();
    } catch (err) {
      if (token !== this._loadToken) return;
      this._clearEntitySubscriptions();
      this._card = undefined;
      this._error = err instanceof Error ? err.message : String(err);
    }
  }

  private _close(): void {
    this.dispatchEvent(
      new CustomEvent("dialog-closed", { bubbles: true, composed: true }),
    );
  }

  protected render() {
    if (!this.open) return nothing;
    void this._statesRevision;
    const heading = this.title.trim() || String(this.cardConfig?.name ?? "控制");

    return html`
      <div
        class="backdrop"
        @pointerdown=${this._backdropDismiss.onBackdropPointerDown}
        @pointerup=${this._backdropDismiss.onBackdropPointerUp}
        @pointercancel=${this._backdropDismiss.onBackdropPointerCancel}
      >
        <div class="dialog" role="dialog" aria-modal="true" aria-label=${heading}>
          <header class="header">
            <h2 class="title">${heading}</h2>
            <button type="button" class="close" aria-label="关闭" @click=${this._close}>×</button>
          </header>
          <div class="body">
            ${this._error
              ? html`<p class="error">${this._error}</p>`
              : this._card
                ? this._card
                : html`<p class="loading">加载中…</p>`}
          </div>
        </div>
      </div>
    `;
  }

  static styles = css`
    .backdrop {
      position: fixed;
      inset: 0;
      z-index: 320;
      background: rgba(0, 0, 0, 0.45);
      display: flex;
      align-items: center;
      justify-content: center;
      padding: 24px;
      box-sizing: border-box;
    }
    .dialog {
      width: min(560px, 100%);
      max-height: min(86vh, 720px);
      overflow: auto;
      background: var(--primary-background-color, #f5f5f5);
      border-radius: 20px;
      box-shadow:
        0 11px 15px -7px rgba(0, 0, 0, 0.2),
        0 24px 38px 3px rgba(0, 0, 0, 0.14),
        0 9px 46px 8px rgba(0, 0, 0, 0.12);
    }
    .header {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 12px;
      padding: 16px 16px 8px;
      position: sticky;
      top: 0;
      background: var(--primary-background-color, #f5f5f5);
      z-index: 1;
    }
    .title {
      margin: 0;
      font-size: 18px;
      font-weight: 600;
      color: var(--primary-text-color);
    }
    .close {
      width: 32px;
      height: 32px;
      border: none;
      border-radius: 50%;
      background: color-mix(in srgb, var(--primary-text-color) 8%, transparent);
      color: var(--primary-text-color);
      font-size: 22px;
      line-height: 1;
      cursor: pointer;
    }
    .body {
      padding: 8px 16px 16px;
    }
    .body > * {
      display: block;
      width: 100%;
    }
    .loading,
    .error {
      margin: 12px 0;
      font-size: 13px;
      color: var(--secondary-text-color);
    }
    .error {
      color: var(--error-color, #db4437);
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "flow-badge-card-dialog": FlowBadgeCardDialog;
  }
}
