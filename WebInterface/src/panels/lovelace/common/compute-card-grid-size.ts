import type { LovelaceGridOptions, LovelaceLayoutOptions } from "../../../types";

export const GRID_COLUMN_MULTIPLIER = 3;

function multiplyBy<T>(value: T, multiplier: number): T {
  return typeof value === "number" ? ((value * multiplier) as T) : value;
}

export function migrateLayoutToGridOptions(
  options: LovelaceLayoutOptions,
): LovelaceGridOptions {
  const gridOptions: LovelaceGridOptions = {
    columns: multiplyBy(options.grid_columns, GRID_COLUMN_MULTIPLIER),
    max_columns: multiplyBy(options.grid_max_columns, GRID_COLUMN_MULTIPLIER),
    min_columns: multiplyBy(options.grid_min_columns, GRID_COLUMN_MULTIPLIER),
    rows: options.grid_rows,
    max_rows: options.grid_max_rows,
    min_rows: options.grid_min_rows,
  };
  for (const [key, value] of Object.entries(gridOptions)) {
    if (value === undefined) {
      delete gridOptions[key as keyof LovelaceGridOptions];
    }
  }
  return gridOptions;
}

export interface CardGridSize {
  rows: number | "auto";
  columns: number | "full";
}

/** Port of HA DEFAULT_GRID_SIZE */
export const DEFAULT_GRID_SIZE: CardGridSize = {
  columns: 12,
  rows: "auto",
};

function clamp(value: number, min?: number, max?: number): number {
  let result = value;
  if (min !== undefined) result = Math.max(result, min);
  if (max !== undefined) result = Math.min(result, max);
  return result;
}

/** Port of HA computeCardGridSize */
export function computeCardGridSize(options: LovelaceGridOptions = {}): CardGridSize {
  const rows = options.rows ?? "auto";
  const columns = options.columns ?? 12;
  const minRows = options.min_rows;
  const maxRows = options.max_rows;
  const minColumns = options.min_columns;
  const maxColumns = options.max_columns;

  const clampedRows =
    typeof rows === "string" ? rows : clamp(rows, minRows, maxRows);

  const clampedColumns =
    typeof columns === "string" ? columns : clamp(columns, minColumns, maxColumns);

  return {
    rows: clampedRows,
    columns: clampedColumns,
  };
}

/** Apply CardGridSize to CSS variables for hui-card-container */
export function cardGridSizeToCssVars(size: CardGridSize): {
  columnSize: number;
  rowSize: number;
  fullWidth: boolean;
  autoHeight: boolean;
} {
  const fullWidth = size.columns === "full";
  const autoHeight = size.rows === "auto";
  const columnSize = fullWidth ? 12 : (size.columns as number);
  const rowSize = autoHeight ? 1 : (size.rows as number);

  return { columnSize, rowSize, fullWidth, autoHeight };
}

export function isPreciseMode(options: LovelaceGridOptions): boolean {
  return typeof options.columns === "number" && options.columns % GRID_COLUMN_MULTIPLIER !== 0;
}
