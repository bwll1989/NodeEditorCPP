import { css, html, LitElement, nothing } from "lit";
import { customElement, state } from "lit/decorators.js";
import { actionDisplayName, createAction } from "../../../data/actions";

@customElement("flow-manual-action-form")
export class FlowManualActionForm extends LitElement {
  @state() private _entity = "";

  @state() private _name = "";

  @state() private _error = "";

  @state() private _adding = false;

  @state() private _success = "";

  private _reset(): void {
    this._entity = "";
    this._name = "";
    this._error = "";
  }

  private async _submit(): Promise<void> {
    const entity = this._entity.trim();
    if (!entity) {
      this._error = "请输入控制地址";
      return;
    }
    if (!entity.startsWith("/")) {
      this._error = "控制地址应以 / 开头";
      return;
    }

    this._adding = true;
    this._error = "";
    this._success = "";
    const name = this._name.trim() || entity;
    const item = await createAction(entity, name);
    this._adding = false;
    if (!item) {
      this._error = "添加失败，请检查地址是否有效";
      return;
    }

    this._success = `已添加：${actionDisplayName(item)}`;
    this._reset();
  }

  protected render() {
    return html`
      <p class="help">无节点右键导出时，可在此手动登记控制地址。</p>
      <div class="form">
        <label class="field">
          <span class="label">控制地址</span>
          <input
            type="text"
            .value=${this._entity}
            placeholder="/dataflow/0/mix"
            ?disabled=${this._adding}
            @input=${(ev: Event) => {
              this._entity = (ev.target as HTMLInputElement).value;
              this._error = "";
              this._success = "";
            }}
          />
        </label>
        <label class="field">
          <span class="label">显示名称</span>
          <input
            type="text"
            .value=${this._name}
            placeholder="留空则用地址"
            ?disabled=${this._adding}
            @input=${(ev: Event) => {
              this._name = (ev.target as HTMLInputElement).value;
            }}
          />
        </label>
        <div class="actions">
          <button
            type="button"
            class="btn primary"
            ?disabled=${this._adding}
            @click=${() => void this._submit()}
          >
            ${this._adding ? "添加中…" : "添加"}
          </button>
        </div>
      </div>
      ${this._error ? html`<p class="error">${this._error}</p>` : nothing}
      ${this._success ? html`<p class="success">${this._success}</p>` : nothing}
    `;
  }

  static styles = css`
    :host {
      display: block;
    }
    .help {
      margin: 0 0 8px;
      font-size: 12px;
      line-height: 1.4;
      color: var(--secondary-text-color);
    }
    .form {
      display: grid;
      grid-template-columns: minmax(0, 1.3fr) minmax(0, 1fr) auto;
      gap: 8px;
      align-items: end;
    }
    .field {
      display: flex;
      flex-direction: column;
      gap: 4px;
      min-width: 0;
    }
    .label {
      font-size: 12px;
      color: var(--secondary-text-color);
    }
    input {
      width: 100%;
      box-sizing: border-box;
      height: 32px;
      font: inherit;
      font-size: 13px;
      border: 1px solid var(--divider-color);
      border-radius: 8px;
      padding: 0 8px;
      color: var(--primary-text-color);
      background: #fff;
    }
    .error,
    .success {
      margin: 6px 0 0;
      font-size: 12px;
    }
    .error {
      color: var(--error-color, #db4437);
    }
    .success {
      color: var(--success-color, #0f9d58);
    }
    .actions {
      display: flex;
      align-items: flex-end;
    }
    .btn {
      border: 1px solid var(--divider-color);
      border-radius: 8px;
      background: #fff;
      color: var(--primary-text-color);
      font: inherit;
      font-size: 13px;
      font-weight: 600;
      height: 32px;
      padding: 0 12px;
      cursor: pointer;
      white-space: nowrap;
    }
    .btn.primary {
      border-color: var(--primary-color);
      background: var(--primary-color);
      color: #fff;
    }
    .btn:disabled {
      opacity: 0.6;
      cursor: not-allowed;
    }
    @media (max-width: 520px) {
      .form {
        grid-template-columns: 1fr;
      }
      .actions {
        justify-content: flex-start;
      }
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "flow-manual-action-form": FlowManualActionForm;
  }
}
