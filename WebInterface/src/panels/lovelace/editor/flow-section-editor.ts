import { css, html, LitElement, nothing } from "lit";
import { customElement, property, state } from "lit/decorators.js";
import type {
  LovelaceSectionBackgroundConfig,
  LovelaceSectionConfig,
} from "../../../types";
import { DEFAULT_SECTION_BACKGROUND_OPACITY } from "../../../data/section-config";
import { createBackdropDismissHandlers } from "../../../common/dom/modal-backdrop";
import "../../../components/ha-color-picker";

@customElement("flow-section-editor")
export class FlowSectionEditor extends LitElement {
  @property({ type: Boolean }) public open = false;

  @property({ attribute: false }) public config?: LovelaceSectionConfig;

  @property({ type: Number }) public maxColumns = 4;

  @state() private _columnSpan = 1;
  @state() private _backgroundEnabled = false;
  @state() private _backgroundColor = "default";
  @state() private _backgroundOpacity = DEFAULT_SECTION_BACKGROUND_OPACITY;

  private readonly _backdropDismiss = createBackdropDismissHandlers(() => this._close());

  protected updated(changed: import("lit").PropertyValues): void {
    if (changed.has("config") && this.config) {
      this._columnSpan = this.config.column_span ?? 1;
      const bg = this.config.background;
      this._backgroundEnabled = bg !== undefined;
      if (typeof bg === "object" && bg) {
        this._backgroundColor = bg.color ?? "default";
        this._backgroundOpacity = bg.opacity ?? DEFAULT_SECTION_BACKGROUND_OPACITY;
      } else {
        this._backgroundColor = "default";
        this._backgroundOpacity = DEFAULT_SECTION_BACKGROUND_OPACITY;
      }
    }
  }

  private _pickerColor(): string {
    return this._backgroundColor === "default" ? "" : this._backgroundColor;
  }

  private _close(): void {
    this.dispatchEvent(new CustomEvent("close", { bubbles: true, composed: true }));
  }

  private _save(): void {
    const patch: Partial<LovelaceSectionConfig> = {
      column_span: this._columnSpan,
    };

    if (this._backgroundEnabled) {
      const bg: LovelaceSectionBackgroundConfig = {
        opacity: this._backgroundOpacity,
      };
      if (this._backgroundColor !== "default") {
        bg.color = this._backgroundColor;
      }
      patch.background = bg;
    } else {
      patch.background = undefined;
    }

    this.dispatchEvent(
      new CustomEvent("save", {
        bubbles: true,
        composed: true,
        detail: { patch },
      }),
    );
  }

  protected render() {
    if (!this.open) return nothing;

    return html`
      <div
        class="backdrop"
        @pointerdown=${this._backdropDismiss.onBackdropPointerDown}
        @pointerup=${this._backdropDismiss.onBackdropPointerUp}
        @pointercancel=${this._backdropDismiss.onBackdropPointerCancel}
      >
        <div class="dialog">
          <h3>分区设置</h3>
          <p class="hint">标题请在分区内的「Title」卡片中编辑，与 Home Assistant 一致。</p>

          <label>
            <span>宽度（列数）: ${this._columnSpan}</span>
            <input
              type="range"
              min="1"
              max=${this.maxColumns}
              step="1"
              .value=${String(this._columnSpan)}
              @input=${(ev: Event) => {
                this._columnSpan = Number((ev.target as HTMLInputElement).value);
              }}
            />
          </label>

          <label class="checkbox">
            <input
              type="checkbox"
              .checked=${this._backgroundEnabled}
              @change=${(ev: Event) => {
                this._backgroundEnabled = (ev.target as HTMLInputElement).checked;
              }}
            />
            <span>启用背景色</span>
          </label>

          ${this._backgroundEnabled
            ? html`
                <ha-color-picker
                  label="背景颜色"
                  .value=${this._pickerColor()}
                  ?includeState=${false}
                  ?includeDefault=${true}
                  defaultLabel="默认"
                  ?hideHelper=${true}
                  @color-changed=${(ev: CustomEvent<{ value: string }>) => {
                    const value = ev.detail.value;
                    this._backgroundColor = value ? value : "default";
                  }}
                ></ha-color-picker>
                <label>
                  <span>背景不透明度: ${this._backgroundOpacity}%</span>
                  <input
                    type="range"
                    min="0"
                    max="100"
                    step="1"
                    .value=${String(this._backgroundOpacity)}
                    @input=${(ev: Event) => {
                      this._backgroundOpacity = Number((ev.target as HTMLInputElement).value);
                    }}
                  />
                </label>
              `
            : nothing}
          <div class="actions">
            <button type="button" class="secondary" @click=${this._close}>取消</button>
            <button type="button" class="primary" @click=${this._save}>保存</button>
          </div>
        </div>
      </div>
    `;
  }

  static styles = css`
    .backdrop {
      position: fixed;
      inset: 0;
      background: rgba(15, 23, 42, 0.45);
      display: flex;
      align-items: center;
      justify-content: center;
      z-index: 200;
      padding: 16px;
      box-sizing: border-box;
    }
    .dialog {
      width: min(440px, 100%);
      background: var(--card-background-color);
      border-radius: 16px;
      padding: 20px;
      box-shadow: 0 20px 50px rgba(0, 0, 0, 0.18);
    }
    h3 {
      margin: 0 0 8px;
      font-size: 18px;
      font-weight: 600;
    }
    .hint {
      margin: 0 0 16px;
      font-size: 13px;
      color: var(--secondary-text-color);
      line-height: 1.45;
    }
    label {
      display: flex;
      flex-direction: column;
      gap: 6px;
      margin-bottom: 12px;
      font-size: 13px;
      color: var(--secondary-text-color);
    }
    label.checkbox {
      flex-direction: row;
      align-items: center;
      gap: 8px;
      color: var(--primary-text-color);
    }
    ha-color-picker {
      display: block;
      margin-bottom: 12px;
    }
    .actions {
      display: flex;
      justify-content: flex-end;
      gap: 8px;
      margin-top: 8px;
    }
    button {
      border-radius: 8px;
      padding: 8px 14px;
      font-weight: 600;
      cursor: pointer;
      border: 1px solid var(--divider-color);
      background: #fff;
    }
    button.primary {
      background: var(--primary-color);
      border-color: var(--primary-color);
      color: #fff;
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "flow-section-editor": FlowSectionEditor;
  }
}
