import { css, html, LitElement, nothing } from "lit";
import { customElement, property, state } from "lit/decorators.js";
import { styleMap } from "lit/directives/style-map.js";
import { computeFixedDropdownPosition } from "../common/dom/fixed-dropdown";
import {
  ensureMdiRegistry,
  onMdiRegistryLoaded,
  resolveIconPath,
  searchMdiIcons,
} from "../common/icons/mdi-registry";
import "./ha-icon";
import "./ha-picker-field";

@customElement("ha-icon-picker")
export class HaIconPicker extends LitElement {
  @property() public label = "图标";

  @property() public value = "";

  @property() public placeholder = "mdi:home";

  @property({ type: Boolean }) public compact = false;

  @state() private _open = false;

  @state() private _filter = "";

  @state() private _results: string[] = [];

  @state() private _registryReady = false;

  @state() private _menuStyle: Record<string, string> = {};

  private _unloadRegistryListener?: () => void;

  private _scrollParent?: HTMLElement | null;

  connectedCallback(): void {
    super.connectedCallback();
    document.addEventListener("click", this._onDocumentClick);
    window.addEventListener("resize", this._repositionMenu);
    this._results = searchMdiIcons("");
    this._unloadRegistryListener = onMdiRegistryLoaded(() => {
      this._registryReady = true;
      this._results = searchMdiIcons(this._filter);
      this.requestUpdate();
    });
    void ensureMdiRegistry();
  }

  disconnectedCallback(): void {
    document.removeEventListener("click", this._onDocumentClick);
    window.removeEventListener("resize", this._repositionMenu);
    this._detachScrollListener();
    this._unloadRegistryListener?.();
    super.disconnectedCallback();
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
    const trigger = this.shadowRoot?.querySelector("ha-picker-field");
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

  private _toggleOpen(ev: Event): void {
    ev.stopPropagation();
    if (this._open) {
      this._close();
      return;
    }
    void ensureMdiRegistry().then(() => {
      this._filter = "";
      this._results = searchMdiIcons("");
      this._open = true;
      this._attachScrollListener();
      this.requestUpdate();
      requestAnimationFrame(() => {
        this._updateMenuPosition();
        this.shadowRoot?.querySelector<HTMLInputElement>(".search-input")?.focus();
      });
    });
  }

  private _close(): void {
    this._open = false;
    this._detachScrollListener();
  }

  private _onFilterInput(ev: Event): void {
    const value = (ev.target as HTMLInputElement).value;
    this._filter = value;
    this._results = searchMdiIcons(value);
  }

  private _select(name: string, ev: Event): void {
    ev.stopPropagation();
    this._emit(name);
    this._close();
    this._filter = "";
  }

  private _emit(value: string): void {
    this.dispatchEvent(
      new CustomEvent("icon-changed", {
        bubbles: true,
        composed: true,
        detail: { value },
      }),
    );
  }

  protected render() {
    const displayIcon = this.value || this.placeholder;
    const path = resolveIconPath(displayIcon);
    const hasValue = Boolean(this.value);

    return html`
      <div class="picker ${this.compact ? "compact" : ""}">
        <ha-picker-field
          .label=${this.label}
          .hasValue=${hasValue}
          .opened=${this._open}
          .compact=${this.compact}
          @click=${this._toggleOpen}
        >
          <ha-svg-icon slot="leading" .path=${path}></ha-svg-icon>
          <span slot="value">${this.value || this.placeholder}</span>
        </ha-picker-field>
        ${this._open
          ? html`
              <div class="menu" style=${styleMap(this._menuStyle)} @click=${(ev: Event) => ev.stopPropagation()}>
                <div class="search">
                  <ha-svg-icon class="search-icon" .path=${resolveIconPath("mdi:magnify")}></ha-svg-icon>
                  <input
                    class="search-input"
                    placeholder="搜索 | 添加自定义项目"
                    .value=${this._filter}
                    @input=${this._onFilterInput}
                  />
                </div>
                <ul class="list" role="listbox">
                  ${!this._registryReady
                    ? html`<li class="empty">正在加载图标库…</li>`
                    : this._results.length
                    ? this._results.map(
                        (name) => html`
                          <li>
                            <button
                              type="button"
                              class="option ${name === this.value ? "selected" : ""}"
                              @click=${(ev: Event) => this._select(name, ev)}
                            >
                              <ha-svg-icon .path=${resolveIconPath(name)}></ha-svg-icon>
                              <span>${name}</span>
                            </button>
                          </li>
                        `,
                      )
                    : html`<li class="empty">未找到匹配的图标</li>`}
                </ul>
              </div>
            `
          : nothing}
      </div>
    `;
  }

  static styles = css`
    :host {
      display: block;
      min-width: 0;
    }
    .picker {
      position: relative;
      margin-bottom: 12px;
    }
    .picker.compact {
      margin-bottom: 0;
    }
    ha-picker-field {
      width: 100%;
    }
    ha-picker-field ha-svg-icon {
      width: 24px;
      height: 24px;
    }
    .menu {
      background: var(--card-background-color, #fff);
      border-radius: 12px;
      box-shadow: 0 8px 28px rgba(0, 0, 0, 0.16);
      border: 1px solid var(--divider-color);
      overflow: hidden;
      display: flex;
      flex-direction: column;
    }
    .search {
      display: flex;
      align-items: center;
      gap: 8px;
      padding: 10px 12px;
      border-bottom: 1px solid var(--divider-color);
      flex-shrink: 0;
    }
    .search-icon {
      width: 20px;
      height: 20px;
      color: var(--secondary-text-color);
      flex-shrink: 0;
    }
    .search-input {
      flex: 1;
      border: none;
      outline: none;
      font: inherit;
      font-size: 14px;
      color: var(--primary-text-color);
      background: transparent;
      min-width: 0;
    }
    .list {
      list-style: none;
      margin: 0;
      padding: 4px 0;
      overflow-y: auto;
      flex: 1;
      min-height: 0;
    }
    .option {
      width: 100%;
      display: flex;
      align-items: center;
      gap: 12px;
      border: none;
      background: transparent;
      padding: 10px 14px;
      cursor: pointer;
      text-align: left;
      font: inherit;
      font-size: 14px;
      color: var(--primary-text-color);
    }
    .option:hover,
    .option.selected {
      background: rgba(3, 169, 244, 0.08);
    }
    .option ha-svg-icon {
      width: 22px;
      height: 22px;
      flex-shrink: 0;
    }
    .option span {
      overflow: hidden;
      text-overflow: ellipsis;
      white-space: nowrap;
    }
    .empty {
      padding: 16px;
      text-align: center;
      color: var(--secondary-text-color);
      font-size: 13px;
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "ha-icon-picker": HaIconPicker;
  }
}
