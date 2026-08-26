import { css, html, LitElement, nothing } from "lit";
import { customElement, property } from "lit/decorators.js";
import { createBackdropDismissHandlers } from "../common/dom/modal-backdrop";

@customElement("flow-confirm-dialog")
export class FlowConfirmDialog extends LitElement {
  @property({ type: Boolean }) public open = false;

  @property() public title = "";

  @property() public text = "";

  @property() public confirmText = "删除";

  @property() public cancelText = "取消";

  @property({ type: Boolean }) public destructive = false;

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
        <div
          class="dialog"
          role="alertdialog"
          aria-labelledby="confirm-title"
          aria-describedby="confirm-text"
        >
          <h2 id="confirm-title" class="title">${this.title}</h2>
          ${this.text ? html`<p id="confirm-text" class="text">${this.text}</p>` : nothing}
          <div class="footer">
            <button type="button" class="btn cancel" @click=${this._cancel}>
              ${this.cancelText}
            </button>
            <button
              type="button"
              class="btn confirm ${this.destructive ? "destructive" : ""}"
              @click=${this._confirm}
            >
              ${this.confirmText}
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
      width: min(360px, 100%);
      background: var(--card-background-color, #fff);
      border-radius: 28px;
      padding: 24px 24px 16px;
      box-shadow:
        0 11px 15px -7px rgba(0, 0, 0, 0.2),
        0 24px 38px 3px rgba(0, 0, 0, 0.14),
        0 9px 46px 8px rgba(0, 0, 0, 0.12);
    }
    .title {
      margin: 0 0 12px;
      font-size: 20px;
      font-weight: 500;
      line-height: 1.3;
      color: var(--primary-text-color);
    }
    .text {
      margin: 0 0 20px;
      font-size: 14px;
      line-height: 1.5;
      color: var(--secondary-text-color);
    }
    .footer {
      display: flex;
      justify-content: flex-end;
      align-items: center;
      gap: 8px;
      margin-top: 8px;
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
    .btn.confirm {
      color: var(--primary-color, #03a9f4);
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
    "flow-confirm-dialog": FlowConfirmDialog;
  }
}
