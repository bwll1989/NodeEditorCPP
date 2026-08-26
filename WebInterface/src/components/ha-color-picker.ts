import { css, html, LitElement, nothing } from "lit";
import { customElement, property, state } from "lit/decorators.js";
import { styleMap } from "lit/directives/style-map.js";
import {
  computeCssColor,
  THEME_COLOR_OPTIONS,
  themeColorLabel,
} from "../common/color/theme-colors";
import { computeFixedDropdownPosition } from "../common/dom/fixed-dropdown";
import { resolveIconPath } from "../common/icons/mdi-registry";
import "./ha-icon";
import "./ha-picker-field";

const STATE_COLOR = { value: "", label: "状态颜色" };

@customElement("ha-color-picker")
export class HaColorPicker extends LitElement {
  @property() public label = "颜色";

  /** Theme token, hex, or empty for state color */
  @property() public value = "";

  @property({ type: Boolean }) public compact = false;

  @property({ type: Boolean }) public hideHelper = false;

  /** Show "状态颜色" as the empty-value option (tile cards) */
  @property({ type: Boolean }) public includeState = true;

  /** Show a default / theme-background option instead of state color */
  @property({ type: Boolean }) public includeDefault = false;

  @property() public defaultLabel = "默认";

  @property() public helper = "";

  @state() private _open = false;

  @state() private _custom = "#03a9f4";

  @state() private _showCustom = false;

  @state() private _menuStyle: Record<string, string> = {};

  private _scrollParent?: HTMLElement | null;

  connectedCallback(): void {
    super.connectedCallback();
    document.addEventListener("click", this._onDocumentClick);
    window.addEventListener("resize", this._repositionMenu);
    this._syncCustom();
  }

  disconnectedCallback(): void {
    document.removeEventListener("click", this._onDocumentClick);
    window.removeEventListener("resize", this._repositionMenu);
    this._detachScrollListener();
    super.disconnectedCallback();
  }

  updated(changed: Map<string, unknown>): void {
    if (changed.has("value")) {
      this._syncCustom();
    }
  }

  private _syncCustom(): void {
    if (this.value && !THEME_COLOR_OPTIONS.some((c) => c.value === this.value) && this.value !== "") {
      this._custom = this.value;
    }
  }

  private _onDocumentClick = (ev: MouseEvent): void => {
    if (!this._open) return;
    if (!ev.composedPath().includes(this)) {
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

  private _displayLabel(): string {
    if (!this.value) {
      return this.includeDefault ? this.defaultLabel : STATE_COLOR.label;
    }
    return themeColorLabel(this.value);
  }

  private _emptySelected(): boolean {
    return !this.value;
  }

  private _showPrimaryOption(): boolean {
    return this.includeState || this.includeDefault;
  }

  private _primaryOptionLabel(): string {
    return this.includeDefault ? this.defaultLabel : STATE_COLOR.label;
  }

  private _displayColor(): string {
    if (!this.value) return "var(--primary-color, #03a9f4)";
    return computeCssColor(this.value);
  }

  private _emit(value: string): void {
    this.dispatchEvent(
      new CustomEvent("color-changed", {
        bubbles: true,
        composed: true,
        detail: { value },
      }),
    );
  }

  private _select(value: string): void {
    this._showCustom = false;
    this._close();
    this._emit(value);
  }

  private _applyCustom(): void {
    this._close();
    this._emit(this._custom);
  }

  private _toggleOpen(ev: Event): void {
    ev.stopPropagation();
    if (this._open) {
      this._close();
      return;
    }
    this._open = true;
    this._showCustom = false;
    this._attachScrollListener();
    requestAnimationFrame(() => this._updateMenuPosition());
  }

  private _close(): void {
    this._open = false;
    this._detachScrollListener();
  }

  private _renderSwatch(color: string, state = false): unknown {
    if (state) {
      return html`<span class="swatch state"></span>`;
    }
    return html`<span class="swatch" style=${`background:${color}`}></span>`;
  }

  protected render() {
    const palettePath = resolveIconPath("mdi:palette");
    const hasValue = Boolean(this.value);
    const displayColor = this._displayColor();
    const showFieldValue = hasValue || this.compact || (this.includeDefault && !this.value);

    return html`
      <div class="picker ${this.compact ? "compact" : ""}">
        <ha-picker-field
          .label=${this.label}
          .hasValue=${showFieldValue}
          .opened=${this._open}
          .compact=${this.compact}
          @click=${this._toggleOpen}
        >
          ${hasValue && !THEME_COLOR_OPTIONS.some((c) => c.value === this.value)
            ? html`<span slot="leading" class="swatch leading" style=${`background:${displayColor}`}></span>`
            : html`<ha-svg-icon slot="leading" .path=${palettePath}></ha-svg-icon>`}
          <span slot="value">${this._displayLabel()}</span>
        </ha-picker-field>

        ${this._open
          ? html`
              <div class="menu" style=${styleMap(this._menuStyle)} @click=${(ev: Event) => ev.stopPropagation()}>
                ${this._showPrimaryOption()
                  ? html`
                      <button
                        type="button"
                        class="option ${this._emptySelected() ? "selected" : ""}"
                        @click=${() => this._select("")}
                      >
                        ${this.includeDefault
                          ? html`<span class="swatch default"></span>`
                          : this._renderSwatch("var(--primary-color)", true)}
                        <span>${this._primaryOptionLabel()}</span>
                      </button>
                    `
                  : nothing}
                ${THEME_COLOR_OPTIONS.map(
                  (opt) => html`
                    <button
                      type="button"
                      class="option ${this.value === opt.value ? "selected" : ""}"
                      @click=${() => this._select(opt.value)}
                    >
                      ${this._renderSwatch(computeCssColor(opt.value))}
                      <span>${opt.label}</span>
                    </button>
                  `,
                )}
                <div class="custom-section">
                  <button
                    type="button"
                    class="option custom-toggle"
                    @click=${() => {
                      this._showCustom = !this._showCustom;
                    }}
                  >
                    <ha-svg-icon class="rainbow" .path=${resolveIconPath("mdi:palette-swatch")}></ha-svg-icon>
                    <span>自定义颜色</span>
                  </button>
                  ${this._showCustom
                    ? html`
                        <div class="custom-row">
                          <input
                            type="color"
                            .value=${this._custom}
                            @input=${(ev: Event) => {
                              this._custom = (ev.target as HTMLInputElement).value;
                            }}
                          />
                          <input
                            type="text"
                            class="hex"
                            .value=${this._custom}
                            @input=${(ev: Event) => {
                              this._custom = (ev.target as HTMLInputElement).value;
                            }}
                          />
                          <button type="button" class="apply" @click=${this._applyCustom}>确定</button>
                        </div>
                      `
                    : nothing}
                </div>
              </div>
            `
          : nothing}

        ${this.hideHelper
          ? nothing
          : html`<p class="helper">${this.helper || "非活动状态（例如关闭或闭合）将不会被着色。"}</p>`}
      </div>
    `;
  }

  static styles = css`
    :host {
      display: block;
      min-width: 0;
    }
    .picker {
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
      color: var(--secondary-text-color);
    }
    .swatch {
      width: 20px;
      height: 20px;
      border-radius: 50%;
      flex-shrink: 0;
      border: 1px solid rgba(0, 0, 0, 0.08);
      box-sizing: border-box;
      display: block;
    }
    .swatch.leading {
      width: 20px;
      height: 20px;
    }
    .swatch.state {
      background: linear-gradient(135deg, #bdbdbd 0%, var(--primary-color, #03a9f4) 100%);
    }
    .swatch.default {
      background: var(--ha-section-background-color, var(--secondary-background-color, #e5e5e5));
    }
    .menu {
      background: var(--card-background-color, #fff);
      border-radius: 12px;
      box-shadow: 0 8px 28px rgba(0, 0, 0, 0.16);
      border: 1px solid var(--divider-color);
      overflow: auto;
      padding: 6px;
    }
    .option {
      width: 100%;
      display: flex;
      align-items: center;
      gap: 10px;
      border: none;
      background: transparent;
      padding: 10px 12px;
      border-radius: 8px;
      cursor: pointer;
      font: inherit;
      text-align: left;
      color: var(--primary-text-color);
    }
    .option:hover,
    .option.selected {
      background: rgba(3, 169, 244, 0.08);
    }
    .rainbow {
      width: 22px;
      height: 22px;
      color: var(--secondary-text-color);
    }
    .custom-section {
      border-top: 1px solid var(--divider-color);
      margin-top: 4px;
      padding-top: 4px;
    }
    .custom-row {
      display: flex;
      align-items: center;
      gap: 8px;
      padding: 4px 6px 6px;
    }
    .custom-row input[type="color"] {
      width: 36px;
      height: 36px;
      border: none;
      padding: 0;
      background: transparent;
      cursor: pointer;
    }
    .hex {
      flex: 1;
      min-width: 0;
      font: inherit;
      border: 1px solid var(--divider-color);
      border-radius: 8px;
      padding: 6px 8px;
    }
    .apply {
      border: none;
      background: var(--primary-color);
      color: #fff;
      border-radius: 8px;
      padding: 6px 10px;
      font-size: 13px;
      cursor: pointer;
      flex-shrink: 0;
    }
    .helper {
      margin: 6px 0 0;
      font-size: 12px;
      line-height: 1.4;
      color: var(--secondary-text-color);
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "ha-color-picker": HaColorPicker;
  }
}
