import { css, html, LitElement, nothing, type PropertyValues } from "lit";
import { customElement, property, query, state } from "lit/decorators.js";
import { createBackdropDismissHandlers } from "../common/dom/modal-backdrop";

@customElement("flow-auth-dialog")
export class FlowAuthDialog extends LitElement {
  @property({ type: Boolean }) public open = false;

  @property() public title = "身份验证";

  @property() public hint = "";

  @property() public error = "";

  @state() private _password = "";

  @state() private _submitting = false;

  @query("#auth-password") private _input?: HTMLInputElement;

  private readonly _backdropDismiss = createBackdropDismissHandlers(() => this._cancel());

  protected updated(changed: PropertyValues): void {
    if (changed.has("open") && this.open) {
      this._password = "";
      this._submitting = false;
      window.setTimeout(() => {
        this._input?.focus();
        this._input?.select();
      }, 40);
    }
  }

  private _cancel(): void {
    if (this._submitting) return;
    this.dispatchEvent(
      new CustomEvent("dialog-result", {
        bubbles: true,
        composed: true,
        detail: { password: null },
      }),
    );
  }

  private _submit(): void {
    if (this._submitting) return;
    this._submitting = true;
    this.dispatchEvent(
      new CustomEvent("dialog-result", {
        bubbles: true,
        composed: true,
        detail: { password: this._password },
      }),
    );
  }

  private _onKeydown(ev: KeyboardEvent): void {
    if (ev.key === "Enter") {
      ev.preventDefault();
      this._submit();
    } else if (ev.key === "Escape") {
      ev.preventDefault();
      this._cancel();
    }
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
          role="dialog"
          aria-modal="true"
          aria-labelledby="auth-title"
          @keydown=${this._onKeydown}
        >
          <h2 id="auth-title" class="title">${this.title}</h2>
          ${this.hint ? html`<p class="hint">${this.hint}</p>` : nothing}
          <label class="field">
            <span>编辑口令</span>
            <input
              id="auth-password"
              type="password"
              autocomplete="current-password"
              .value=${this._password}
              placeholder="请输入密码"
              ?disabled=${this._submitting}
              @input=${(ev: Event) => {
                this._password = (ev.target as HTMLInputElement).value;
              }}
            />
          </label>
          ${this.error ? html`<div class="error" role="alert">${this.error}</div>` : nothing}
          <div class="footer">
            <button type="button" class="btn cancel" @click=${this._cancel} ?disabled=${this._submitting}>
              取消
            </button>
            <button type="button" class="btn confirm" @click=${this._submit} ?disabled=${this._submitting}>
              ${this._submitting ? "验证中…" : "确定"}
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
      z-index: 320;
      background: rgba(0, 0, 0, 0.45);
      display: flex;
      align-items: center;
      justify-content: center;
      padding: 24px;
      box-sizing: border-box;
    }
    .dialog {
      width: min(380px, 100%);
      background: var(--card-background-color, #fff);
      border-radius: 20px;
      padding: 24px 24px 16px;
      box-shadow:
        0 11px 15px -7px rgba(0, 0, 0, 0.2),
        0 24px 38px 3px rgba(0, 0, 0, 0.14),
        0 9px 46px 8px rgba(0, 0, 0, 0.12);
    }
    .title {
      margin: 0 0 8px;
      font-size: 20px;
      font-weight: 500;
      line-height: 1.3;
      color: var(--primary-text-color);
    }
    .hint {
      margin: 0 0 16px;
      font-size: 13px;
      line-height: 1.5;
      color: var(--secondary-text-color);
    }
    .field {
      display: flex;
      flex-direction: column;
      gap: 6px;
      font-size: 13px;
      color: var(--secondary-text-color);
    }
    .field input {
      height: 40px;
      border: 1px solid var(--divider-color, #e0e0e0);
      border-radius: 10px;
      padding: 0 12px;
      font: inherit;
      font-size: 14px;
      color: var(--primary-text-color);
      background: var(--card-background-color, #fff);
      box-sizing: border-box;
    }
    .field input:focus {
      outline: none;
      border-color: var(--primary-color, #03a9f4);
      box-shadow: 0 0 0 2px rgba(3, 169, 244, 0.2);
    }
    .error {
      margin-top: 10px;
      font-size: 13px;
      color: var(--error-color, #b91c1c);
    }
    .footer {
      display: flex;
      justify-content: flex-end;
      align-items: center;
      gap: 8px;
      margin-top: 18px;
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
    .btn:disabled {
      opacity: 0.55;
      cursor: default;
    }
    .btn.cancel {
      color: var(--primary-color, #03a9f4);
    }
    .btn.cancel:hover:not(:disabled) {
      background: rgba(3, 169, 244, 0.08);
    }
    .btn.confirm {
      background: var(--primary-color, #03a9f4);
      color: #fff;
    }
    .btn.confirm:hover:not(:disabled) {
      filter: brightness(1.05);
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "flow-auth-dialog": FlowAuthDialog;
  }
}
