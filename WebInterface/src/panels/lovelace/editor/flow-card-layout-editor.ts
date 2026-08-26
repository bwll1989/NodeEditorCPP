import { css, html, LitElement, nothing } from "lit";
import { customElement, property, state } from "lit/decorators.js";
import type {
  LovelaceCardConfig,
  LovelaceGridOptions,
  LovelaceSectionConfig,
} from "../../../types";
import {
  computeCardGridSize,
  DEFAULT_GRID_SIZE,
  GRID_COLUMN_MULTIPLIER,
  isPreciseMode,
  migrateLayoutToGridOptions,
  type CardGridSize,
} from "../common/compute-card-grid-size";
import { createCardElement } from "../create-element/create-card-element";
import "../../../components/ha-grid-size-picker";

/** Port of HA `hui-card-layout-editor` */
@customElement("flow-card-layout-editor")
export class FlowCardLayoutEditor extends LitElement {
  @property({ attribute: false }) public config!: LovelaceCardConfig;

  @property({ attribute: false }) public sectionConfig?: LovelaceSectionConfig;

  @state() private _defaultGridOptions?: LovelaceGridOptions;

  @state() private _preciseMode = false;

  private async _loadDefaults(): Promise<void> {
    try {
      const card = await createCardElement(this.config);
      this._defaultGridOptions = card.getGridOptions?.() ?? {};
    } catch {
      this._defaultGridOptions = {};
    }
  }

  protected firstUpdated(): void {
    void this._loadDefaults();
  }

  protected willUpdate(changed: import("lit").PropertyValues): void {
    if (changed.has("config")) {
      const options = this.config.grid_options;
      if (!options) {
        this._preciseMode = this._defaultGridOptions
          ? isPreciseMode(this._defaultGridOptions)
          : false;
        return;
      }
      const preciseMode = isPreciseMode(options);
      if (!this._preciseMode && preciseMode) {
        this._preciseMode = preciseMode;
      }
    }
  }

  protected updated(changed: import("lit").PropertyValues): void {
    if (changed.has("config")) {
      void this._loadDefaults();
    }
  }

  private _configGridOptions(config: LovelaceCardConfig): LovelaceGridOptions {
    if (config.grid_options) return config.grid_options;
    if (config.layout_options) return migrateLayoutToGridOptions(config.layout_options);
    return {};
  }

  private _mergedOptions(
    options?: LovelaceGridOptions,
    defaultOptions?: LovelaceGridOptions,
  ): LovelaceGridOptions {
    return {
      ...DEFAULT_GRID_SIZE,
      ...defaultOptions,
      ...options,
    };
  }

  private _isDefault(options?: LovelaceGridOptions): boolean {
    return options?.columns === undefined && options?.rows === undefined;
  }

  private _updateGridOptions(options: LovelaceGridOptions): void {
    const value: LovelaceCardConfig = {
      ...this.config,
      grid_options: { ...options },
    };
    if (value.grid_options) {
      for (const [k, v] of Object.entries(value.grid_options)) {
        if (v === undefined) {
          delete value.grid_options[k as keyof LovelaceGridOptions];
        }
      }
      if (Object.keys(value.grid_options).length === 0) {
        delete value.grid_options;
      }
    }
    if (value.layout_options) {
      delete value.layout_options;
    }
    this.dispatchEvent(
      new CustomEvent("config-changed", {
        bubbles: true,
        composed: true,
        detail: { config: value },
      }),
    );
  }

  private _gridSizeChanged(ev: CustomEvent<{ value: CardGridSize }>): void {
    ev.stopPropagation();
    const value = ev.detail.value;
    this._updateGridOptions({
      ...this.config.grid_options,
      columns: value.columns,
      rows: value.rows,
    });
  }

  private _fullWidthChanged(ev: Event): void {
    ev.stopPropagation();
    const checked = (ev.target as HTMLInputElement).checked;
    const defaultGridOptions = {
      ...DEFAULT_GRID_SIZE,
      ...this._defaultGridOptions,
    };

    let columns: number | "full" | undefined;
    if (checked) {
      columns = "full";
    } else if (defaultGridOptions.columns === "full") {
      const columnSpan = this.sectionConfig?.column_span ?? 1;
      const gridTotalColumns = 12 * columnSpan;
      columns = defaultGridOptions.max_columns ?? gridTotalColumns;
    } else {
      columns = undefined;
    }

    this._updateGridOptions({
      ...this.config.grid_options,
      columns,
    });
  }

  private _autoHeightChanged(ev: Event): void {
    ev.stopPropagation();
    const checked = (ev.target as HTMLInputElement).checked;
    const defaultGridOptions = {
      ...DEFAULT_GRID_SIZE,
      ...this._defaultGridOptions,
    };

    let rows: number | "auto" | undefined;
    if (checked) {
      rows = "auto";
    } else if (defaultGridOptions.rows === "auto") {
      rows = defaultGridOptions.min_rows ?? 1;
    } else {
      rows = undefined;
    }

    this._updateGridOptions({
      ...this.config.grid_options,
      rows,
    });
  }

  private _preciseModeChanged(ev: Event): void {
    ev.stopPropagation();
    this._preciseMode = (ev.target as HTMLInputElement).checked;
    if (this._preciseMode) return;

    const columns = this.config.grid_options?.columns;
    if (typeof columns === "number" && columns % GRID_COLUMN_MULTIPLIER !== 0) {
      const newColumns =
        Math.ceil(columns / GRID_COLUMN_MULTIPLIER) * GRID_COLUMN_MULTIPLIER;
      this._updateGridOptions({
        ...this.config.grid_options,
        columns: newColumns,
      });
    }
  }

  protected render() {
    const configOptions = this._configGridOptions(this.config);
    const options = this._mergedOptions(configOptions, this._defaultGridOptions);
    const gridValue = computeCardGridSize(options);

    const columnSpan = this.sectionConfig?.column_span ?? 1;
    const gridTotalColumns = 12 * columnSpan;

    const autoHeight = options.rows === "auto";
    const fullWidth = options.columns === "full";
    const noGridSupport =
      this._defaultGridOptions !== undefined &&
      Object.keys(this._defaultGridOptions).length === 0;

    return html`
      ${noGridSupport
        ? html`
            <div class="alert">
              此卡片未声明网格尺寸默认值，布局调整可能不完全生效。
            </div>
          `
        : nothing}

      <ha-grid-size-picker
        class=${noGridSupport ? "disabled" : ""}
        .value=${gridValue}
        .rows=${8}
        .columns=${gridTotalColumns}
        .rowMin=${options.min_rows}
        .rowMax=${options.max_rows ?? 8}
        .columnMin=${options.min_columns}
        .columnMax=${options.max_columns ?? gridTotalColumns}
        .step=${this._preciseMode ? 1 : GRID_COLUMN_MULTIPLIER}
        .isDefault=${this._isDefault(configOptions)}
        @value-changed=${this._gridSizeChanged}
      ></ha-grid-size-picker>

      <label class="switch-row">
        <div>
          <div class="switch-label">自动高度</div>
          <div class="switch-hint">根据卡片内容调整卡片高度</div>
        </div>
        <input
          type="checkbox"
          role="switch"
          .checked=${autoHeight}
          @change=${this._autoHeightChanged}
        />
      </label>

      <label class="switch-row">
        <div>
          <div class="switch-label">全宽</div>
          <div class="switch-hint">占据整个部件的宽度，无论其大小</div>
        </div>
        <input
          type="checkbox"
          role="switch"
          .checked=${fullWidth}
          @change=${this._fullWidthChanged}
        />
      </label>

      <label class="switch-row">
        <div>
          <div class="switch-label">精确模式</div>
          <div class="switch-hint">更精确地更改卡片宽度（步进 1 而非 3）</div>
        </div>
        <input
          type="checkbox"
          role="switch"
          .checked=${this._preciseMode}
          @change=${this._preciseModeChanged}
        />
      </label>
    `;
  }

  static styles = css`
    :host {
      display: block;
    }
    .alert {
      padding: 12px;
      margin-bottom: 12px;
      border-radius: 8px;
      background: color-mix(in srgb, var(--primary-color) 12%, transparent);
      color: var(--primary-text-color);
      font-size: 13px;
    }
    ha-grid-size-picker {
      display: block;
      margin: 16px auto;
      direction: ltr;
    }
    ha-grid-size-picker.disabled {
      opacity: 0.5;
      pointer-events: none;
    }
    .switch-row {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 12px;
      cursor: pointer;
      padding: 12px 0;
      border-top: 1px solid var(--divider-color);
    }
    .switch-label {
      font-size: 14px;
      font-weight: 500;
      color: var(--primary-text-color);
    }
    .switch-hint {
      font-size: 12px;
      color: var(--secondary-text-color);
      margin-top: 2px;
      line-height: 1.35;
    }
    .switch-row input[type="checkbox"] {
      appearance: none;
      width: 36px;
      height: 20px;
      border-radius: 999px;
      background: var(--disabled-color, #9e9e9e);
      position: relative;
      flex-shrink: 0;
      cursor: pointer;
      transition: background 180ms ease;
    }
    .switch-row input[type="checkbox"]::after {
      content: "";
      position: absolute;
      top: 2px;
      left: 2px;
      width: 16px;
      height: 16px;
      border-radius: 50%;
      background: #fff;
      transition: transform 180ms ease;
    }
    .switch-row input[type="checkbox"]:checked {
      background: var(--primary-color);
    }
    .switch-row input[type="checkbox"]:checked::after {
      transform: translateX(16px);
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "flow-card-layout-editor": FlowCardLayoutEditor;
  }
}
