import { css, html, LitElement, nothing } from "lit";
import { customElement, property, state } from "lit/decorators.js";
import type { LovelaceViewConfig } from "../../../types";
import { createBackdropDismissHandlers } from "../../../common/dom/modal-backdrop";
import "../../../components/ha-icon-picker";

const VALID_PATH_REGEX = /^[a-zA-Z0-9_-]+$/;
const INTEGER_REGEX = /^[0-9]+$/;

/** HA-style ASCII slug for view path suggestions. */
export function slugifyViewPath(title: string): string {
  const slug = title
    .trim()
    .toLowerCase()
    .normalize("NFKD")
    .replace(/[\u0300-\u036f]/g, "")
    .replace(/[^a-z0-9]+/g, "-")
    .replace(/^-+|-+$/g, "");
  if (!slug) return "";
  if (INTEGER_REGEX.test(slug)) return `view-${slug}`;
  return slug;
}

@customElement("flow-view-editor")
export class FlowViewEditor extends LitElement {
  @property({ type: Boolean }) public open = false;

  /** Creating a new view (HA `isNew`) */
  @property({ type: Boolean }) public isNew = false;

  @property({ attribute: false }) public config?: LovelaceViewConfig;

  /** Existing paths used by other views — for uniqueness checks */
  @property({ attribute: false }) public reservedPaths: string[] = [];

  @state() private _title = "";
  @state() private _icon = "mdi:view-dashboard";
  @state() private _path = "";
  @state() private _maxColumns = 4;
  @state() private _densePlacement = false;
  @state() private _pathError = "";
  @state() private _suggestedPath = false;

  private readonly _backdropDismiss = createBackdropDismissHandlers(() => this._close());

  protected updated(changed: import("lit").PropertyValues): void {
    if ((changed.has("open") || changed.has("config")) && this.open) {
      const cfg = this.config;
      this._title = cfg?.title ?? (this.isNew ? "新建视图" : "");
      this._icon = cfg?.icon || "mdi:view-dashboard";
      this._path = cfg?.path ?? "";
      this._maxColumns = Math.max(1, Math.min(10, Number(cfg?.max_columns) || 4));
      this._densePlacement = Boolean(cfg?.dense_section_placement);
      this._pathError = "";
      this._suggestedPath = false;
      if (this.isNew && !this._path) {
        this._path = this._uniquePath(slugifyViewPath(this._title) || "view");
        this._suggestedPath = true;
      }
      this._validatePath(this._path);
    }
  }

  private _uniquePath(base: string): string {
    const reserved = new Set(this.reservedPaths);
    if (!reserved.has(base) && base) return base;
    let n = 2;
    let candidate = `${base || "view"}-${n}`;
    while (reserved.has(candidate)) {
      n += 1;
      candidate = `${base || "view"}-${n}`;
    }
    return candidate;
  }

  private _validatePath(path: string): boolean {
    if (!path) {
      this._pathError = "网址路径不能为空";
      return false;
    }
    if (!VALID_PATH_REGEX.test(path)) {
      this._pathError = "仅允许字母、数字、下划线和连字符";
      return false;
    }
    if (INTEGER_REGEX.test(path)) {
      this._pathError = "不能是纯数字（会与视图索引冲突）";
      return false;
    }
    if (this.reservedPaths.includes(path)) {
      this._pathError = "该路径已被其他视图占用";
      return false;
    }
    this._pathError = "";
    return true;
  }

  private _onTitleInput(ev: Event): void {
    const title = (ev.target as HTMLInputElement).value;
    this._title = title;
    // HA: keep suggesting path from title until the user edits path manually
    if (this.isNew && this._suggestedPath) {
      const slug = slugifyViewPath(title);
      this._path = this._uniquePath(slug || "view");
      this._validatePath(this._path);
    }
  }

  private _onPathInput(ev: Event): void {
    this._suggestedPath = false;
    this._path = (ev.target as HTMLInputElement).value.trim();
    this._validatePath(this._path);
  }

  private _close(): void {
    this.dispatchEvent(new CustomEvent("close", { bubbles: true, composed: true }));
  }

  private _save(): void {
    if (!this._validatePath(this._path)) return;
    const config: LovelaceViewConfig = {
      ...(this.config ?? { sections: [] }),
      title: this._title.trim() || "新建视图",
      icon: this._icon || "mdi:view-dashboard",
      path: this._path,
      max_columns: this._maxColumns,
      dense_section_placement: this._densePlacement || undefined,
      sections: this.config?.sections ?? [],
    };
    if (!config.dense_section_placement) {
      delete config.dense_section_placement;
    }
    this.dispatchEvent(
      new CustomEvent("save", {
        bubbles: true,
        composed: true,
        detail: { config, isNew: this.isNew },
      }),
    );
  }

  protected render() {
    if (!this.open) return nothing;
    const canSave = !this._pathError && Boolean(this._path);

    return html`
      <div
        class="backdrop"
        @pointerdown=${this._backdropDismiss.onBackdropPointerDown}
        @pointerup=${this._backdropDismiss.onBackdropPointerUp}
        @pointercancel=${this._backdropDismiss.onBackdropPointerCancel}
      >
        <div class="dialog" role="dialog" aria-labelledby="view-editor-title">
          <h3 id="view-editor-title">${this.isNew ? "添加视图" : "配置视图"}</h3>
          <p class="hint">设置视图标题、图标与网址路径（与 Home Assistant 一致）。</p>

          <label>
            <span>标题</span>
            <input
              type="text"
              .value=${this._title}
              placeholder="例如：客厅"
              @input=${this._onTitleInput}
            />
          </label>

          <ha-icon-picker
            label="图标"
            .value=${this._icon}
            @icon-changed=${(ev: CustomEvent<{ value: string }>) => {
              ev.stopPropagation();
              this._icon = ev.detail.value || "mdi:view-dashboard";
            }}
          ></ha-icon-picker>

          <label>
            <span>网址</span>
            <input
              type="text"
              .value=${this._path}
              placeholder="home"
              @input=${this._onPathInput}
            />
            ${this._pathError
              ? html`<span class="error">${this._pathError}</span>`
              : html`<span class="helper">该值将成为打开此视图的网址路径的一部分（如 #/${this._path || "…"}）。</span>`}
          </label>

          <label>
            <span>最大列数: ${this._maxColumns}</span>
            <input
              type="range"
              min="1"
              max="10"
              step="1"
              .value=${String(this._maxColumns)}
              @input=${(ev: Event) => {
                this._maxColumns = Number((ev.target as HTMLInputElement).value);
              }}
            />
          </label>

          <label class="checkbox">
            <input
              type="checkbox"
              .checked=${this._densePlacement}
              @change=${(ev: Event) => {
                this._densePlacement = (ev.target as HTMLInputElement).checked;
              }}
            />
            <div>
              <div class="switch-label">紧凑分区排列</div>
              <div class="helper">尽量填满空隙放置分区（HA dense_section_placement）。</div>
            </div>
          </label>

          <div class="actions">
            <button type="button" class="secondary" @click=${this._close}>取消</button>
            <button
              type="button"
              class="primary"
              ?disabled=${!canSave}
              @click=${this._save}
            >
              保存
            </button>
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
      max-height: min(90vh, 720px);
      overflow: auto;
      background: var(--card-background-color, #fff);
      border-radius: 16px;
      padding: 20px;
      box-shadow: 0 20px 50px rgba(0, 0, 0, 0.18);
      box-sizing: border-box;
    }
    h3 {
      margin: 0 0 8px;
      font-size: 18px;
      font-weight: 600;
      color: var(--primary-text-color);
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
      align-items: flex-start;
      gap: 10px;
      color: var(--primary-text-color);
    }
    label.checkbox input {
      margin-top: 3px;
      width: auto;
    }
    .switch-label {
      font-size: 14px;
      font-weight: 500;
      color: var(--primary-text-color);
    }
    input[type="text"] {
      font: inherit;
      font-size: 14px;
      color: var(--primary-text-color);
      border: none;
      border-bottom: 1px solid var(--divider-color);
      border-radius: 4px 4px 0 0;
      background: var(--ha-color-form-background, #f5f5f5);
      padding: 10px 12px;
      outline: none;
    }
    input[type="text"]:focus {
      border-bottom-color: var(--primary-color, #03a9f4);
      border-bottom-width: 2px;
    }
    input[type="range"] {
      width: 100%;
    }
    ha-icon-picker {
      display: block;
      margin-bottom: 12px;
    }
    .helper {
      font-size: 12px;
      line-height: 1.4;
      color: var(--secondary-text-color);
    }
    .error {
      font-size: 12px;
      line-height: 1.4;
      color: var(--error-color, #db4437);
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
      color: var(--primary-text-color);
    }
    button.primary {
      background: var(--primary-color, #03a9f4);
      border-color: var(--primary-color, #03a9f4);
      color: #fff;
    }
    button.primary:disabled {
      opacity: 0.45;
      cursor: not-allowed;
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "flow-view-editor": FlowViewEditor;
  }
}
