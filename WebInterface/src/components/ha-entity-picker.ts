import { css, html, LitElement, nothing } from "lit";
import { customElement, property, state } from "lit/decorators.js";
import { styleMap } from "lit/directives/style-map.js";
import { consume } from "@lit/context";
import type { Flow } from "../types";
import { flowContext } from "../context/flow-context";
import { computeFixedDropdownPosition } from "../common/dom/fixed-dropdown";
import {
  actionDisplayName,
  fetchActions,
  findActionByEntity,
  findActionsByName,
  type FlowAction,
} from "../data/actions";

export interface EntityPickerValueChangedDetail {
  /** OSC 地址（写入布局配置） */
  value: string;
  /** 动作库中的显示名称 */
  actionName?: string;
}

interface EntityOption {
  action: FlowAction;
  label: string;
  hint?: string;
}

@customElement("ha-entity-picker")
export class HaEntityPicker extends LitElement {
  @consume({ context: flowContext, subscribe: true })
  @property({ attribute: false })
  public flow?: Flow;

  @property() public label = "动作";

  /** 绑定的 OSC 地址（entity） */
  @property() public value = "";

  @property() public placeholder = "搜索动作名";

  @property({ type: Boolean }) public optional = false;

  @property({ type: Boolean, reflect: true }) public compact = false;

  @state() private _actions: FlowAction[] = [];

  @state() private _open = false;

  @state() private _menuStyle: Record<string, string> = {};

  @state() private _focused = false;

  @state() private _query = "";

  /** 本次聚焦后用户是否编辑过搜索框 */
  private _editedQuery = false;

  /** 刚从下拉选中，跳过一次 blur 提交 */
  private _suppressNextBlur = false;

  private _scrollParent?: HTMLElement | null;

  private readonly _onActionsChanged = (): void => {
    void this._loadActions();
  };

  connectedCallback(): void {
    super.connectedCallback();
    document.addEventListener("click", this._onDocumentClick);
    document.addEventListener("flow-actions-changed", this._onActionsChanged);
    window.addEventListener("resize", this._repositionMenu);
    void this._loadActions();
  }

  disconnectedCallback(): void {
    document.removeEventListener("click", this._onDocumentClick);
    document.removeEventListener("flow-actions-changed", this._onActionsChanged);
    window.removeEventListener("resize", this._repositionMenu);
    this._detachScrollListener();
    super.disconnectedCallback();
  }

  private async _loadActions(): Promise<void> {
    const actions = await fetchActions();
    this._actions = actions;
    const entities = actions.map((item) => item.entity).filter(Boolean);
    if (entities.length && this.flow) {
      this.flow.connection.trackAddresses(entities);
    }
  }

  private _inputText(): string {
    if (this._focused) return this._query;
    const action = findActionByEntity(this._actions, this.value);
    return action ? actionDisplayName(action) : "";
  }

  private _actionOptions(): EntityOption[] {
    const matches = findActionsByName(this._actions, this._focused ? this._query : "");
    return [...matches]
      .sort((a, b) => {
        if (a.used !== b.used) return a.used ? 1 : -1;
        return actionDisplayName(a).localeCompare(actionDisplayName(b), "zh-CN");
      })
      .map((action) => ({
        action,
        label: actionDisplayName(action),
        hint: action.entity,
      }));
  }

  private _emitSelection(entity: string, actionName?: string): void {
    this.dispatchEvent(
      new CustomEvent<EntityPickerValueChangedDetail>("value-changed", {
        bubbles: true,
        composed: true,
        detail: { value: entity, actionName },
      }),
    );
  }

  private _onDocumentClick = (ev: MouseEvent): void => {
    if (!this._open) return;
    const path = ev.composedPath();
    if (!path.includes(this)) {
      this._close();
    }
  };

  private _repositionMenu = (): void => {
    if (!this._open) return;
    this._updateMenuPosition();
  };

  private _attachScrollListener(): void {
    this._detachScrollListener();
    let node: HTMLElement | null = this.parentElement;
    while (node) {
      const style = getComputedStyle(node);
      if (/(auto|scroll)/.test(style.overflowY)) {
        this._scrollParent = node;
        node.addEventListener("scroll", this._repositionMenu, { passive: true });
        return;
      }
      node = node.parentElement;
    }
  }

  private _detachScrollListener(): void {
    this._scrollParent?.removeEventListener("scroll", this._repositionMenu);
    this._scrollParent = undefined;
  }

  private _updateMenuPosition(): void {
    const trigger = this.shadowRoot?.querySelector(".input-wrap") as HTMLElement | null;
    if (!trigger) return;
    const pos = computeFixedDropdownPosition(trigger);
    this._menuStyle = {
      position: "fixed",
      top: `${pos.top}px`,
      left: `${pos.left}px`,
      width: `${pos.width}px`,
      maxHeight: `${pos.maxHeight}px`,
      zIndex: "500",
    };
  }

  private _openMenu(): void {
    if (this._open) return;
    this._open = true;
    this._attachScrollListener();
    this.requestUpdate();
    requestAnimationFrame(() => this._updateMenuPosition());
  }

  private _close(): void {
    if (!this._open) return;
    this._open = false;
    this._detachScrollListener();
  }

  private _onInput(ev: Event): void {
    this._editedQuery = true;
    this._query = (ev.target as HTMLInputElement).value;
    this._openMenu();
    requestAnimationFrame(() => this._updateMenuPosition());
  }

  private _onFocus(): void {
    void this._loadActions();
    this._focused = true;
    this._editedQuery = false;
    const action = findActionByEntity(this._actions, this.value);
    this._query = action ? actionDisplayName(action) : "";
    this._openMenu();
  }

  private _onBlur(): void {
    window.setTimeout(() => {
      if (this._suppressNextBlur) {
        this._suppressNextBlur = false;
        this._focused = false;
        this._query = "";
        this._close();
        return;
      }

      this._focused = false;
      const trimmed = this._query.trim();
      if (!trimmed) {
        if (this.optional && this._editedQuery) {
          this._emitSelection("");
        }
        this._query = "";
        this._close();
        return;
      }
      const exact = findActionsByName(this._actions, trimmed).find(
        (action) => actionDisplayName(action).toLowerCase() === trimmed.toLowerCase(),
      );
      if (exact && exact.entity !== this.value) {
        this._emitSelection(exact.entity, actionDisplayName(exact));
      }
      this._query = "";
      this._close();
    }, 0);
  }

  private _selectAction(action: FlowAction): void {
    this._suppressNextBlur = true;
    this._focused = false;
    this._editedQuery = false;
    this._query = "";
    this._close();
    this._emitSelection(action.entity, actionDisplayName(action));
  }

  private _renderOption(option: EntityOption) {
    return html`
      <button
        type="button"
        class="option"
        @mousedown=${(ev: Event) => ev.preventDefault()}
        @click=${() => this._selectAction(option.action)}
      >
        <span class="option-value">${option.label}</span>
        ${option.hint ? html`<span class="option-hint">${option.hint}</span>` : nothing}
      </button>
    `;
  }

  protected render() {
    const actionOptions = this._actionOptions();
    const hasMenu = actionOptions.length > 0;

    return html`
      <label class="field ${this.compact ? "compact" : ""}">
        ${this.compact
          ? nothing
          : html`
              <span class="label">
                ${this.label}
                ${this.optional ? html`<span class="optional">（可选）</span>` : ""}
              </span>
            `}
        <div class="input-wrap">
          <input
            type="text"
            .value=${this._inputText()}
            placeholder=${this.placeholder}
            @input=${this._onInput}
            @focus=${this._onFocus}
            @blur=${this._onBlur}
          />
        </div>
      </label>
      ${this._open && hasMenu
        ? html`
            <div class="menu" style=${styleMap(this._menuStyle)}>
              <div class="section">
                <div class="section-title">动作库</div>
                ${actionOptions.map((option) => this._renderOption(option))}
              </div>
            </div>
          `
        : this._open
          ? html`
              <div class="menu" style=${styleMap(this._menuStyle)}>
                <div class="section empty-hint">
                  <p>未找到匹配动作，请先在侧边栏「动作库」中添加</p>
                </div>
              </div>
            `
          : nothing}
    `;
  }

  static styles = css`
    :host {
      display: block;
      position: relative;
    }
    .field {
      display: flex;
      flex-direction: column;
      gap: 6px;
      margin-bottom: 12px;
    }
    .field.compact {
      gap: 0;
      margin-bottom: 0;
    }
    .field.compact input {
      height: 36px;
      padding: 0 10px;
      border-radius: 8px;
    }
    .label {
      font-size: 13px;
      color: var(--secondary-text-color);
    }
    .optional {
      font-weight: 400;
      opacity: 0.8;
    }
    .input-wrap {
      display: block;
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
    .menu {
      overflow-y: auto;
      border: 1px solid var(--divider-color);
      border-radius: 8px;
      background: #fff;
      box-shadow: 0 4px 16px rgba(0, 0, 0, 0.12);
    }
    .section-title {
      padding: 8px 12px 4px;
      font-size: 11px;
      font-weight: 600;
      letter-spacing: 0.02em;
      text-transform: uppercase;
      color: var(--secondary-text-color);
    }
    .empty-hint p {
      margin: 0;
      padding: 10px 12px;
      font-size: 12px;
      color: var(--secondary-text-color);
    }
    .option {
      display: flex;
      flex-direction: column;
      align-items: flex-start;
      gap: 2px;
      width: 100%;
      padding: 8px 12px;
      border: none;
      background: transparent;
      cursor: pointer;
      font: inherit;
      text-align: left;
      color: var(--primary-text-color);
    }
    .option:hover,
    .option:focus-visible {
      background: rgba(var(--rgb-primary-color, 3, 169, 244), 0.08);
    }
    .option-value {
      font-size: 14px;
      line-height: 1.3;
      word-break: break-all;
    }
    .option-hint {
      font-size: 12px;
      color: var(--primary-color);
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "ha-entity-picker": HaEntityPicker;
  }
}
