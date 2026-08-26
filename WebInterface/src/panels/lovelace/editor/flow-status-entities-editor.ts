import { css, html, LitElement } from "lit";
import { customElement, property } from "lit/decorators.js";
import type { EntityPickerValueChangedDetail } from "../../../components/ha-entity-picker";
import type { LovelaceStatusEntityConfig } from "../../../types";
import "../../../components/ha-entity-picker";
import "../../../components/ha-icon-picker";

@customElement("flow-status-entities-editor")
export class FlowStatusEntitiesEditor extends LitElement {
  @property({ attribute: false }) public entities: LovelaceStatusEntityConfig[] = [];

  private _emit(entities: LovelaceStatusEntityConfig[]): void {
    this.dispatchEvent(
      new CustomEvent("entities-changed", {
        bubbles: true,
        composed: true,
        detail: { entities },
      }),
    );
  }

  private _add(): void {
    this._emit([...this.entities, { entity: "", name: "", icon: "" }]);
  }

  private _remove(index: number): void {
    this._emit(this.entities.filter((_, i) => i !== index));
  }

  private _patch(index: number, patch: Partial<LovelaceStatusEntityConfig>): void {
    this._emit(this.entities.map((item, i) => (i === index ? { ...item, ...patch } : item)));
  }

  protected render() {
    return html`
      <div class="editor">
        <div class="head">
          <span class="label">状态项（每项绑定一个动作）</span>
          <button type="button" class="add" @click=${this._add}>+ 添加状态</button>
        </div>
        ${this.entities.length === 0
          ? html`<p class="empty">添加动作后，每行显示对应动作的实时状态</p>`
          : this.entities.map(
              (item, index) => html`
                <div class="row">
                  <span class="index">${index + 1}</span>
                  <ha-entity-picker
                    compact
                    label=""
                    placeholder="动作"
                    .value=${item.entity ?? ""}
                    @value-changed=${(ev: CustomEvent<EntityPickerValueChangedDetail>) => {
                      ev.stopPropagation();
                      this._patch(index, {
                        entity: ev.detail.value,
                        ...(ev.detail.actionName ? { name: ev.detail.actionName } : {}),
                      });
                    }}
                  ></ha-entity-picker>
                  <input
                    class="name"
                    type="text"
                    .value=${item.name ?? ""}
                    placeholder="名称"
                    @input=${(ev: Event) =>
                      this._patch(index, { name: (ev.target as HTMLInputElement).value })}
                  />
                  <ha-icon-picker
                    compact
                    label="图标"
                    .value=${item.icon ?? ""}
                    @icon-changed=${(ev: CustomEvent<{ value: string }>) => {
                      ev.stopPropagation();
                      this._patch(index, { icon: ev.detail.value });
                    }}
                  ></ha-icon-picker>
                  <button
                    type="button"
                    class="remove"
                    aria-label="删除"
                    title="删除"
                    @click=${() => this._remove(index)}
                  >
                    ×
                  </button>
                </div>
              `,
            )}
      </div>
    `;
  }

  static styles = css`
    .editor {
      margin-bottom: 12px;
    }
    .head {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 8px;
      margin-bottom: 8px;
    }
    .label {
      font-size: 13px;
      font-weight: 600;
      color: var(--primary-text-color);
    }
    .add {
      border: none;
      background: transparent;
      color: var(--primary-color, #03a9f4);
      font-size: 13px;
      font-weight: 600;
      cursor: pointer;
      padding: 0;
    }
    .empty {
      margin: 0 0 8px;
      font-size: 12px;
      color: var(--secondary-text-color);
      line-height: 1.4;
    }
    .row {
      display: flex;
      align-items: center;
      gap: 8px;
      padding: 6px 0;
      border-top: 1px solid var(--divider-color);
    }
    .index {
      flex: 0 0 20px;
      font-size: 12px;
      color: var(--secondary-text-color);
      text-align: center;
    }
    .row ha-entity-picker {
      flex: 1 1 45%;
      min-width: 96px;
    }
    .name {
      flex: 0 1 88px;
      min-width: 64px;
      box-sizing: border-box;
      height: 36px;
      font: inherit;
      font-size: 13px;
      border: 1px solid var(--divider-color);
      border-radius: 8px;
      padding: 0 10px;
      color: var(--primary-text-color);
      background: #fff;
    }
    .row ha-icon-picker {
      flex: 0 1 108px;
      min-width: 80px;
    }
    .remove {
      flex: 0 0 28px;
      width: 28px;
      height: 28px;
      border: none;
      border-radius: 6px;
      background: transparent;
      color: var(--secondary-text-color);
      font-size: 20px;
      line-height: 1;
      cursor: pointer;
      padding: 0;
    }
    .remove:hover {
      color: var(--error-color, #db4437);
      background: rgba(219, 68, 55, 0.08);
    }
    @media (max-width: 520px) {
      .row {
        flex-wrap: wrap;
      }
      .row ha-entity-picker {
        flex: 1 1 100%;
      }
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "flow-status-entities-editor": FlowStatusEntitiesEditor;
  }
}
