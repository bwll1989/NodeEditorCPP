import { css, html, LitElement, nothing } from "lit";
import { customElement, property, state } from "lit/decorators.js";
import type { EntityPickerValueChangedDetail } from "../../../components/ha-entity-picker";
import type { LovelacePictureElementConfig } from "../../../types";
import { parseBadgeWriteValue } from "../heading-badges/badge-value";
import { badgeShowText } from "../heading-badges/badge-flags";
import { parsePercent, withPosition } from "../picture-elements/position";
import "../../../components/ha-entity-picker";
import "../../../components/ha-icon-picker";
import "../../../components/ha-color-picker";
import "../../../components/ha-icon";

@customElement("flow-picture-elements-editor")
export class FlowPictureElementsEditor extends LitElement {
  @property({ attribute: false }) public elements: LovelacePictureElementConfig[] = [];

  @state() private _openIndex: number | null = 0;

  get selectedIndex(): number {
    if (this._openIndex !== null && this.elements[this._openIndex]) return this._openIndex;
    return this.elements.length > 0 ? 0 : -1;
  }

  applyPosition(left: number, top: number): void {
    const index = this.selectedIndex;
    if (index < 0) return;
    this._emit(this.elements.map((el, i) => (i === index ? withPosition(el, top, left) : el)));
  }

  private _emit(elements: LovelacePictureElementConfig[]): void {
    this.dispatchEvent(
      new CustomEvent("elements-changed", {
        bubbles: true,
        composed: true,
        detail: { elements },
      }),
    );
  }

  private _add(type: "state-badge" | "button"): void {
    const next: LovelacePictureElementConfig =
      type === "button"
        ? {
            type: "button",
            entity: "",
            icon: "mdi:gesture-tap-button",
            text: "触发",
            value: true,
            show_name: true,
            style: { top: "50%", left: "50%" },
          }
        : {
            type: "state-badge",
            entity: "",
            icon: "mdi:thermometer",
            show_icon: true,
            show_name: true,
            style: { top: "40%", left: "40%" },
          };
    const elements = [...this.elements, next];
    this._openIndex = elements.length - 1;
    this._emit(elements);
  }

  private _remove(index: number): void {
    const elements = this.elements.filter((_, i) => i !== index);
    this._openIndex = elements.length === 0 ? null : Math.min(index, elements.length - 1);
    this._emit(elements);
  }

  private _duplicate(index: number): void {
    const current = this.elements[index];
    if (!current) return;
    const pos = {
      top: Math.min(90, parsePercent(current.style?.top, 50) + 6),
      left: Math.min(90, parsePercent(current.style?.left, 50) + 6),
    };
    const copy = withPosition({ ...current }, pos.top, pos.left);
    const elements = [...this.elements.slice(0, index + 1), copy, ...this.elements.slice(index + 1)];
    this._openIndex = index + 1;
    this._emit(elements);
  }

  private _move(index: number, delta: number): void {
    const next = index + delta;
    if (next < 0 || next >= this.elements.length) return;
    const elements = [...this.elements];
    const [item] = elements.splice(index, 1);
    elements.splice(next, 0, item);
    this._openIndex = next;
    this._emit(elements);
  }

  private _patch(index: number, patch: Partial<LovelacePictureElementConfig>): void {
    this._emit(this.elements.map((el, i) => (i === index ? { ...el, ...patch } : el)));
  }

  private _changeType(index: number, type: "state-badge" | "button"): void {
    const current = this.elements[index];
    if (type === "button") {
      this._patch(index, {
        type: "button",
        text: String(current?.text ?? current?.name ?? "触发"),
        value: current?.value ?? true,
        icon: current?.icon || "mdi:gesture-tap-button",
        show_name: current?.show_name ?? true,
      });
      return;
    }
    this._patch(index, {
      type: "state-badge",
      icon: current?.icon || "mdi:thermometer",
      show_icon: current?.show_icon ?? true,
      show_name: current?.show_name ?? current?.show_state ?? true,
    });
  }

  private _setPos(index: number, axis: "top" | "left", raw: string): void {
    const current = this.elements[index];
    if (!current) return;
    const top = axis === "top" ? parsePercent(raw, 50) : parsePercent(current.style?.top, 50);
    const left = axis === "left" ? parsePercent(raw, 50) : parsePercent(current.style?.left, 50);
    this._emit(this.elements.map((el, i) => (i === index ? withPosition(el, top, left) : el)));
  }

  private _title(element: LovelacePictureElementConfig): string {
    return element.type === "button" ? "控制徽章" : "状态徽章";
  }

  private _subtitle(element: LovelacePictureElementConfig): string {
    return String(element.entity || element.text || element.name || "未选择地址");
  }

  protected render() {
    return html`
      <div class="elements-editor">
        <div class="head">
          <span class="label">元素</span>
          <div class="add-group">
            <button type="button" class="add" @click=${() => this._add("state-badge")}>+ 状态徽章</button>
            <button type="button" class="add" @click=${() => this._add("button")}>+ 控制徽章</button>
          </div>
        </div>
        <p class="hint">选中一项后，可在右侧预览图上点击或拖动徽章来定位。</p>
        ${this.elements.length === 0
          ? html`<p class="empty">还没有元素。添加后会叠在底图上。</p>`
          : this.elements.map((element, index) => {
              const type = element.type === "button" ? "button" : "state-badge";
              const open = this._openIndex === index;
              return html`
                <div class="row ${open ? "open" : ""}">
                  <div class="row-head">
                    <div class="meta" @click=${() => {
                      this._openIndex = open ? null : index;
                    }}>
                      <strong>${this._title(element)}</strong>
                      <span>${this._subtitle(element)}</span>
                    </div>
                    <div class="actions">
                      <button type="button" title="上移" @click=${() => this._move(index, -1)}>↑</button>
                      <button type="button" title="下移" @click=${() => this._move(index, 1)}>↓</button>
                      <button type="button" title="复制" @click=${() => this._duplicate(index)}>
                        <ha-icon icon="mdi:content-copy"></ha-icon>
                      </button>
                      <button type="button" class="danger" title="删除" @click=${() => this._remove(index)}>
                        <ha-icon icon="mdi:delete"></ha-icon>
                      </button>
                    </div>
                  </div>
                  ${open
                    ? html`
                        <div class="row-body">
                          <label class="field">
                            <span>类型</span>
                            <select
                              @change=${(ev: Event) =>
                                this._changeType(
                                  index,
                                  (ev.target as HTMLSelectElement).value as "state-badge" | "button",
                                )}
                            >
                              <option value="state-badge" ?selected=${type === "state-badge"}>状态徽章</option>
                              <option value="button" ?selected=${type === "button"}>控制徽章</option>
                            </select>
                          </label>
                          <ha-entity-picker
                            label="动作"
                            .value=${String(element.entity ?? "")}
                            ?optional=${type === "button"}
                            @value-changed=${(ev: CustomEvent<EntityPickerValueChangedDetail>) =>
                              this._patch(index, { entity: ev.detail.value })}
                          ></ha-entity-picker>
                          <ha-icon-picker
                            label="图标"
                            .value=${String(element.icon ?? "")}
                            placeholder=${type === "button" ? "mdi:gesture-tap-button" : "mdi:thermometer"}
                            @icon-changed=${(ev: CustomEvent<{ value: string }>) =>
                              this._patch(index, { icon: ev.detail.value })}
                          ></ha-icon-picker>
                          <ha-color-picker
                            label="颜色"
                            .value=${String(element.color ?? "")}
                            ?includeState=${type === "state-badge"}
                            @color-changed=${(ev: CustomEvent<{ value: string }>) =>
                              this._patch(index, { color: ev.detail.value })}
                          ></ha-color-picker>
                          ${type === "state-badge"
                            ? html`
                                <label class="switch-row">
                                  <span>显示图标</span>
                                  <input
                                    type="checkbox"
                                    .checked=${element.show_icon !== false}
                                    @change=${(ev: Event) =>
                                      this._patch(index, {
                                        show_icon: (ev.target as HTMLInputElement).checked,
                                      })}
                                  />
                                </label>
                                <label class="switch-row">
                                  <span>显示文字</span>
                                  <input
                                    type="checkbox"
                                    .checked=${badgeShowText(element)}
                                    @change=${(ev: Event) =>
                                      this._patch(index, {
                                        show_name: (ev.target as HTMLInputElement).checked,
                                      })}
                                  />
                                </label>
                                <label class="field">
                                  <span>单位</span>
                                  <input
                                    .value=${String(element.unit ?? "")}
                                    placeholder="kWh"
                                    @input=${(ev: Event) =>
                                      this._patch(index, {
                                        unit: (ev.target as HTMLInputElement).value,
                                      })}
                                  />
                                </label>
                              `
                            : html`
                                <label class="field">
                                  <span>文字</span>
                                  <input
                                    .value=${String(element.text ?? element.name ?? "")}
                                    placeholder="触发"
                                    @input=${(ev: Event) =>
                                      this._patch(index, {
                                        text: (ev.target as HTMLInputElement).value,
                                      })}
                                  />
                                </label>
                                <label class="switch-row">
                                  <span>显示文字</span>
                                  <input
                                    type="checkbox"
                                    .checked=${badgeShowText(element)}
                                    @change=${(ev: Event) =>
                                      this._patch(index, {
                                        show_name: (ev.target as HTMLInputElement).checked,
                                      })}
                                  />
                                </label>
                                <label class="field">
                                  <span>写入值</span>
                                  <input
                                    .value=${element.value === undefined ? "true" : String(element.value)}
                                    placeholder="true"
                                    @input=${(ev: Event) =>
                                      this._patch(index, {
                                        value: parseBadgeWriteValue(
                                          (ev.target as HTMLInputElement).value,
                                        ),
                                      })}
                                  />
                                  <p class="helper">点击时向地址写入该值，默认 true</p>
                                </label>
                              `}
                          <div class="pos">
                            <label class="field">
                              <span>上 (%)</span>
                              <input
                                type="number"
                                min="0"
                                max="100"
                                step="0.5"
                                .value=${String(parsePercent(element.style?.top, 50))}
                                @input=${(ev: Event) =>
                                  this._setPos(index, "top", (ev.target as HTMLInputElement).value)}
                              />
                            </label>
                            <label class="field">
                              <span>左 (%)</span>
                              <input
                                type="number"
                                min="0"
                                max="100"
                                step="0.5"
                                .value=${String(parsePercent(element.style?.left, 50))}
                                @input=${(ev: Event) =>
                                  this._setPos(index, "left", (ev.target as HTMLInputElement).value)}
                              />
                            </label>
                          </div>
                        </div>
                      `
                    : nothing}
                </div>
              `;
            })}
      </div>
    `;
  }

  static styles = css`
    .elements-editor {
      margin-bottom: 12px;
    }
    .head {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 8px;
      margin-bottom: 4px;
    }
    .label {
      font-size: 13px;
      font-weight: 600;
    }
    .add-group {
      display: flex;
      gap: 12px;
    }
    .add {
      border: none;
      background: transparent;
      color: var(--primary-color);
      font-size: 13px;
      font-weight: 600;
      cursor: pointer;
      padding: 4px 0;
    }
    .hint,
    .empty,
    .helper {
      margin: 0 0 8px;
      font-size: 12px;
      color: var(--secondary-text-color);
      line-height: 1.4;
    }
    .row {
      border: 1px solid var(--divider-color);
      border-radius: 10px;
      margin-bottom: 8px;
      background: var(--secondary-background-color, #f8f9fa);
      overflow: hidden;
    }
    .row.open {
      border-color: color-mix(in srgb, var(--primary-color) 45%, var(--divider-color));
    }
    .row-head {
      display: flex;
      align-items: center;
      gap: 8px;
      padding: 8px 10px;
    }
    .meta {
      min-width: 0;
      flex: 1;
      cursor: pointer;
    }
    .meta strong {
      display: block;
      font-size: 14px;
    }
    .meta span {
      display: block;
      font-size: 12px;
      color: var(--secondary-text-color);
      overflow: hidden;
      text-overflow: ellipsis;
      white-space: nowrap;
    }
    .actions {
      display: flex;
      align-items: center;
    }
    .actions button {
      border: none;
      background: transparent;
      color: var(--secondary-text-color);
      cursor: pointer;
      width: 28px;
      height: 28px;
      padding: 0;
      display: inline-flex;
      align-items: center;
      justify-content: center;
    }
    .actions .danger {
      color: var(--error-color, #b91c1c);
    }
    ha-icon {
      --mdc-icon-size: 16px;
    }
    .row-body {
      padding: 0 12px 12px;
    }
    .field {
      display: flex;
      flex-direction: column;
      gap: 6px;
      margin-bottom: 8px;
      font-size: 13px;
      color: var(--secondary-text-color);
    }
    .field input,
    .field select {
      font: inherit;
      border: 1px solid var(--divider-color);
      border-radius: 8px;
      padding: 8px 10px;
      background: #fff;
      color: var(--primary-text-color);
    }
    .switch-row {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 12px;
      margin-bottom: 8px;
      font-size: 13px;
      color: var(--primary-text-color);
      cursor: pointer;
    }
    .pos {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 0 12px;
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "flow-picture-elements-editor": FlowPictureElementsEditor;
  }
}
