import { LitElement, css, html, nothing } from "lit";
import { customElement, property, state } from "lit/decorators.js";
import { styleMap } from "lit/directives/style-map.js";
import {
  DEFAULT_GRID_SIZE,
  type CardGridSize,
} from "../panels/lovelace/common/compute-card-grid-size";
import "../panels/lovelace/editor/ha-grid-layout-slider";
import "./ha-icon-button";
import { mdiRestore } from "@mdi/js";

function conditionalClamp(value: number, min?: number, max?: number): number {
  let result = value;
  if (min !== undefined) result = Math.max(result, min);
  if (max !== undefined) result = Math.min(result, max);
  return result;
}

/** Port of HA `ha-grid-size-picker` */
@customElement("ha-grid-size-picker")
export class HaGridSizePicker extends LitElement {
  @property({ attribute: false }) public value?: CardGridSize;

  @property({ type: Number }) public rows = 8;

  @property({ type: Number }) public columns = 12;

  @property({ type: Number }) public rowMin?: number;

  @property({ type: Number }) public rowMax?: number;

  @property({ type: Number }) public columnMin?: number;

  @property({ type: Number }) public columnMax?: number;

  @property({ type: Boolean }) public isDefault = false;

  @property({ type: Number }) public step = 1;

  @state() private _localValue?: CardGridSize = { rows: 1, columns: 1 };

  protected willUpdate(changed: Map<PropertyKey, unknown>): void {
    if (changed.has("value")) {
      this._localValue = this.value;
    }
  }

  private _cellClick(ev: Event): void {
    const cell = ev.currentTarget as HTMLElement;
    const rows = Number(cell.getAttribute("data-row"));
    const columns = Number(cell.getAttribute("data-column"));
    const clampedRow: CardGridSize["rows"] = conditionalClamp(rows, this.rowMin, this.rowMax);
    let clampedColumn: CardGridSize["columns"] = conditionalClamp(
      columns,
      this.columnMin,
      this.columnMax,
    );

    const currentSize = this.value ?? DEFAULT_GRID_SIZE;
    if (currentSize.columns === "full" && clampedColumn === this.columns) {
      clampedColumn = "full";
    }
    this.dispatchEvent(
      new CustomEvent("value-changed", {
        bubbles: true,
        composed: true,
        detail: { value: { rows: clampedRow, columns: clampedColumn } },
      }),
    );
  }

  private _valueChanged(ev: CustomEvent<{ value: number }>): void {
    ev.stopPropagation();
    const key = (ev.currentTarget as HTMLElement).id as "rows" | "columns";
    const currentSize = this.value ?? DEFAULT_GRID_SIZE;
    let value: CardGridSize[typeof key] = ev.detail.value as CardGridSize[typeof key];

    if (key === "columns" && currentSize.columns === "full" && value === this.columns) {
      value = "full";
    }

    this.dispatchEvent(
      new CustomEvent("value-changed", {
        bubbles: true,
        composed: true,
        detail: {
          value: {
            ...currentSize,
            [key]: value,
          },
        },
      }),
    );
  }

  private _reset(ev: Event): void {
    ev.stopPropagation();
    this.dispatchEvent(
      new CustomEvent("value-changed", {
        bubbles: true,
        composed: true,
        detail: {
          value: {
            rows: undefined,
            columns: undefined,
          },
        },
      }),
    );
  }

  private _sliderMoved(ev: CustomEvent<{ value?: number }>): void {
    ev.stopPropagation();
    const key = (ev.currentTarget as HTMLElement).id as "rows" | "columns";
    const currentSize = this.value ?? DEFAULT_GRID_SIZE;
    const value = ev.detail.value;
    if (value === undefined) return;
    this._localValue = {
      ...currentSize,
      [key]: value,
    };
  }

  protected render() {
    const autoHeight = this._localValue?.rows === "auto";
    const fullWidth = this._localValue?.columns === "full";

    const disabledColumns =
      fullWidth || (this.columnMin !== undefined && this.columnMin === this.columnMax);
    const disabledRows =
      autoHeight || (this.rowMin !== undefined && this.rowMin === this.rowMax);

    const rowMin = this.rowMin ?? 1;
    const rowMax = this.rowMax ?? this.rows;
    const columnMin = Math.ceil((this.columnMin ?? 1) / this.step) * this.step;
    const columnMax = Math.ceil((this.columnMax ?? this.columns) / this.step) * this.step;
    const rowValue = autoHeight ? rowMin : this._localValue?.rows;
    const columnValue = this._localValue?.columns;

    const previewRows = autoHeight ? 1 : Number(this._localValue?.rows) || 1;
    const previewCols = fullWidth ? this.columns : Number(this._localValue?.columns) || 1;

    return html`
      <div
        class="grid"
        style=${styleMap({
          "--preview-rows": String(this.rows),
        })}
      >
        <ha-grid-layout-slider
          id="columns"
          .value=${typeof columnValue === "number" ? columnValue : this.columns}
          .min=${columnMin}
          .max=${columnMax}
          .range=${this.columns}
          .step=${this.step}
          .disabled=${disabledColumns}
          @slider-moved=${this._sliderMoved}
          @value-changed=${this._valueChanged}
        ></ha-grid-layout-slider>

        <div class="reset">
          ${!this.isDefault
            ? html`
                <ha-icon-button
                  .path=${mdiRestore}
                  label="恢复默认"
                  @click=${this._reset}
                ></ha-icon-button>
              `
            : nothing}
        </div>

        <div
          class="preview"
          style=${styleMap({
            "--rows": String(previewRows),
            "--columns": String(previewCols),
            "--total-columns": String(this.columns),
          })}
        >
          <table>
            ${Array(this.rows)
              .fill(0)
              .map((_, index) => {
                const row = index + 1;
                return html`
                  <tr>
                    ${Array(this.columns)
                      .fill(0)
                      .map((__, columnIndex) => {
                        const column = columnIndex + 1;
                        if (
                          column % this.step !== 0 ||
                          (this.columns > 24 && column % 3 !== 0)
                        ) {
                          return nothing;
                        }
                        return html`
                          <td
                            data-row=${row}
                            data-column=${column}
                            @click=${this._cellClick}
                          ></td>
                        `;
                      })}
                  </tr>
                `;
              })}
          </table>
          <div class="preview-card"></div>
        </div>

        <ha-grid-layout-slider
          vertical
          id="rows"
          .value=${typeof rowValue === "number" ? rowValue : rowMin}
          .min=${rowMin}
          .max=${rowMax}
          .range=${this.rows}
          .disabled=${disabledRows}
          @slider-moved=${this._sliderMoved}
          @value-changed=${this._valueChanged}
        ></ha-grid-layout-slider>
      </div>
    `;
  }

  static styles = css`
    .grid {
      display: grid;
      grid-template-areas:
        "reset column-slider"
        "row-slider preview";
      grid-template-rows: auto auto;
      grid-template-columns: auto 1fr;
      gap: var(--ha-space-2, 8px);
      direction: ltr;
    }
    #columns {
      grid-area: column-slider;
    }
    #rows {
      grid-area: row-slider;
      height: calc(var(--preview-rows, 8) * 30px);
      align-self: stretch;
    }
    .reset {
      grid-area: reset;
      --ha-icon-button-size: 36px;
    }
    .preview {
      position: relative;
      grid-area: preview;
    }
    .preview table,
    .preview tr,
    .preview td {
      border: 2px dotted var(--divider-color);
      border-collapse: collapse;
    }
    .preview table {
      width: 100%;
    }
    .preview tr {
      height: 30px;
    }
    .preview td {
      cursor: pointer;
    }
    .preview-card {
      position: absolute;
      top: 0;
      left: 0;
      background-color: var(--primary-color);
      opacity: 0.3;
      border-radius: var(--ha-border-radius-md, 8px);
      height: calc(var(--rows, 1) * 30px);
      width: calc(var(--columns, 1) * 100% / var(--total-columns, 12));
      pointer-events: none;
      transition:
        width ease-in-out 180ms,
        height ease-in-out 180ms;
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "ha-grid-size-picker": HaGridSizePicker;
  }
}
