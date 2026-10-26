import { css, html, LitElement, nothing, type PropertyValues, type TemplateResult } from "lit";
import { customElement, property, state } from "lit/decorators.js";
import type { EntityPickerValueChangedDetail } from "../../../components/ha-entity-picker";
import type {
  LovelaceHeadingBadgeConfig,
  LovelacePathBadgeConfig,
  LovelacePictureElementConfig,
} from "../../../types";
import { renderBadgeActionFields } from "../heading-badges/badge-action-fields";
import { renderBadgeAppearanceFields } from "../heading-badges/badge-appearance-fields";
import { badgeShowBackground, badgeShowText } from "../heading-badges/badge-flags";
import {
  createButtonBadgeStub,
  createEntityBadgeStub,
  isButtonBadge,
  normalizeTapAction,
  patchBadgeConfig,
  type NormalizedBadgeConfig,
  type RawBadgeConfig,
} from "../heading-badges/badge-model";
import { parsePercent, withPosition } from "../picture-elements/position";
import "../../../components/ha-entity-picker";
import "../../../components/ha-icon-picker";
import "../../../components/ha-color-picker";
import "../../../components/ha-icon";

export type BadgeListMode = "heading" | "picture" | "path";

type BadgeItem =
  | LovelaceHeadingBadgeConfig
  | LovelacePictureElementConfig
  | LovelacePathBadgeConfig;

@customElement("flow-badges-list-editor")
export class FlowBadgesListEditor extends LitElement {
  /** heading=标题行；picture=图像元素；path=轨迹徽章 */
  @property() public mode: BadgeListMode = "heading";

  @property() public label = "徽章";

  @property() public hint = "";

  @property() public emptyText = "可添加状态徽章（只读）或控制徽章（点击操作）";

  @property({ attribute: false }) public badges: BadgeItem[] = [];

  /** 当前选中（高亮），与是否展开表单分开 */
  @state() private _selectedIndex = 0;

  /** 当前展开编辑的表单项 */
  @state() private _openIndex: number | null = 0;

  get selectedIndex(): number {
    if (this._selectedIndex >= 0 && this.badges[this._selectedIndex]) {
      return this._selectedIndex;
    }
    return this.badges.length > 0 ? 0 : -1;
  }

  /** 外部（如预览点击）选中某一项并展开 */
  selectIndex(index: number): void {
    if (index < 0 || index >= this.badges.length) return;
    this._select(index, true);
  }

  protected updated(changed: PropertyValues): void {
    if (!changed.has("badges")) return;
    if (this.badges.length === 0) {
      this._selectedIndex = -1;
      this._openIndex = null;
      return;
    }
    if (this._selectedIndex < 0 || this._selectedIndex >= this.badges.length) {
      this._selectedIndex = Math.min(
        Math.max(0, this._selectedIndex),
        this.badges.length - 1,
      );
      if (this._openIndex !== null) this._openIndex = this._selectedIndex;
    }
  }

  /** 图像预览拖放定位用 */
  applyPicturePosition(left: number, top: number): void {
    if (this.mode !== "picture") return;
    const index = this.selectedIndex;
    if (index < 0) return;
    this._emit(
      this.badges.map((el, i) =>
        i === index ? withPosition(el as LovelacePictureElementConfig, top, left) : el,
      ),
    );
  }

  private _emit(badges: BadgeItem[]): void {
    this.dispatchEvent(
      new CustomEvent("badges-changed", {
        bubbles: true,
        composed: true,
        detail: { badges },
      }),
    );
  }

  private _emitSelection(): void {
    const index = this.selectedIndex;
    this.dispatchEvent(
      new CustomEvent("badge-selected", {
        bubbles: true,
        composed: true,
        detail: { index, mode: this.mode },
      }),
    );
    if (this.mode === "picture") {
      document.dispatchEvent(
        new CustomEvent("flow-picture-badge-highlight", {
          detail: { index },
        }),
      );
    }
  }

  private _select(index: number, open = true): void {
    if (index < 0 || index >= this.badges.length) return;
    this._selectedIndex = index;
    if (open) this._openIndex = index;
    this._emitSelection();
    void this.updateComplete.then(() => this._scrollSelectedIntoView());
  }

  private _scrollSelectedIntoView(): void {
    const chip = this.renderRoot.querySelector(
      `.chip[data-index="${this.selectedIndex}"]`,
    ) as HTMLElement | null;
    chip?.scrollIntoView({ inline: "nearest", block: "nearest", behavior: "smooth" });
  }

  private _entityIcon(type: "entity" | "button"): string {
    if (type === "button") return "mdi:gesture-tap-button";
    if (this.mode === "path") return "mdi:map-marker";
    return "mdi:thermometer";
  }

  private _createStub(type: "entity" | "button"): BadgeItem {
    const extras: Partial<NormalizedBadgeConfig> = {};
    if (this.mode === "picture") {
      extras.style =
        type === "button"
          ? { top: "50%", left: "50%" }
          : { top: "40%", left: "40%" };
    }
    if (this.mode === "path" && type === "entity") {
      return createEntityBadgeStub({
        icon: this._entityIcon("entity"),
        color: "accent",
        show_icon: true,
        show_name: false,
        show_state: false,
        ...extras,
      }) as LovelacePathBadgeConfig;
    }
    return type === "button"
      ? createButtonBadgeStub({ icon: this._entityIcon("button"), ...extras })
      : createEntityBadgeStub({ icon: this._entityIcon("entity"), ...extras });
  }

  private _add(type: "entity" | "button"): void {
    const badges = [...this.badges, this._createStub(type)];
    const index = badges.length - 1;
    this._selectedIndex = index;
    this._openIndex = index;
    this._emit(badges);
    void this.updateComplete.then(() => {
      this._emitSelection();
      this._scrollSelectedIntoView();
    });
  }

  private _remove(index: number): void {
    const badges = this.badges.filter((_, i) => i !== index);
    const next =
      badges.length === 0 ? -1 : Math.min(index, badges.length - 1);
    this._selectedIndex = next;
    this._openIndex = next < 0 ? null : next;
    this._emit(badges);
    void this.updateComplete.then(() => this._emitSelection());
  }

  private _move(index: number, delta: number): void {
    const next = index + delta;
    if (next < 0 || next >= this.badges.length) return;
    const badges = [...this.badges];
    const [item] = badges.splice(index, 1);
    badges.splice(next, 0, item);
    this._selectedIndex = next;
    this._openIndex = next;
    this._emit(badges);
    void this.updateComplete.then(() => this._emitSelection());
  }

  private _duplicate(index: number): void {
    const current = this.badges[index];
    if (!current) return;
    let copy: BadgeItem = { ...current };
    if (this.mode === "picture") {
      const el = current as LovelacePictureElementConfig;
      copy = withPosition(
        { ...el },
        Math.min(90, parsePercent(el.style?.top, 50) + 6),
        Math.min(90, parsePercent(el.style?.left, 50) + 6),
      );
    }
    const badges = [
      ...this.badges.slice(0, index + 1),
      copy,
      ...this.badges.slice(index + 1),
    ];
    this._selectedIndex = index + 1;
    this._openIndex = index + 1;
    this._emit(badges);
    void this.updateComplete.then(() => {
      this._emitSelection();
      this._scrollSelectedIntoView();
    });
  }

  private _patch(index: number, patch: object): void {
    this._emit(
      this.badges.map((badge, i) =>
        i === index ? patchBadgeConfig(badge, patch as Record<string, unknown>) : badge,
      ),
    );
  }

  private _changeType(index: number, type: "entity" | "button"): void {
    const current = this.badges[index];
    if (!current) return;
    const shared = {
      entity: current.entity,
      icon: current.icon || this._entityIcon(type),
      color: current.color,
      size: current.size,
      show_background: current.show_background,
      style: (current as LovelacePictureElementConfig).style,
      attribute: (current as LovelacePathBadgeConfig).attribute,
      name: current.name,
      unit: current.unit,
    };
    if (type === "button") {
      this._patch(
        index,
        createButtonBadgeStub({
          ...shared,
          text: String(current.text ?? current.name ?? "触发"),
          show_name: current.show_name ?? true,
        }),
      );
      return;
    }
    this._patch(
      index,
      createEntityBadgeStub({
        ...shared,
        show_icon: current.show_icon ?? true,
        show_name:
          current.show_name ??
          current.show_state ??
          (this.mode === "path" ? false : true),
      }),
    );
  }

  private _setPos(index: number, axis: "top" | "left", raw: string): void {
    const current = this.badges[index] as LovelacePictureElementConfig | undefined;
    if (!current) return;
    const top = axis === "top" ? parsePercent(raw, 50) : parsePercent(current.style?.top, 50);
    const left =
      axis === "left" ? parsePercent(raw, 50) : parsePercent(current.style?.left, 50);
    this._emit(
      this.badges.map((el, i) =>
        i === index ? withPosition(el as LovelacePictureElementConfig, top, left) : el,
      ),
    );
  }

  private _title(badge: BadgeItem, index: number): string {
    const role = isButtonBadge(badge) ? "控制徽章" : "状态徽章";
    const name = String(badge.name || badge.text || "").trim();
    if (name) return name;
    return `${role} ${index + 1}`;
  }

  private _subtitle(badge: BadgeItem): string {
    const tap = normalizeTapAction(badge as RawBadgeConfig);
    if (isButtonBadge(badge) && tap.action === "open-card") {
      const card = tap.card;
      const cardType = String(card?.type ?? "卡片").trim() || "卡片";
      const cardEntity = String(card?.entity ?? badge.entity ?? "").trim();
      return cardEntity ? `弹出 ${cardType} · ${cardEntity}` : `弹出 ${cardType}`;
    }
    return String(badge.entity || badge.text || badge.name || "未选择地址");
  }

  /** 控制徽章的绑定地址；弹出卡片时标题/图像隐藏，轨迹仅保留位置地址 */
  private _renderBadgeEntityField(badge: BadgeItem, index: number, type: "entity" | "button") {
    const tap = type === "button" ? normalizeTapAction(badge as RawBadgeConfig) : undefined;
    const isOpenCard = tap?.action === "open-card";
    if (isOpenCard && this.mode !== "path") return nothing;

    const entityLabel = isOpenCard ? "轨迹位置地址" : "绑定地址";
    return html`
      <ha-entity-picker
        label=${entityLabel}
        .value=${String(badge.entity ?? "")}
        ?optional=${type === "button"}
        @value-changed=${(ev: CustomEvent<EntityPickerValueChangedDetail>) => {
          const entity = ev.detail.value;
          if (isOpenCard) {
            const card = {
              ...(tap?.card ?? { type: "slider" }),
              type: String(tap?.card?.type ?? "slider"),
              ...(String(tap?.card?.entity ?? "").trim()
                ? {}
                : entity
                  ? { entity }
                  : {}),
            };
            this._patch(index, {
              entity,
              tap_action: { action: "open-card", card },
              action: "open-card",
              card,
            });
            return;
          }
          this._patch(index, { entity });
        }}
      ></ha-entity-picker>
      ${isOpenCard
        ? html`<p class="helper">仅用于徽章在轨迹上的位置，不负责弹出卡片里的读写。</p>`
        : nothing}
    `;
  }

  private _renderCommonFields(badge: BadgeItem, index: number, type: "entity" | "button") {
    return html`
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
      ${type === "entity" ? this._renderBadgeEntityField(badge, index, type) : nothing}
      ${this.mode === "path"
        ? html`
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
                .value=${String((badge as LovelacePathBadgeConfig).attribute ?? "")}
                placeholder="progress"
                @input=${(ev: Event) =>
                  this._patch(index, {
                    attribute: (ev.target as HTMLInputElement).value,
                  })}
              />
              <p class="helper">可选：用该属性值计算轨迹位置；留空则使用实体状态。</p>
            </label>
          `
        : nothing}
      <div class="appearance">
        <ha-icon-picker
          label="图标"
          .value=${String(badge.icon ?? "")}
          placeholder=${this._entityIcon(type)}
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
      </div>
    `;
  }

  private _renderEntityFields(badge: BadgeItem, index: number): TemplateResult {
    const showAppearance = this.mode !== "heading";
    return html`
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
      ${showAppearance
        ? renderBadgeAppearanceFields(badge, (patch) => this._patch(index, patch), {
            textLabel: this.mode === "path" ? "显示状态值" : "显示文字",
          })
        : html`
            <label class="switch-row">
              <span>显示文字</span>
              <input
                type="checkbox"
                .checked=${badgeShowText(badge)}
                @change=${(ev: Event) => {
                  const checked = (ev.target as HTMLInputElement).checked;
                  this._patch(index, { show_name: checked, show_state: checked });
                }}
              />
            </label>
            <label class="switch-row">
              <span>显示底色</span>
              <input
                type="checkbox"
                .checked=${badgeShowBackground(badge)}
                @change=${(ev: Event) =>
                  this._patch(index, {
                    show_background: (ev.target as HTMLInputElement).checked,
                  })}
              />
            </label>
          `}
      ${this.mode !== "heading"
        ? html`
            <label class="field">
              <span>单位</span>
              <input
                .value=${String(badge.unit ?? "")}
                placeholder=${this.mode === "path" ? "°C" : "kWh"}
                @input=${(ev: Event) =>
                  this._patch(index, { unit: (ev.target as HTMLInputElement).value })}
              />
            </label>
          `
        : nothing}
    `;
  }

  private _renderButtonFields(badge: BadgeItem, index: number): TemplateResult {
    const showAppearance = this.mode !== "heading";
    // 标题/图像：先选按下行为；轨迹：位置地址在前（与卡片读写无关）
    return html`
      ${this.mode === "path" ? this._renderBadgeEntityField(badge, index, "button") : nothing}
      ${renderBadgeActionFields(badge, (patch) => this._patch(index, patch))}
      ${this.mode !== "path" ? this._renderBadgeEntityField(badge, index, "button") : nothing}
      <label class="field">
        <span>文字</span>
        <input
          .value=${String(badge.text ?? badge.name ?? "")}
          placeholder="触发"
          @input=${(ev: Event) =>
            this._patch(index, { text: (ev.target as HTMLInputElement).value })}
        />
      </label>
      ${showAppearance
        ? renderBadgeAppearanceFields(badge, (patch) => this._patch(index, patch), {
            textLabel: "显示文字",
          })
        : html`
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
            <label class="switch-row">
              <span>显示底色</span>
              <input
                type="checkbox"
                .checked=${badgeShowBackground(badge)}
                @change=${(ev: Event) =>
                  this._patch(index, {
                    show_background: (ev.target as HTMLInputElement).checked,
                  })}
              />
            </label>
          `}
    `;
  }

  private _renderPicturePosition(badge: BadgeItem, index: number): TemplateResult | typeof nothing {
    if (this.mode !== "picture") return nothing;
    const el = badge as LovelacePictureElementConfig;
    return html`
      <div class="pos">
        <label class="field">
          <span>上 (%)</span>
          <input
            type="number"
            min="0"
            max="100"
            step="0.5"
            .value=${String(parsePercent(el.style?.top, 50))}
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
            .value=${String(parsePercent(el.style?.left, 50))}
            @input=${(ev: Event) =>
              this._setPos(index, "left", (ev.target as HTMLInputElement).value)}
          />
        </label>
      </div>
    `;
  }

  protected render() {
    const selected = this.selectedIndex;
    const badge = selected >= 0 ? this.badges[selected] : undefined;
    const type = badge ? (isButtonBadge(badge) ? "button" : "entity") : "entity";

    return html`
      <div class="badges-editor">
        <div class="head">
          <span class="label">${this.label}</span>
          <div class="add-group">
            <button type="button" class="add" @click=${() => this._add("entity")}>
              + 状态徽章
            </button>
            <button type="button" class="add" @click=${() => this._add("button")}>
              + 控制徽章
            </button>
          </div>
        </div>
        ${this.hint ? html`<p class="hint">${this.hint}</p>` : nothing}
        ${this.badges.length === 0
          ? html`<p class="empty">${this.emptyText}</p>`
          : html`
              <div class="chip-row" role="tablist" aria-label=${this.label}>
                ${this.badges.map((item, index) => {
                  const itemType = isButtonBadge(item) ? "button" : "entity";
                  const active = selected === index;
                  return html`
                    <button
                      type="button"
                      role="tab"
                      class="chip ${active ? "selected" : ""} ${itemType}"
                      data-index=${index}
                      aria-selected=${active ? "true" : "false"}
                      title=${this._subtitle(item)}
                      @click=${() => this._select(index, true)}
                    >
                      <span class="chip-index">${index + 1}</span>
                      <span class="chip-type">${itemType === "button" ? "控制" : "状态"}</span>
                      <span class="chip-name">${this._title(item, index)}</span>
                    </button>
                  `;
                })}
              </div>
              ${badge
                ? html`
                    <div class="detail" data-index=${selected}>
                      <div class="detail-toolbar">
                        <span class="detail-title">
                          #${selected + 1}
                          · ${type === "button" ? "控制徽章" : "状态徽章"}
                        </span>
                        <div class="actions">
                          <button
                            type="button"
                            title="上移"
                            ?disabled=${selected <= 0}
                            @click=${() => this._move(selected, -1)}
                          >
                            ↑
                          </button>
                          <button
                            type="button"
                            title="下移"
                            ?disabled=${selected >= this.badges.length - 1}
                            @click=${() => this._move(selected, 1)}
                          >
                            ↓
                          </button>
                          <button
                            type="button"
                            title="复制"
                            @click=${() => this._duplicate(selected)}
                          >
                            <ha-icon icon="mdi:content-copy"></ha-icon>
                          </button>
                          <button
                            type="button"
                            class="danger"
                            title="删除"
                            @click=${() => this._remove(selected)}
                          >
                            <ha-icon icon="mdi:delete"></ha-icon>
                          </button>
                        </div>
                      </div>
                      ${this._renderCommonFields(badge, selected, type)}
                      ${type === "entity"
                        ? this._renderEntityFields(badge, selected)
                        : this._renderButtonFields(badge, selected)}
                      ${this._renderPicturePosition(badge, selected)}
                    </div>
                  `
                : nothing}
            `}
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
    .add-group {
      display: flex;
      gap: 12px;
      flex-wrap: wrap;
      justify-content: flex-end;
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
    .chip-row {
      display: flex;
      flex-direction: row;
      flex-wrap: nowrap;
      gap: 8px;
      overflow-x: auto;
      padding: 2px 2px 10px;
      margin-bottom: 4px;
      scrollbar-width: thin;
    }
    .chip {
      flex: 0 0 auto;
      display: inline-flex;
      align-items: center;
      gap: 6px;
      max-width: 160px;
      height: 32px;
      padding: 0 10px 0 6px;
      border: 1px solid var(--divider-color);
      border-radius: 999px;
      background: var(--card-background-color, #fff);
      color: var(--primary-text-color);
      cursor: pointer;
      font: inherit;
      font-size: 12px;
      line-height: 1;
    }
    .chip:hover {
      border-color: color-mix(in srgb, var(--primary-color) 45%, var(--divider-color));
    }
    .chip.selected {
      border-color: var(--primary-color);
      background: color-mix(in srgb, var(--primary-color) 12%, transparent);
      box-shadow: 0 0 0 1px color-mix(in srgb, var(--primary-color) 30%, transparent);
    }
    .chip-index {
      width: 18px;
      height: 18px;
      border-radius: 50%;
      display: inline-flex;
      align-items: center;
      justify-content: center;
      font-size: 10px;
      font-weight: 700;
      color: var(--secondary-text-color);
      background: color-mix(in srgb, var(--primary-text-color) 8%, transparent);
    }
    .chip.selected .chip-index {
      color: #fff;
      background: var(--primary-color);
    }
    .chip-type {
      flex-shrink: 0;
      font-size: 10px;
      font-weight: 600;
      padding: 1px 5px;
      border-radius: 999px;
    }
    .chip.entity .chip-type {
      color: #0369a1;
      background: color-mix(in srgb, #0ea5e9 16%, transparent);
    }
    .chip.button .chip-type {
      color: #a16207;
      background: color-mix(in srgb, #eab308 18%, transparent);
    }
    .chip-name {
      min-width: 0;
      overflow: hidden;
      text-overflow: ellipsis;
      white-space: nowrap;
      font-weight: 600;
    }
    .detail {
      border: 1px solid var(--divider-color);
      border-radius: 10px;
      padding: 10px 12px 12px;
      background: var(--secondary-background-color, #f8f9fa);
    }
    .detail-toolbar {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 8px;
      margin-bottom: 10px;
    }
    .detail-title {
      font-size: 13px;
      font-weight: 600;
      color: var(--primary-text-color);
    }
    .actions {
      display: flex;
      align-items: center;
      gap: 2px;
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
    .actions button:hover:not(:disabled) {
      background: color-mix(in srgb, var(--primary-color) 10%, transparent);
      color: var(--primary-text-color);
    }
    .actions button:disabled {
      opacity: 0.35;
      cursor: default;
    }
    .actions button.danger {
      color: var(--error-color, #b91c1c);
    }
    .actions button.danger:hover {
      background: color-mix(in srgb, var(--error-color, #d32f2f) 10%, transparent);
      color: var(--error-color, #d32f2f);
    }
    ha-icon {
      --mdc-icon-size: 16px;
    }
    .field {
      display: flex;
      flex-direction: column;
      gap: 6px;
      margin-bottom: 10px;
      font-size: 13px;
      color: var(--secondary-text-color);
    }
    .field span {
      font-size: 13px;
      color: var(--secondary-text-color);
    }
    .field input,
    .field select {
      font: inherit;
      border: 1px solid var(--divider-color);
      border-radius: 8px;
      padding: 8px 10px;
      color: var(--primary-text-color);
      background: #fff;
    }
    .open-card-editor {
      margin-top: 4px;
      padding: 10px;
      border: 1px dashed var(--divider-color);
      border-radius: 10px;
      background: var(--card-background-color, #fff);
    }
    .appearance {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 0 16px;
      align-items: start;
      margin-bottom: 4px;
    }
    .switch-row {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 12px;
      margin-bottom: 10px;
      font-size: 13px;
      color: var(--primary-text-color);
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
    .pos {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 0 12px;
    }
    @media (max-width: 520px) {
      .appearance,
      .pos {
        grid-template-columns: 1fr;
      }
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "flow-badges-list-editor": FlowBadgesListEditor;
  }
}
