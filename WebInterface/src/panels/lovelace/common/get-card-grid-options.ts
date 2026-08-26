import type { LovelaceCard, LovelaceCardConfig, LovelaceGridOptions } from "../../../types";
import { migrateLayoutToGridOptions } from "./compute-card-grid-size";

export const DEFAULT_GRID_SIZE: LovelaceGridOptions = {
  columns: 12,
  rows: "auto",
};

function getConfigGridOptions(config?: LovelaceCardConfig): LovelaceGridOptions {
  if (config?.grid_options) {
    return config.grid_options;
  }
  if (config?.layout_options) {
    return migrateLayoutToGridOptions(config.layout_options);
  }
  return {};
}

/** Merge element defaults with config.grid_options (HA hui-card.getGridOptions) */
export function getCardGridOptions(
  elementOptions: LovelaceGridOptions = {},
  config?: LovelaceCardConfig,
): LovelaceGridOptions {
  const configOptions = getConfigGridOptions(config);
  const merged: LovelaceGridOptions = {
    ...DEFAULT_GRID_SIZE,
    ...elementOptions,
    ...configOptions,
  };

  // Layout-driven cards may have stale persisted rows; never go below element minimums.
  if (
    typeof elementOptions.min_rows === "number" &&
    typeof merged.rows === "number" &&
    merged.rows < elementOptions.min_rows
  ) {
    merged.rows = elementOptions.min_rows;
  }
  if (
    typeof elementOptions.min_columns === "number" &&
    typeof merged.columns === "number" &&
    merged.columns < elementOptions.min_columns
  ) {
    merged.columns = elementOptions.min_columns;
  }

  return merged;
}

export function getMergedGridOptionsFromCard(
  card: LovelaceCard | undefined,
  config?: LovelaceCardConfig,
): LovelaceGridOptions {
  return getCardGridOptions(card?.getGridOptions?.() ?? {}, config);
}
