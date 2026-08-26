import { css, html, LitElement } from "lit";
import { customElement, property } from "lit/decorators.js";
import type { EntityPickerValueChangedDetail } from "../../../components/ha-entity-picker";
import type { LovelaceHeadingBadgeConfig } from "../../../types";
import { parseBadgeWriteValue } from "../heading-badges/badge-value";
import { badgeShowText } from "../heading-badges/badge-flags";
import "../../../components/ha-entity-picker";
import "../../../components/ha-icon-picker";
import "../../../components/ha-color-picker";

@customElement("flow-heading-badges-editor")
export class FlowHeadingBadgesEditor extends LitElement {
  @property({ attribute: false }) public badges: LovelaceHeadingBadgeConfig[] = [];

  private _emit(badges: LovelaceHeadingBadgeConfig[]): void {
    this.dispatchEvent(
      new CustomEvent("badges-changed", {
        bubbles: true,
        composed: true,
        detail: { badges },
      }),
    );
  }

  private _add(type: "entity" | "button"): void {
    const next: LovelaceHeadingBadgeConfig =
      type === "button"
        ? { type: "button", entity: "", icon: "mdi:gesture-tap-button", text: "触发", value: true, show_name: true }
        : { type: "entity", entity: "", icon: "mdi:thermometer", show_icon: true, show_name: true };
    this._emit([...this.badges, next]);
  }

  private _remove(index: number): void {
    this._emit(this.badges.filter((_, i) => i !== index));
  }

  private _patch(index: number, patch: Partial<LovelaceHeadingBadgeConfig>): void {
    const badges = this.badges.map((badge, i) => (i === index ? { ...badge, ...patch } : badge));
    this._emit(badges);
  }

  private _changeType(index: number, type: "entity" | "button"): void {
    const current = this.badges[index];
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
      type: "entity",
      show_icon: current?.show_icon ?? true,
      show_name: current?.show_name ?? current?.show_state ?? true,
      icon: current?.icon || "mdi:thermometer",
    });
  }

  protected render() {
    return html`
      <div class="badges-editor">
        <div class="head">
          <span class="label">徽章</span>
          <div class="add-group">
            <button type="button" class="add" @click=${() => this._add("entity")}>+ 状态徽章</button>
            <button type="button" class="add" @click=${() => this._add("button")}>+ 控制徽章</button>
          </div>
        </div>
        ${this.badges.length === 0
          ? html`<p class="empty">可添加状态徽章（只读）或控制徽章（点击写值）</p>`
          : this.badges.map((badge, index) => {
              const type = badge.type === "button" ? "button" : "entity";
              return html`
                <div class="badge-row">
                  <label class="field">
                    <span>类型</span>
                    <select
                      @change=${(ev: Event) =>
                        this._changeType(
                          index,
                          (ev.target as HTMLSelectElement).value as "entity" | "button",
                        )}
                    >
                      <option value="entity" ?selected=${type === "entity"}>状态徽章</option>
                      <option value="button" ?selected=${type === "button"}>控制徽章</option>
                    </select>
                  </label>
                  <ha-entity-picker
                    label="动作"
                    .value=${String(badge.entity ?? "")}
                    ?optional=${type === "button"}
                    @value-changed=${(ev: CustomEvent<EntityPickerValueChangedDetail>) =>
                      this._patch(index, { entity: ev.detail.value })}
                  ></ha-entity-picker>
                  <ha-icon-picker
                    label="图标"
                    .value=${String(badge.icon ?? "")}
                    placeholder=${type === "button" ? "mdi:gesture-tap-button" : "mdi:thermometer"}
                    @icon-changed=${(ev: CustomEvent<{ value: string }>) =>
                      this._patch(index, { icon: ev.detail.value })}
                  ></ha-icon-picker>
                  <ha-color-picker
                    label="颜色"
                    .value=${String(badge.color ?? "")}
                    ?includeState=${type === "entity"}
                    @color-changed=${(ev: CustomEvent<{ value: string }>) =>
                      this._patch(index, { color: ev.detail.value })}
                  ></ha-color-picker>
                  ${type === "entity"
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
                          <span>显示文字</span>
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
                      `
                    : html`
                        <label class="field">
                          <span>文字</span>
                          <input
                            .value=${String(badge.text ?? "")}
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
                          <p class="helper">点击时向地址写入该值，默认 true</p>
                        </label>
                      `}
                  <button type="button" class="remove" @click=${() => this._remove(index)}>
                    删除
                  </button>
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
      margin-bottom: 8px;
      gap: 8px;
    }
    .label {
      font-size: 13px;
      font-weight: 600;
      color: var(--primary-text-color);
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
    .empty {
      margin: 0 0 8px;
      font-size: 12px;
      color: var(--secondary-text-color);
    }
    .badge-row {
      border: 1px solid var(--divider-color);
      border-radius: 10px;
      padding: 12px;
      margin-bottom: 8px;
      background: var(--secondary-background-color, #f8f9fa);
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
    .helper {
      margin: 0;
      font-size: 12px;
      color: var(--secondary-text-color);
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
    .remove {
      border: none;
      background: transparent;
      color: var(--error-color, #b91c1c);
      font-size: 13px;
      cursor: pointer;
      padding: 0;
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "flow-heading-badges-editor": FlowHeadingBadgesEditor;
  }
}
