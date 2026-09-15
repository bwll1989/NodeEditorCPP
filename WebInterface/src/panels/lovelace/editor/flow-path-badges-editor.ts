import { css, html, LitElement } from "lit";
import { customElement, property, state } from "lit/decorators.js";
import type { EntityPickerValueChangedDetail } from "../../../components/ha-entity-picker";
import type { LovelacePathBadgeConfig } from "../../../types";
import { badgeShowText } from "../heading-badges/badge-flags";
import { parseBadgeWriteValue } from "../heading-badges/badge-value";
import "../../../components/ha-entity-picker";
import "../../../components/ha-icon-picker";
import "../../../components/ha-color-picker";
import "../../../components/ha-icon";

@customElement("flow-path-badges-editor")
export class FlowPathBadgesEditor extends LitElement {
  @property({ attribute: false }) public badges: LovelacePathBadgeConfig[] = [];

  @state() private _openIndex: number | null = 0;

  get selectedIndex(): number {
    if (this._openIndex !== null && this.badges[this._openIndex]) return this._openIndex;
    return this.badges.length > 0 ? 0 : -1;
  }

  private _emit(badges: LovelacePathBadgeConfig[]): void {
    this.dispatchEvent(
      new CustomEvent("badges-changed", {
        bubbles: true,
        composed: true,
        detail: { badges },
      }),
    );
  }

  private _add(): void {
    const badges = [
      ...this.badges,
      {
        type: "state-badge",
        entity: "",
        icon: "mdi:map-marker",
        color: "accent",
        show_icon: true,
        show_name: false,
        show_state: false,
      },
    ];
    this._openIndex = badges.length - 1;
    this._emit(badges);
  }

  private _patch(index: number, patch: Partial<LovelacePathBadgeConfig>): void {
    this._emit(this.badges.map((badge, current) => (current === index ? { ...badge, ...patch } : badge)));
  }

  private _remove(index: number): void {
    const badges = this.badges.filter((_, current) => current !== index);
    this._openIndex = badges.length === 0 ? null : Math.min(index, badges.length - 1);
    this._emit(badges);
  }

  private _move(index: number, delta: number): void {
    const next = index + delta;
    if (next < 0 || next >= this.badges.length) return;
    const badges = [...this.badges];
    const [item] = badges.splice(index, 1);
    badges.splice(next, 0, item);
    this._openIndex = next;
    this._emit(badges);
  }

  private _duplicate(index: number): void {
    const current = this.badges[index];
    if (!current) return;
    const badges = [
      ...this.badges.slice(0, index + 1),
      { ...current },
      ...this.badges.slice(index + 1),
    ];
    this._openIndex = index + 1;
    this._emit(badges);
  }

  private _title(badge: LovelacePathBadgeConfig, index: number): string {
    const fallback = String(badge.type ?? "state-badge") === "button" ? "控制徽章" : "状态徽章";
    return String(badge.name || badge.text || badge.entity || `${fallback} ${index + 1}`);
  }

  private _changeType(index: number, type: "state-badge" | "button"): void {
    const current = this.badges[index];
    if (!current) return;
    if (type === "button") {
      this._patch(index, {
        type,
        icon: current.icon || "mdi:gesture-tap-button",
        text: String(current.text ?? current.name ?? "触发"),
        show_name: current.show_name ?? true,
      });
      return;
    }

    this._patch(index, {
      type,
      icon: current.icon || "mdi:map-marker",
      show_icon: current.show_icon ?? true,
      show_name: current.show_name ?? current.show_state ?? false,
    });
  }

  protected render() {
    return html`
      <div class="badges-editor">
        <div class="head">
          <span class="label">轨迹徽章</span>
          <button type="button" class="add" @click=${this._add}>+ 添加徽章</button>
        </div>
        <p class="hint">轨迹元素现在支持状态徽章和控制徽章。它们共用同一条轨迹，但每个徽章都可以绑定各自的实体、显示方式和进度来源。</p>
        ${this.badges.length === 0
          ? html`<p class="empty">还没有徽章。至少添加一个徽章后，轨迹上才会显示运动标记。</p>`
          : this.badges.map((badge, index) => {
              const open = this._openIndex === index;
              const type = badge.type === "button" ? "button" : "state-badge";
              return html`
                <div class="row ${open ? "open" : ""}">
                  <div class="row-head">
                    <div
                      class="meta"
                      @click=${() => {
                        this._openIndex = open ? null : index;
                      }}
                    >
                      <strong>${this._title(badge, index)}</strong>
                      <span>${badge.entity || "未选择地址"}</span>
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
                            .value=${String(badge.entity ?? "")}
                            @value-changed=${(ev: CustomEvent<EntityPickerValueChangedDetail>) =>
                              this._patch(index, { entity: ev.detail.value })}
                          ></ha-entity-picker>
                          <label class="field">
                            <span>名称</span>
                            <input
                              .value=${String(badge.name ?? "")}
                              placeholder="留空则使用实体名"
                              @input=${(ev: Event) =>
                                this._patch(index, { name: (ev.target as HTMLInputElement).value })}
                            />
                          </label>
                          <label class="field">
                            <span>位置属性名</span>
                            <input
                              .value=${String(badge.attribute ?? "")}
                              placeholder="progress"
                              @input=${(ev: Event) =>
                                this._patch(index, { attribute: (ev.target as HTMLInputElement).value })}
                            />
                            <p class="helper">可选：用该属性值计算轨迹位置；留空则使用实体状态。</p>
                          </label>
                          ${type === "state-badge"
                            ? html`
                                <label class="switch-row">
                                  <span>显示图标</span>
                                  <input
                                    type="checkbox"
                                    .checked=${badge.show_icon !== false}
                                    @change=${(ev: Event) =>
                                      this._patch(index, {
                                        show_icon: (ev.target as HTMLInputElement).checked,
                                      })}
                                  />
                                </label>
                                <label class="switch-row">
                                  <span>显示状态值</span>
                                  <input
                                    type="checkbox"
                                    .checked=${badgeShowText(badge)}
                                    @change=${(ev: Event) =>
                                      this._patch(index, {
                                        show_name: (ev.target as HTMLInputElement).checked,
                                        show_state: (ev.target as HTMLInputElement).checked,
                                      })}
                                  />
                                </label>
                                <label class="field">
                                  <span>单位</span>
                                  <input
                                    .value=${String(badge.unit ?? "")}
                                    placeholder="°C"
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
                                    .value=${String(badge.text ?? badge.name ?? "")}
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
                                    .checked=${badgeShowText(badge)}
                                    @change=${(ev: Event) =>
                                      this._patch(index, {
                                        show_name: (ev.target as HTMLInputElement).checked,
                                      })}
                                  />
                                </label>
                                <label class="field">
                                  <span>写入值</span>
                                  <input
                                    .value=${badge.value === undefined ? "true" : String(badge.value)}
                                    placeholder="true"
                                    @input=${(ev: Event) =>
                                      this._patch(index, {
                                        value: parseBadgeWriteValue(
                                          (ev.target as HTMLInputElement).value,
                                        ),
                                      })}
                                  />
                                  <p class="helper">点击轨迹控制徽章时，会向实体写入该值。</p>
                                </label>
                              `}
                          <div class="appearance">
                            <ha-icon-picker
                              label="图标"
                              .value=${String(badge.icon ?? "")}
                              placeholder=${type === "button" ? "mdi:gesture-tap-button" : "mdi:map-marker"}
                              @icon-changed=${(ev: CustomEvent<{ value: string }>) =>
                                this._patch(index, { icon: ev.detail.value })}
                            ></ha-icon-picker>
                            <ha-color-picker
                              label="颜色"
                              .value=${String(badge.color ?? "")}
                              ?includeState=${type === "state-badge"}
                              @color-changed=${(ev: CustomEvent<{ value: string }>) =>
                                this._patch(index, { color: ev.detail.value })}
                            ></ha-color-picker>
                          </div>
                        </div>
                      `
                    : undefined}
                </div>
              `;
            })}
      </div>
    `;
  }

  static styles = css`
    .badges-editor {
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
      white-space: nowrap;
      overflow: hidden;
      text-overflow: ellipsis;
    }
    .actions {
      display: flex;
      align-items: center;
      gap: 4px;
    }
    .actions button {
      border: none;
      background: transparent;
      color: var(--secondary-text-color);
      cursor: pointer;
      padding: 4px;
      border-radius: 6px;
      width: 28px;
      height: 28px;
      display: inline-flex;
      align-items: center;
      justify-content: center;
    }
    .actions button:hover {
      background: color-mix(in srgb, var(--primary-color) 10%, transparent);
      color: var(--primary-text-color);
    }
    .actions button.danger:hover {
      background: color-mix(in srgb, var(--error-color, #d32f2f) 10%, transparent);
      color: var(--error-color, #d32f2f);
    }
    .row-body {
      padding: 0 10px 10px;
    }
    .field {
      display: flex;
      flex-direction: column;
      gap: 6px;
      margin-bottom: 12px;
    }
    .field span {
      font-size: 13px;
      color: var(--secondary-text-color);
    }
    .field input {
      font: inherit;
      border: 1px solid var(--divider-color);
      border-radius: 8px;
      padding: 8px 10px;
      color: var(--primary-text-color);
      background: #fff;
    }
    .field select {
      font: inherit;
      border: 1px solid var(--divider-color);
      border-radius: 8px;
      padding: 8px 10px;
      color: var(--primary-text-color);
      background: #fff;
    }
    .appearance {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 0 16px;
      align-items: start;
    }
    .switch-row {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 12px;
      margin-bottom: 12px;
      cursor: pointer;
    }
    .switch-row span {
      font-size: 13px;
      color: var(--secondary-text-color);
    }
    .switch-row input[type="checkbox"] {
      width: 18px;
      height: 18px;
      accent-color: var(--primary-color);
    }
    @media (max-width: 520px) {
      .appearance {
        grid-template-columns: 1fr;
      }
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "flow-path-badges-editor": FlowPathBadgesEditor;
  }
}
