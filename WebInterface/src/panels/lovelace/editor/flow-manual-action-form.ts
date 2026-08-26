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
            placeholder="留空则使用控制地址"
            ?disabled=${this._adding}
            @input=${(ev: Event) => {
              this._name = (ev.target as HTMLInputElement).value;
            }}
          />
        </label>
        ${this._error ? html`<p class="error">${this._error}</p>` : nothing}
        ${this._success ? html`<p class="success">${this._success}</p>` : nothing}
        <div class="actions">
          <button
            type="button"
            class="btn primary"
            ?disabled=${this._adding}
            @click=${() => void this._submit()}
          >
            ${this._adding ? "添加中…" : "添加到动作库"}
          </button>
        </div>
      </div>
    `;
  }

  static styles = css`
    :host {
      display: block;
    }
    .help {
      margin: 0 0 12px;
      font-size: 13px;
      line-height: 1.45;
      color: var(--secondary-text-color);
    }
    .form {
      display: flex;
      flex-direction: column;
      gap: 0;
    }
    .field {
      display: flex;
      flex-direction: column;
      gap: 6px;
      margin-bottom: 12px;
    }
    .label {
      font-size: 13px;
      color: var(--secondary-text-color);
    }
    input {
      width: 100%;
      box-sizing: border-box;
      font: inherit;
      border: 1px solid var(--divider-color);
      border-radius: 8px;
      padding: 8px 10px;
      color: var(--primary-text-color);
      background: #fff;
    }
    .error {
      margin: -4px 0 8px;
      font-size: 12px;
      color: var(--error-color, #db4437);
    }
    .success {
      margin: -4px 0 8px;
      font-size: 12px;
      color: var(--success-color, #0f9d58);
    }
    .actions {
      display: flex;
      align-items: center;
      gap: 8px;
      flex-wrap: wrap;
    }
    .btn {
      border: 1px solid var(--divider-color);
      border-radius: 8px;
      background: #fff;
      color: var(--primary-text-color);
      font: inherit;
      font-size: 13px;
      font-weight: 600;
      padding: 8px 14px;
      cursor: pointer;
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
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "flow-manual-action-form": FlowManualActionForm;
  }
}
