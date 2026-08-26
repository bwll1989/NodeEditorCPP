import { css, html, LitElement, nothing } from "lit";
import { customElement, property } from "lit/decorators.js";
import type { LovelaceCardConfig } from "../types";
import { createBackdropDismissHandlers } from "../common/dom/modal-backdrop";
import "../panels/lovelace/hui-card-container";

@customElement("flow-delete-card-dialog")
export class FlowDeleteCardDialog extends LitElement {
  @property({ type: Boolean }) public open = false;

  @property({ attribute: false }) public cardConfig?: LovelaceCardConfig;

  private readonly _backdropDismiss = createBackdropDismissHandlers(() => this._cancel());

  private _cancel(): void {
    this.dispatchEvent(
      new CustomEvent("dialog-result", { bubbles: true, composed: true, detail: { confirmed: false } }),
    );
  }

  private _confirm(): void {
    this.dispatchEvent(
      new CustomEvent("dialog-result", { bubbles: true, composed: true, detail: { confirmed: true } }),
    );
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
          ${this.cardConfig
            ? html`
                <div class="preview">
                  <hui-card-container
                    .config=${this.cardConfig}
                    .editMode=${false}
                  ></hui-card-container>
                </div>
              `
            : nothing}
          <div class="footer">
            <button type="button" class="btn cancel" @click=${this._cancel}>取消</button>
            <button type="button" class="btn confirm destructive" @click=${this._confirm}>
              删除
            </button>
          </div>
        </div>
      </div>
    `;
  }

  static styles = css`
    .backdrop {
      position: fixed;
      inset: 0;
      z-index: 300;
      background: rgba(0, 0, 0, 0.4);
      display: flex;
      align-items: center;
      justify-content: center;
      padding: 24px;
      box-sizing: border-box;
    }
    .dialog {
      width: min(420px, 100%);
      background: var(--card-background-color, #fff);
      border-radius: 28px;
      padding: 16px 16px 12px;
      box-shadow:
        0 11px 15px -7px rgba(0, 0, 0, 0.2),
        0 24px 38px 3px rgba(0, 0, 0, 0.14),
        0 9px 46px 8px rgba(0, 0, 0, 0.12);
    }
    .preview {
      position: relative;
      margin: 4px auto 12px;
      max-width: 500px;
      pointer-events: none;
    }
  /* hui-dialog-delete-card preview */
    .preview hui-card-container {
      display: block;
      width: 100%;
    }
    .footer {
      display: flex;
      justify-content: flex-end;
      align-items: center;
      gap: 8px;
    }
    .btn {
      border: none;
      background: transparent;
      cursor: pointer;
      font: inherit;
      font-size: 14px;
      font-weight: 500;
      padding: 10px 16px;
      border-radius: 20px;
      min-width: 64px;
    }
    .btn.cancel {
      color: var(--primary-color, #03a9f4);
    }
    .btn.cancel:hover {
      background: rgba(3, 169, 244, 0.08);
    }
    .btn.confirm.destructive {
      background: #f8b4b4;
      color: #5c1a1a;
    }
    .btn.confirm.destructive:hover {
      background: #f5a3a3;
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "flow-delete-card-dialog": FlowDeleteCardDialog;
  }
}
